#ifndef LIBBASE_FS_HH_
#define LIBBASE_FS_HH_

#include <base/detail/FSCommon.hh>
#include <base/Numeric.hh>
#include <base/Types.hh>
#include <memory>
#include <cstring>

#define LIBBASE_USE_GENERIC_FILESYSTEM_IMPL

#ifdef LIBBASE_USE_GENERIC_FILESYSTEM_IMPL
#    include <base/detail/FSGeneric.hh>
#else
#    ifdef __linux__
#        include <base/detail/FSLinux.hh>
#    else
#        include <base/detail/FSGeneric.hh>
#    endif
#endif

namespace base {
/// A handle to a file on disk.
///
/// The file is closed when this goes out of scope. This should
/// only be used for actual files, not for (named) pipes, sockets,
/// etc.
class File : FileImpl {
    friend FileImpl;
    OpenFlags open_flags;

public:
    /// Get the open flags for the file.
    [[nodiscard]] auto flags() const noexcept -> OpenFlags { return open_flags; }

    /// Print to the file.
    template <typename ...Args>
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
    auto read(std::span<char> into) noexcept -> Result<usz>;

    /// Rewind the file to the beginning.
    void rewind() noexcept;

    /// Get the size of this file.
    auto size() noexcept -> usz;

    /// Write to the file.
    auto write(std::span<const char> data) noexcept -> Result<>;

    /// Write to the file using scatter/gather I/O.
    auto writev(std::span<const std::span<const char>> data) noexcept -> Result<>;

    /// Delete a file or directory.
    static auto Delete(PathRef path, bool recursive = false) -> Result<>;

    /// Open a file.
    ///
    /// \param path The path to the file.
    /// \param flags The flags to open the file with.
    /// \return A `Result` containing the opened file, or an error if
    ///         it could not be opened, for whatever reason.
    static auto Open(
        PathRef path,
        OpenFlags flags = OpenFlags::Read | OpenFlags::Create
    ) noexcept -> Result<File>;

    /// Read an entire file into a string.
    template <CharBuffer Buffer>
    static auto ReadInto(PathRef path, Buffer& buffer) noexcept -> Result<> {
        auto sz = buffer.size();
        auto f = Try(Open(path, OpenFlags::Read));
        buffer.resize(sz + f.size());

        // A short read is possible here if the file is being resized by
        // someone else; there is nothing we can really do about that, so
        // don’t bother checking the size here.
        Try(f.read(std::span(buffer.data() + sz, f.size())));
        return {};
    }

    /// Read an entire file into a string.
    template <typename Buffer = std::string>
    static auto Read(PathRef path) noexcept -> Result<Buffer> {
        Buffer buffer;
        Try(ReadInto(path, buffer));
        return buffer;
    }

    /// Write data to a file on disk.
    static auto Write(PathRef path, std::span<const char> data) noexcept -> Result<>;
};
} // namespace base

#endif // LIBBASE_FS_HH_
