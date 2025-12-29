#ifndef LIBBASE_FS_HH
#define LIBBASE_FS_HH

#include <base/Assert.hh>
#include <base/detail/SystemInfo.hh>
#include <base/Macros.hh>
#include <base/Result.hh>
#include <base/Span.hh>
#include <base/Str.hh>
#include <base/Types.hh>
#include <base/Utils.hh>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace base::fs {
namespace stdfs = std::filesystem;
class File;
class FileContentsBase;
}

#ifdef LIBBASE_FS_LINUX
#    include <base/detail/FSLinux.hh>
#else
#    include <base/detail/FSGeneric.hh>
#endif

namespace base::fs {
class FileContents;

enum struct OpenMode : u8;

using InputView [[deprecated("Use ByteSpan instead")]] = ByteSpan;
using OutputView [[deprecated("Use MutableByteSpan instead")]] = MutableByteSpan;
using InputVector = Span<ByteSpan>;
using Path = stdfs::path;
using PathRef = const stdfs::path&;

/// Change the current working directory.
auto ChangeDirectory(PathRef path) -> Result<>;

/// Get the path to the current working directory.
auto CurrentDirectory() -> Path;

/// Get the path to the current executable.
///
/// This can fail on platforms where this isn’t supported.
auto ExecutablePath() -> Result<Path>;

/// Get a vector containing all files in a directory.
///
/// This is basically just a wrapper around IterateFilesInDirectory().
///
/// \see IterateFilesInDirectory()
auto GetFilesInDirectory(PathRef dir, bool recursive) -> Result<std::vector<Path>>;

/// Iterate over all files in a directory.
///
/// This follows symlinks and ignores anything that is not
/// a regular file. The iteration order is unspecified.
///
/// \tparam recursive Iterate over subdirectories as well.
/// \param dir The directory whose to iterate.
/// \return A Result<T>, where T is an iterable range of std::filesystem::directory_entry.
/// \see GetFilesInDirectory() if you want a vector of paths.
template <bool recursive>
auto IterateFilesInDirectory(PathRef dir) {
    auto filter = +[](const stdfs::directory_entry& e) { return e.is_regular_file(); };
    using It = std::conditional_t<recursive, stdfs::recursive_directory_iterator, stdfs::directory_iterator>;
    using Res = Result<rgs::filter_view<It, decltype(filter)>>;

    std::error_code ec;
    It it{dir, ec};
    if (not ec) return Res{vws::filter(std::move(it), filter)};
    return Res{Error("Could not iterate directory '{}': {}", dir.string(), ec.message())};
}

/// Get a temporary file path.
auto TempPath(str extension = {}) -> std::string;
} // namespace base::fs

namespace base {
using fs::File;
}

/// Do NOT use this as a bitmask! Only ever supply at most one of
/// these. The values of the enumerators are an implementation detail.
enum struct base::fs::OpenMode : base::u8 {
    /// Open the file for reading. The file must exist.
    Read = 1,

    /// Open the file for writing. Truncates the file if it
    /// exists, creates it if it does not.
    Write = 2,

    /// Same as Write, but does not perform truncation.
    Append = 4,

    /// Open the file for reading and writing.
    ReadWrite = Read | Write,

    /// Open the file for reading and appending.
    ReadAppend = Read | Write | Append,
};

class base::fs::FileContents : public FileContentsBase {
public:
    using FileContentsBase::FileContentsBase;

    /// Get the contents as a span.
    [[nodiscard]] auto span() const -> ByteSpan {
        return {data<std::byte>(), size()};
    }

    /// Get the contents as a string view.
    [[nodiscard]] auto view() const -> std::string_view {
        return {data(), size()};
    }

    /// Compare the contents of two files.
    [[nodiscard]] bool operator==(const FileContents& other) const {
        return span() == other.span();
    }

    [[nodiscard]] auto operator<=>(const FileContents& other) const {
        return span() <=> other.span();
    }

    [[nodiscard]] operator std::string_view() const { return view(); }
    [[nodiscard]] operator str() const { return view(); }
};

/// A handle to a file on disk.
///
/// The file is closed when this goes out of scope. This should
/// only be used for actual files, not for (named) pipes, sockets,
/// etc.
class base::fs::File {
    OpenMode open_mode;
    std::unique_ptr<FILE, decltype(&std::fclose)> handle{nullptr, std::fclose};
    Path abs_path; // Hack so we can truncate the file.

public:
    /// Get the open mode of the file.
    [[nodiscard]] auto mode() const noexcept -> OpenMode { return open_mode; }

    /// Print to the file.
    template <typename... Args>
    auto print(std::format_string<Args...> fmt, Args&&... args) noexcept -> Result<> {
        return write(std::format(fmt, std::forward<Args>(args)...));
    }

    /// Read from the file.
    ///
    /// \param into The buffer to read into. This will try to read
    ///        `into.size()` bytes from the file.
    /// \return A `Result` containing the number of bytes read, or an
    ///         error if the file could not be read from. A short read
    ///         is not an error and can happen if the file is smaller
    ///         than the buffer.
    auto read(MutableByteSpan into) noexcept -> Result<usz>;

    /// Rewind the file to the beginning.
    void rewind() noexcept;

    /// Get the size of this file.
    auto size() noexcept -> usz;

    /// Truncate or extend the file to a given size.
    auto resize(usz size) noexcept -> Result<>;

    /// Write to the file.
    auto write(ByteSpan data) noexcept -> Result<>;

    /// Write to the file using scatter/gather I/O.
    auto writev(InputVector data) noexcept -> Result<>;

    /// Delete a file or directory.
    static auto Delete(PathRef path, bool recursive = false) -> Result<bool>;

    /// Check if a file exists.
    static auto Exists(PathRef path) noexcept -> bool;

    /// Open a file.
    ///
    /// \param path The path to the file.
    /// \param mode The mode to open the file with.
    /// \return A `Result` containing the opened file, or an error if
    ///         it could not be opened, for whatever reason.
    static auto Open(PathRef path, OpenMode mode = OpenMode::Read) noexcept -> Result<File>;

    /// Read an entire file into a container.
    template <utils::ResizableByteRange Buffer>
    static auto ReadInto(PathRef path, Buffer& buffer) -> Result<> {
        auto sz = buffer.size();
        auto f = Try(Open(path, OpenMode::Read));
        buffer.resize(sz + f.size());

        // A short read is possible here if the file is being resized by
        // someone else; there is nothing we can really do about that, so
        // don’t bother checking the size here.
        Try(f.read(MutableByteSpan(buffer.data() + sz, f.size())));
        return {};
    }

    /// Overload of ReadInto() that behaves like Read() for generic programming.
    static auto ReadInto(PathRef path, FileContents& contents) -> Result<> {
        contents = Try(Read(path));
        return {};
    }

    /// Read an entire file into a container.
    template <typename Buffer = std::string>
    static auto ReadToContainer(PathRef path) noexcept -> Result<Buffer> {
        Buffer buffer;
        Try(ReadInto(path, buffer));
        return buffer;
    }

    /// Read an entire file and get the raw data.
    static auto Read(PathRef path) noexcept -> Result<FileContents>;

    /// Write data to a file on disk.
    ///
    /// This automatically creates any intermediate
    /// directories that do not exist.
    static auto Write(PathRef path, ByteSpan data) noexcept -> Result<>;
};

template <>
struct std::formatter<base::fs::FileContents> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const base::fs::FileContents& contents, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(contents.view(), ctx);
    }
};

#endif
