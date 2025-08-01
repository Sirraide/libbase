#ifndef LIBBASE_FS_HH
#define LIBBASE_FS_HH

#include <base/Assert.hh>
#include <base/detail/SystemInfo.hh>
#include <base/Macros.hh>
#include <base/Result.hh>
#include <base/Types.hh>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace base::fs {
namespace stdfs = std::filesystem;
class File;
class FileContents;
}

#ifdef LIBBASE_FS_LINUX
#    include <base/detail/FSLinux.hh>
#else
#    include <base/detail/FSGeneric.hh>
#endif

namespace base::fs {
struct InputView;
struct OutputView;

enum struct OpenMode : u8;

using InputVector = std::span<const InputView>;
using Path = stdfs::path;
using PathRef = const stdfs::path&;

template <typename Ty>
concept CharBuffer = requires (Ty t, usz sz) {
    { t.data() } -> std::same_as<char*>;
    { t.size() } -> std::integral;
    { t.resize(sz) };
};

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

struct base::fs::InputView : std::span<const std::byte> {
    using std::span<const std::byte>::span;
    using std::span<const std::byte>::operator=;

    // Allow construction from string view.
    InputView(std::string_view sv) noexcept
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(sv.data()),
              sv.size()
          ) {}

    // Allow construction from char buffer.
    InputView(const CharBuffer auto& buf) noexcept
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(buf.data()),
              buf.size()
          ) {}

    // Allow wrapping a compile-time constant char array.
    template <usz N>
    InputView(const char (&arr)[N]) noexcept
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(arr),
              arr[N - 1] == '\0' ? N - 1 : N
          ) {}
};

struct base::fs::OutputView : std::span<std::byte> {
    using std::span<std::byte>::span;
    using std::span<std::byte>::operator=;

    // Allow construction from char range.
    OutputView(std::span<char> buf) noexcept
        : std::span<std::byte>(
              reinterpret_cast<std::byte*>(buf.data()),
              buf.size()
          ) {}

    // Allow construction from char buffer.
    OutputView(CharBuffer auto& buf) noexcept
        : std::span<std::byte>(
              reinterpret_cast<std::byte*>(buf.data()),
              buf.size()
          ) {}
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
    auto read(OutputView into) noexcept -> Result<usz>;

    /// Rewind the file to the beginning.
    void rewind() noexcept;

    /// Get the size of this file.
    auto size() noexcept -> usz;

    /// Truncate or extend the file to a given size.
    auto resize(usz size) noexcept -> Result<>;

    /// Write to the file.
    auto write(InputView data) noexcept -> Result<>;

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
    template <CharBuffer Buffer>
    static auto ReadInto(PathRef path, Buffer& buffer) noexcept -> Result<> {
        auto sz = buffer.size();
        auto f = Try(Open(path, OpenMode::Read));
        buffer.resize(sz + f.size());

        // A short read is possible here if the file is being resized by
        // someone else; there is nothing we can really do about that, so
        // don’t bother checking the size here.
        Try(f.read(std::span(buffer.data() + sz, f.size())));
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
    static auto Write(PathRef path, InputView data) noexcept -> Result<>;
};

#endif
