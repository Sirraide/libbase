#include <base/Base.hh>
#include <charconv>
#include <concepts>
#include <string>
#include <system_error>

using namespace base;

template <typename Ty>
concept Numeric = std::integral<Ty> or std::floating_point<Ty>;

template <Numeric Number>
[[nodiscard]] auto ParseImpl(std::string_view sv) noexcept -> Result<Number> {
    Number n;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), n);

    if (ec != std::errc()) return Error(
        "Failed to parse number from string '{}': {}",
        sv,
        std::make_error_code(ec).message()
    );

    if (ptr != sv.data() + sv.size()) return Error(
        "Failed to parse integer: Trailing junk at end of string '{}'",
        sv
    );

    return n;
}

template <>
auto base::Parse<bool>(std::string_view sv) noexcept -> Result<bool> {
    if (sv == "true") return true;
    if (sv == "false") return false;
    return Error("Expected 'true' or 'false', was '{}'", sv);
}

template <> auto base::Parse<i8>(std::string_view sv) noexcept -> Result<i8> { return ParseImpl<i8>(sv); }
template <> auto base::Parse<i16>(std::string_view sv) noexcept -> Result<i16> { return ParseImpl<i16>(sv); }
template <> auto base::Parse<i32>(std::string_view sv) noexcept -> Result<i32> { return ParseImpl<i32>(sv); }
template <> auto base::Parse<i64>(std::string_view sv) noexcept -> Result<i64> { return ParseImpl<i64>(sv); }
template <> auto base::Parse<u8>(std::string_view sv) noexcept -> Result<u8> { return ParseImpl<u8>(sv); }
template <> auto base::Parse<u16>(std::string_view sv) noexcept -> Result<u16> { return ParseImpl<u16>(sv); }
template <> auto base::Parse<u32>(std::string_view sv) noexcept -> Result<u32> { return ParseImpl<u32>(sv); }
template <> auto base::Parse<u64>(std::string_view sv) noexcept -> Result<u64> { return ParseImpl<u64>(sv); }
template <> auto base::Parse<f32>(std::string_view sv) noexcept -> Result<f32> { return ParseImpl<f32>(sv); }
template <> auto base::Parse<f64>(std::string_view sv) noexcept -> Result<f64> { return ParseImpl<f64>(sv); }
