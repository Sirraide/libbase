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

auto FileImpl::open(PathRef path, OpenMode mode) noexcept -> Result<> {
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
    auto ptr = std::fopen(path.c_str(), mode_str.c_str());
    if (not ptr) return Error("Could not open file: {}", std::strerror(errno));
    handle.reset(ptr);

    // Save the flags.
    static_cast<File*>(this)->open_mode = mode;

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
