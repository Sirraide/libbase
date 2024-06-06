#ifndef LIBBASE_NUMBERS_HH_
#define LIBBASE_NUMBERS_HH_

#include <base/Result.hh>
#include <base/Types.hh>
#include <string_view>
#include <utility>

#define LIBBASE_DEFINE_FLAG_ENUM(e)                                           \
    constexpr auto operator|(e a, e b) noexcept -> e { return e(+a | +b); }   \
    constexpr auto operator|=(e& a, e b) noexcept -> e& { return a = a | b; } \
    constexpr bool operator&(e a, e b) noexcept { return (+a & +b) != 0; }

namespace base {
/// Convert a value of enumeration type to its underlying type.
template <typename Ty>
requires std::is_enum_v<Ty>
[[nodiscard]] constexpr auto operator+(Ty ty) noexcept -> std::underlying_type_t<Ty> {
    return std::to_underlying(ty);
}

/// Parse a 'Ty' from a string.
template <typename Ty>
auto Parse(std::string_view sv) -> Result<Ty> = delete;

/// Bool parser.
template <> auto Parse<bool>(std::string_view sv) -> Result<bool>;

/// Integer parsers.
template <> auto Parse<i8>(std::string_view sv) -> Result<i8>;
template <> auto Parse<i16>(std::string_view sv) -> Result<i16>;
template <> auto Parse<i32>(std::string_view sv) -> Result<i32>;
template <> auto Parse<i64>(std::string_view sv) -> Result<i64>;
template <> auto Parse<u8>(std::string_view sv) -> Result<u8>;
template <> auto Parse<u16>(std::string_view sv) -> Result<u16>;
template <> auto Parse<u32>(std::string_view sv) -> Result<u32>;
template <> auto Parse<u64>(std::string_view sv) -> Result<u64>;

/// Floating-point parsers.
template <> auto Parse<f32>(std::string_view sv) -> Result<f32>;
template <> auto Parse<f64>(std::string_view sv) -> Result<f64>;
} // namespace base

#endif // LIBBASE_NUMBERS_HH_
