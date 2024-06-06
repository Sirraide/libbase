#ifndef LIBBASE_BASE_HH_
#define LIBBASE_BASE_HH_

#include <base/Assert.hh>
#include <base/Result.hh>
#include <base/Types.hh>
#include <print>
#include <string>
#include <vector>

namespace base {
using namespace std::literals;
namespace rgs = std::ranges;
namespace vws = std::ranges::views;

/// Convert a value of enumeration type to its underlying type.
template <typename Ty>
requires std::is_enum_v<Ty>
[[nodiscard]] auto operator+(Ty ty) -> std::underlying_type_t<Ty> {
    return std::to_underlying(ty);
}
} // namespace base

#endif // LIBBASE_BASE_HH_
