auto File::Read(PathRef path) noexcept -> Result<FileContents> {
    FileContents c;
    Try(ReadInto(path, c.vec));
    return c;
}

auto fs::ExecutablePath() -> Result<Path> {
    return Error("ExecutablePath() is not supported on this platform");
}
