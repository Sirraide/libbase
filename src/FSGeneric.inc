#include <base/detail/FSGeneric.hh>
#include <cerrno>
#include <cstring>

using namespace base;

auto FileImpl::Delete(PathRef path, bool recursive) -> Result<> {
    std::error_code ec;
    if (recursive) {
        std::filesystem::remove_all(path, ec);
        if (ec) return Error("Could not remove path '{}': {}", path.string(), ec.message());
        return {};
    }

    std::filesystem::remove(path, ec);
    if (ec) return Error("Could not deleting path '{}': {}", path.string(), ec.message());
    return {};
}

auto FileImpl::open(PathRef path, OpenFlags flags) noexcept -> Result<> {
    using enum OpenFlags;
    static constexpr OpenFlags AllModes = Read | Write | Append;
    static constexpr OpenFlags All = AllModes | Create;

    // Save the flags before adjusting them; this way, we can
    // still disallow writing to the file in read+create mode.
    static_cast<File*>(this)->open_flags = flags;

    // If the user specified Create, also open it for writing; there
    // is no other good way of doing this with just fopen().
    if (flags & Create) flags |= Append;

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
    return {};
}

auto FileImpl::read(std::span<std::byte> into) noexcept -> Result<usz> {
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

auto FileImpl::size() noexcept -> usz {
    auto offs = std::ftell(handle.get());
    if (offs == -1) return 0;
    std::fseek(handle.get(), 0, SEEK_END);
    auto size = std::ftell(handle.get());
    std::fseek(handle.get(), offs, SEEK_SET);
    return usz(size);
}

auto FileImpl::write(std::span<const std::byte> data) noexcept -> Result<> {
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

auto FileImpl::writev(std::span<const std::span<const std::byte>> data) noexcept -> Result<> {
    // <cstdio> doesn’t support scatter/gather I/O :(.
    for (auto& d : data) Try(write(d));
    return {};
}
