#ifndef LIBBASE_NUMERIC_HH
#define LIBBASE_NUMERIC_HH

#include <base/Result.hh>
#include <base/Types.hh>
#include <string_view>
#include <utility>
#include <bit>

namespace base {
/// Convert a value of enumeration type to its underlying type.
template <typename Ty>
requires std::is_enum_v<Ty>
[[nodiscard]] constexpr auto operator+(Ty ty) noexcept -> std::underlying_type_t<Ty> {
    return std::to_underlying(ty);
}

/// Compute the floor base-2 logarithm of an integer >=1. If given 0, returns -1.
template <std::integral T>
[[nodiscard]] constexpr auto Log2Floor(T n) noexcept -> u32 {
    using U = std::make_unsigned_t<T>;
    return u32(std::numeric_limits<U>::digits - 1 - std::countl_zero(U(n)));
}

/// Parse a 'Ty' from a string.
template <typename Ty>
auto Parse(std::string_view sv) noexcept -> Result<Ty> = delete;

/// Bool parser.
template <> auto Parse<bool>(std::string_view sv) noexcept -> Result<bool>;

/// Integer parsers.
template <> auto Parse<i8>(std::string_view sv) noexcept -> Result<i8>;
template <> auto Parse<i16>(std::string_view sv) noexcept -> Result<i16>;
template <> auto Parse<i32>(std::string_view sv) noexcept -> Result<i32>;
template <> auto Parse<i64>(std::string_view sv) noexcept -> Result<i64>;
template <> auto Parse<u8>(std::string_view sv) noexcept -> Result<u8>;
template <> auto Parse<u16>(std::string_view sv) noexcept -> Result<u16>;
template <> auto Parse<u32>(std::string_view sv) noexcept -> Result<u32>;
template <> auto Parse<u64>(std::string_view sv) noexcept -> Result<u64>;

/// Floating-point parsers.
template <> auto Parse<f32>(std::string_view sv) noexcept -> Result<f32>;
template <> auto Parse<f64>(std::string_view sv) noexcept -> Result<f64>;
} // namespace base

#endif // LIBBASE_NUMERIC_HH
