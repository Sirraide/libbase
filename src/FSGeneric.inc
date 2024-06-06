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
    if (flags & Create) flags |= Write;

    // Check for invalid flags.
    if ((+flags & +All) != +flags) return Error("Invalid flags specified");
    if (flags & Append and flags & Write) return Error("'Append' and 'Write' are exclusive");
    if (not(flags & AllModes)) return Error("No access mode specified");

    // Convert flags to mode string.
    std::string fstr;
    if (flags & Read) fstr += 'r';
    if (flags & Write) fstr += 'w';
    if (flags & Append) fstr += 'a';
    fstr += 'b';

    // Dew it.
    auto ptr = std::fopen(path.c_str(), fstr.c_str());
    if (not ptr) return Error("Could not open file: {}", std::strerror(errno));
    handle.reset(ptr);
    return {};
}

auto FileImpl::read(std::span<char> into) noexcept -> Result<usz> {
    usz n_read = 0;
    while (not into.empty() and not std::feof(handle.get())) {
        auto r = std::fread(into.data(), 1, into.size(), handle.get());
        if (auto err = std::ferror(handle.get())) return Error(
            "Could not read from file: {}",
            std::strerror(err)
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

auto FileImpl::write(std::span<const char> data) noexcept -> Result<> {
    while (not data.empty()) {
        auto r = std::fwrite(data.data(), 1, data.size(), handle.get());
        if (auto err = std::ferror(handle.get())) return Error(
            "Could not write to file: {}",
            std::strerror(err)
        );
        data = data.subspan(r);
    }
    return {};
}

auto FileImpl::writev(std::span<const std::span<const char>> data) noexcept -> Result<> {
    // <cstdio> doesn’t support scatter/gather I/O :(.
    for (auto& d : data) Try(write(d));
    return {};
}
