#include <base/Base.hh>
#include <base/FS.hh>
#include <cerrno>
#include <cstring>
#include <filesystem>

#ifdef __linux__
#    include <fcntl.h>
#    include <linux/limits.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

using namespace base;
using namespace base::fs;

namespace std_fs = std::filesystem;

#ifdef LIBBASE_FS_LINUX
#    include "FSLinux.inc"
#else
#    include "FSGeneric.inc"
#endif

auto fs::ChangeDirectory(PathRef path) -> Result<> {
    std::error_code ec;
    std_fs::current_path(path, ec);
    if (ec) return Error("Could not change directory to '{}': {}", path.string(), ec.message());
    return {};
}

auto fs::CurrentDirectory() -> Path {
    // If this ever errors, your system is broken.
    std::error_code ec;
    return std_fs::current_path(ec);
}

auto File::Delete(PathRef path, bool recursive) -> Result<bool> {
    std::error_code ec;
    if (recursive) {
        auto deleted = std_fs::remove_all(path, ec);
        if (ec) return Error("Could not remove path '{}': {}", path.string(), ec.message());
        return deleted;
    }

    auto deleted = std_fs::remove(path, ec);
    if (ec) return Error("Could not delete path '{}': {}", path.string(), ec.message());
    return deleted;
}

auto File::Exists(PathRef path) noexcept -> bool {
    std::error_code ec;
    return std_fs::exists(path, ec);
}

auto File::Open(PathRef path, OpenMode mode) noexcept -> Result<File> {
    std::string mode_str;

    switch (mode) {
        default: return Error("Invalid open mode '{}'", +mode);
        case OpenMode::Read: mode_str = "r"; break;
        case OpenMode::Write: mode_str = "w"; break;
        case OpenMode::Append: mode_str = "a"; break;
        case OpenMode::ReadWrite: mode_str = "w+"; break;
        case OpenMode::ReadAppend: mode_str = "a+"; break;
    }

    // Always use binary mode.
    mode_str += 'b';

    // Dew it.
    auto ptr = std::fopen(path.string().c_str(), mode_str.c_str());
    if (not ptr) return Error("Could not open file: {}", std::strerror(errno));
    File f;
    f.handle.reset(ptr);

    // Save the flags.
    f.open_mode = mode;

    // Save the path.
    std::error_code ec;
    f.abs_path = std_fs::absolute(path, ec);
    if (ec) return Error("Could not get absolute path: {}", ec.message());
    return f;
}

auto File::Write(PathRef path, InputView data) noexcept -> Result<> {
    std::error_code ec;
    auto parent = path.parent_path();
    if (not parent.empty()) std_fs::create_directories(parent, ec);
    if (ec) return Error("Could not create directories for '{}': {}", path.string(), ec.message());
    auto f = Try(Open(path, OpenMode::Write));
    return f.write(data);
}

auto File::read(OutputView into) noexcept -> Result<usz> {
    if ((+open_mode & +OpenMode::Read) == 0) return Error("File is not open for reading");
    usz n_read = 0;
    while (not into.empty() and not std::feof(handle.get())) {
        auto r = std::fread(into.data(), 1, into.size(), handle.get());
        if (std::ferror(handle.get())) return Error(
            "Could not read from file: {}",
            std::strerror(errno)
        );

        n_read += r;
        into = into.subspan(r);
    }
    return n_read;
}

void File::rewind() noexcept {
    std::rewind(handle.get());
}

auto File::resize(usz size) noexcept -> Result<> {
    std::error_code ec;
    std_fs::resize_file(abs_path, size, ec);
    if (ec) return Error("Could not resize file: {}", ec.message());
    return {};
}

auto File::size() noexcept -> usz {
    std::error_code ec;
    auto s = std_fs::file_size(abs_path, ec);
    if (ec) return 0;
    return s;
}

auto File::write(InputView data) noexcept -> Result<> {
    if ((+open_mode & +OpenMode::Write) == 0) return Error("File is not open for writing");
    while (not data.empty()) {
        auto r = std::fwrite(data.data(), 1, data.size(), handle.get());
        if (std::ferror(handle.get())) return Error(
            "Could not write to file: {}",
            std::strerror(errno)
        );
        data = data.subspan(r);
    }
    return {};
}

auto File::writev(InputVector data) noexcept -> Result<> {
    if ((+open_mode & +OpenMode::Write) == 0) return Error("File is not open for writing");

    // <cstdio> doesn’t support scatter/gather I/O :(.
    for (auto& d : data) Try(write(d));
    return {};
}
