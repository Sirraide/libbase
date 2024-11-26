#ifndef LIBBASE_ASSERT_HH
#define LIBBASE_ASSERT_HH

#include <libassert/assert.hpp>

// Some IDEs don’t know about __builtin_expect_with_probability, for some reason.
#if !__has_builtin(__builtin_expect_with_probability)
#    define __builtin_expect_with_probability(x, y, z) __builtin_expect(x, y)
#endif

#ifdef LIBBASE_IS_BUILDING_TESTS
#    define Assert(cond, ...)                                                   \
        do {                                                                    \
            if (not(cond)) {                                                    \
                throw std::runtime_error(__VA_OPT__(std::format(__VA_ARGS__))); \
            }                                                                   \
        } while (false)
#else
#    define Assert(cond, ...) LIBASSERT_ASSERT(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#endif

#define DebugAssert(cond, ...) LIBASSERT_DEBUG_ASSERT(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#define Fatal(...)             LIBASSERT_PANIC(__VA_OPT__(std::format(__VA_ARGS__)))
#define Unreachable(...)       LIBASSERT_UNREACHABLE(__VA_OPT__(std::format(__VA_ARGS__)))
#define Todo(...)              Unreachable("TODO: " __VA_OPT__(": " __VA_ARGS__))

#endif // LIBBASE_ASSERT_HH
