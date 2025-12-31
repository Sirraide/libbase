#ifndef LIBBASE_TYPES_HH
#define LIBBASE_TYPES_HH

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <source_location>
#include <type_traits>

#ifdef __SIZEOF_INT128__
#    define LIBBASE_I128_AVAILABLE
#endif

namespace base {
namespace rgs = std::ranges;
namespace vws = std::ranges::views;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usz = std::size_t;
using uptr = std::uintptr_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isz = std::make_signed_t<std::size_t>;
using iptr = std::intptr_t;

using f32 = float;
using f64 = double;

#ifdef LIBBASE_I128_AVAILABLE
using i128 = __int128;
using u128 = unsigned __int128;
#endif

namespace utils {
/// Used instead of 'Assert()' in some places so we can catch the
/// exception in unit tests.
[[noreturn]] void ThrowOrAbort(
    std::string_view message,
    std::source_location loc = std::source_location::current()
);
}
} // namespace base

#endif // LIBBASE_TYPES_HH
