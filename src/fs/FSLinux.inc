void FileContents::Delete() {
    if (ptr) munmap(ptr, sz);
}

auto File::Read(PathRef path) noexcept -> Result<FileContents> {
    auto Err = [&](std::string_view action) {
        return Error(
            "Could not {} file '{}': {}",
            action,
            path.string(),
            std::strerror(errno)
        );
    };

    // Open the file.
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd == -1) return Err("open");
    defer { close(fd); };

    // Stat it to get its contents.
    struct stat64 st{};
    if (fstat64(fd, &st) == -1) return Err("stat");

    // If stat returns 0, then mmap would fail (because we can’t
    // mmap 0 bytes, so just return early here).
    if (st.st_size == 0) return FileContents();

    // Otherwise, map the file contents.
    void* ptr = mmap(nullptr, usz(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) return Err("mmap");
    return FileContents{ptr, usz(st.st_size)};
}
