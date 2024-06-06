#ifndef LIBBASE_FS_GENERIC_HH_
#define LIBBASE_FS_GENERIC_HH_

#include <base/detail/FSCommon.hh>
#include <cstdio>
#include <filesystem>
#include <memory>

namespace base {
class FileImpl {
public:
    using Path = std::filesystem::path;
    using PathRef = const std::filesystem::path&;

private:
    std::unique_ptr<FILE, decltype(&std::fclose)> handle{nullptr, std::fclose};

protected:
    explicit FileImpl() = default;

    auto open(PathRef path, OpenFlags flags) noexcept -> Result<>;
    auto read(std::span<std::byte> into) noexcept -> Result<usz>;
    void rewind() noexcept;
    auto size() noexcept -> usz;
    auto write(std::span<const std::byte> data) noexcept -> Result<>;
    auto writev(std::span<const std::span<const std::byte>> data) noexcept -> Result<>;

    static auto Delete(PathRef path, bool recursive) -> Result<>;
};
} // namespace base

#endif // LIBBASE_FS_GENERIC_HH_
