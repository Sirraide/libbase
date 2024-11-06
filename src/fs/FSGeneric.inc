auto File::Read(PathRef path) noexcept -> Result<FileContents> {
    FileContents c;
    Try(ReadInto(path, c.vec));
    return c;
}
