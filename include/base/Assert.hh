#ifndef LIBBASE_ASSERT_HH_
#define LIBBASE_ASSERT_HH_

#include <libassert/assert.hpp>

// Some IDEs don’t know about __builtin_expect_with_probability, for some reason.
#if !__has_builtin(__builtin_expect_with_probability)
#    define __builtin_expect_with_probability(x, y, z) __builtin_expect(x, y)
#endif

#define Assert(cond, ...)      LIBASSERT_ASSERT(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#define DebugAssert(cond, ...) LIBASSERT_DEBUG_ASSERT(AK_DebugAssert, cond __VA_OPT__(, __VA_ARGS__))
#define Unreachable(...)       LIBASSERT_UNREACHABLE(__VA_OPT__(std::format(__VA_ARGS__)))
#define Todo(...)              Unreachable("Todo" __VA_OPT__(": " __VA_ARGS__))

#endif // LIBBASE_ASSERT_HH_
