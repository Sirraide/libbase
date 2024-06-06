#include <base/detail/FSGeneric.hh>
#include <cerrno>
#include <cstring>

using namespace base;

auto FileImpl::Delete(PathRef path, bool recursive) -> Result<bool> {
    std::error_code ec;
    if (recursive) {
        auto deleted = std::filesystem::remove_all(path, ec);
        if (ec) return Error("Could not remove path '{}': {}", path.string(), ec.message());
        return deleted;
    }

    auto deleted = std::filesystem::remove(path, ec);
    if (ec) return Error("Could not delete path '{}': {}", path.string(), ec.message());
    return deleted;
}

auto FileImpl::Exists(PathRef path) noexcept -> bool {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

auto FileImpl::open(PathRef path, OpenFlags flags) noexcept -> Result<> {
    using enum OpenFlags;
    static constexpr OpenFlags AllModes = Read | Write | Append;
    static constexpr OpenFlags All = AllModes | Create;

    // Save the flags before adjusting them; this way, we can
    // still disallow writing to the file in read+create mode.
    static_cast<File*>(this)->open_flags = flags;

    // If the user specified Create only, also open it for writing;
    // there is no other good way of doing this with just fopen().
    if (flags == Create) flags = Append | Create;

    // Check for invalid flags.
    if ((+flags & +All) != +flags) return Error("Invalid flags specified");
    if (flags & Append and flags & Write) return Error("'Append' and 'Write' are exclusive");
    if (not(flags & AllModes)) return Error("No access mode specified");
    std::string mode;

    // We map combinations of flags to the closest representation
    // that 'fopen()' happens to support.
    //
    // Honour lack of Create in Read mode only.
    //
    //   CRWA
    //   0100 = R  -> "r"
    //   0110 = RW -> "r+"
    if (flags == Read or flags == ReadWrite) mode = flags == Read ? "r" : "r+";

    // Ignore Create after this. It is always implied. Not all modes
    // listed below are legal, but we’ve already checked for those.
    //
    //   CRWA
    //   X001 = A   -> "a"
    //   X010 = W   -> "w"
    //   X011 = WA  ->  !
    //   X100 = R   -> "a+"
    //   X101 = RA  -> "a+"
    //   X110 = RW  -> "w+"
    //   X111 = RWA ->  !
    else {
        flags = OpenFlags(+flags & ~+Create);
        mode = flags == Append     ? "a"
             : flags == Write      ? "w"
             : flags == Read       ? "a+"
             : flags == ReadAppend ? "a+"
             : flags == ReadWrite  ? "w+"
                                   : "";
    }

    // Always use binary mode.
    mode += 'b';


    // Dew it.
    auto ptr = std::fopen(path.c_str(), mode.c_str());
    if (not ptr) return Error("Could not open file: {}", std::strerror(errno));
    handle.reset(ptr);

    // Save the path.
    std::error_code ec;
    abs_path = std::filesystem::absolute(path, ec);
    if (ec) return Error("Could not get absolute path: {}", ec.message());
    return {};
}

auto FileImpl::read(OutputView into) noexcept -> Result<usz> {
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

void FileImpl::rewind() noexcept {
    std::rewind(handle.get());
}

inline auto FileImpl::resize(usz size) noexcept -> Result<> {
    std::error_code ec;
    std::filesystem::resize_file(abs_path, size, ec);
    if (ec) return Error("Could not resize file: {}", ec.message());
    return {};
}

auto FileImpl::size() noexcept -> usz {
    std::error_code ec;
    auto s = std::filesystem::file_size(abs_path, ec);
    if (ec) return 0;
    return s;
}

auto FileImpl::write(InputView data) noexcept -> Result<> {
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

auto FileImpl::writev(InputVector data) noexcept -> Result<> {
    // <cstdio> doesn’t support scatter/gather I/O :(.
    for (auto& d : data) Try(write(d));
    return {};
}
