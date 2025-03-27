#ifndef LIBBASE_ASSERT_HH
#define LIBBASE_ASSERT_HH

#include <base/Macros.hh>
#include <format>

// Assert() needs to throw in test mode so we can test the assertions.
#if defined LIBBASE_IS_BUILDING_TESTS && !__cpp_exceptions
#    error Cannot build tests with exceptions disabled.
#endif

// ====================================================================
//  With libassert
// ====================================================================
#ifdef LIBBASE_USE_LIBASSERT
#    include <libassert/assert.hpp>

// Some IDEs don’t know about __builtin_expect_with_probability, for some reason.
#    if !__has_builtin(__builtin_expect_with_probability)
#        define __builtin_expect_with_probability(x, y, z) __builtin_expect(x, y)
#    endif

#    define LIBBASE_ASSERT_IMPL(cond, ...)       LIBASSERT_ASSERT(cond __VA_OPT__(, __VA_ARGS__))
#    define LIBBASE_DEBUG_ASSERT_IMPL(cond, ...) LIBASSERT_DEBUG_ASSERT(cond __VA_OPT__(, __VA_ARGS__))
#    define LIBBASE_FATAL_IMPL(...)              LIBASSERT_PANIC(__VA_ARGS__)
#    define LIBBASE_UNREACHABLE_IMPL(...)        LIBASSERT_UNREACHABLE(__VA_ARGS__)

// ====================================================================
//  Without libassert
// ====================================================================
#else
#    include <source_location>
#    include <string_view>

#    define LIBBASE_DO_ASSERT_IMPL(kind, cond, ...)                                                        \
        do {                                                                                               \
            if (not(cond)) ::base::detail::AssertFail(kind, LIBBASE_STR(cond) __VA_OPT__(, ) __VA_ARGS__); \
        } while (false)

#    define LIBBASE_ASSERT_IMPL(cond, ...)       LIBBASE_DO_ASSERT_IMPL("Assertion Failed", cond __VA_OPT__(, __VA_ARGS__))
#    define LIBBASE_DEBUG_ASSERT_IMPL(cond, ...) LIBBASE_DO_ASSERT_IMPL("Debug Assertion Failed", cond __VA_OPT__(, __VA_ARGS__))
#    define LIBBASE_FATAL_IMPL(...)              ::base::detail::AssertFail("Fatal Error" __VA_OPT__(, __VA_ARGS__))
#    define LIBBASE_UNREACHABLE_IMPL(...)        ::base::detail::AssertFail("Unreachable" __VA_OPT__(, __VA_ARGS__))

namespace base::detail {
[[noreturn]] void AssertFail(
    std::string_view kind,
    std::string_view cond = "",
    std::string_view msg = "",
    std::source_location where = std::source_location::current()
);
}
#endif // LIBBASE_USE_LIBASSERT

// ====================================================================
//  API
// ====================================================================
#ifdef LIBBASE_IS_BUILDING_TESTS
#    define Assert(cond, ...)                                                   \
        do {                                                                    \
            if (not(cond)) {                                                    \
                throw std::runtime_error(__VA_OPT__(std::format(__VA_ARGS__))); \
            }                                                                   \
        } while (false)
#    define LIBBASE_NOEXCEPT_UNLESS_TESTING
#else
#    define Assert(cond, ...)               LIBBASE_ASSERT_IMPL(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#    define LIBBASE_NOEXCEPT_UNLESS_TESTING noexcept(true)
#endif

#define DebugAssert(cond, ...) LIBBASE_DEBUG_ASSERT_IMPL(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#define Fatal(...)             LIBBASE_FATAL_IMPL(__VA_OPT__(std::format(__VA_ARGS__)))
#define Unreachable(...)       LIBBASE_UNREACHABLE_IMPL(__VA_OPT__(std::format(__VA_ARGS__)))
#define Todo(...)              Unreachable("TODO: " __VA_OPT__(": " __VA_ARGS__))

#endif // LIBBASE_ASSERT_HH
