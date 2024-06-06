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
    Path abs_path; // Hack so we can truncate the file.

protected:
    explicit FileImpl() = default;

    auto open(PathRef path, OpenFlags flags) noexcept -> Result<>;
    auto read(OutputView into) noexcept -> Result<usz>;
    void rewind() noexcept;
    auto size() noexcept -> usz;
    auto resize(usz size) noexcept -> Result<>;
    auto write(InputView data) noexcept -> Result<>;
    auto writev(InputVector data) noexcept -> Result<>;

    static auto Delete(PathRef path, bool recursive) -> Result<bool>;
    static auto Exists(PathRef path) noexcept -> bool;
};
} // namespace base

#endif // LIBBASE_FS_GENERIC_HH_
