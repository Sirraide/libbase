#include <base/FS.hh>

#ifdef LIBBASE_USE_GENERIC_FILESYSTEM_IMPL
#    include "FSGeneric.inc"
#else
#    ifdef __linux__
#        include "FSLinux.inc"
#    else
#        include "FSGeneric.inc"
#    endif
#endif

using namespace base;

auto File::Delete(PathRef path, bool recursive) -> Result<> {
    return FileImpl::Delete(path, recursive);
}

auto File::Open(PathRef path, OpenFlags flags) noexcept -> Result<File> {
    File f;
    Try(f.open(path, flags));
    return f;
}

auto File::Write(PathRef path, std::span<const char> data) noexcept -> Result<> {
    auto f = Try(Open(path, OpenFlags::Write | OpenFlags::Create));
    return f.write(data);
}

auto File::read(std::span<char> into) noexcept -> Result<usz> {
    if (not (open_flags & OpenFlags::Read)) return Error("File is not open for reading");
    return FileImpl::read(into);
}

void File::rewind() noexcept {
    FileImpl::rewind();
}

auto File::size() noexcept -> usz {
    return FileImpl::size();
}

auto File::write(std::span<const char> data) noexcept -> Result<> {
    if (not (open_flags & OpenFlags::Write)) return Error("File is not open for writing");
    return FileImpl::write(data);
}

auto File::writev(std::span<const std::span<const char>> data) noexcept -> Result<> {
    if (not (open_flags & OpenFlags::Write)) return Error("File is not open for writing");
    return FileImpl::writev(data);
}
