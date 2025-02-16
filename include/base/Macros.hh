#ifndef LIBBASE_MACROS_HH
#define LIBBASE_MACROS_HH

#include <expected>
#include <type_traits>
#include <utility>

#define LIBBASE_STR_(X) #X
#define LIBBASE_STR(X)  LIBBASE_STR_(X)

#define LIBBASE_CAT_(X, Y) X##Y
#define LIBBASE_CAT(X, Y)  LIBBASE_CAT_(X, Y)

#define LIBBASE_IMMOVABLE(cls)           \
    cls(const cls&) = delete;            \
    cls& operator=(const cls&) = delete; \
    cls(cls&&) = delete;                 \
    cls& operator=(cls&&) = delete

#define LIBBASE_NO_COPY(cls)             \
public:                                  \
    cls(const cls&) = delete;            \
    cls& operator=(const cls&) = delete; \
private:

#define LIBBASE_MOVE_ONLY(cls)                \
public:                                       \
    cls(cls&&) noexcept = default;            \
    cls& operator=(cls&&) noexcept = default; \
    cls(const cls&) = delete;                 \
    cls& operator=(const cls&) = delete;      \
private:

#ifndef NDEBUG
#    define LIBBASE_DEBUG(...) __VA_ARGS__
#    define LIBBASE_RELEASE(...)
#    define LIBBASE_IF_DEBUG(debug, release) debug
#else
#    define LIBBASE_DEBUG(...)
#    define LIBBASE_RELEASE(...) __VA_ARGS__
#    define LIBBASE_IF_DEBUG(debug, release) release
#endif

#define LIBBASE_LPAREN_ (
#define LIBBASE_RPAREN_ )

#define LIBBASE_DEFINE_FLAG_ENUM(e)                                           \
    constexpr auto operator|(e a, e b) noexcept -> e { return e(+a | +b); }   \
    constexpr auto operator|=(e& a, e b) noexcept -> e& { return a = a | b; } \
    constexpr bool operator&(e a, e b) noexcept { return (+a & +b) != 0; }

#define LIBBASE_DECLARE_HIDDEN_IMPL(cls, ...) \
private:                                      \
    struct Impl;                              \
    std::unique_ptr<Impl> impl;               \
    cls(__VA_ARGS__);                         \
public:                                       \
    ~cls();                                   \
    cls(const cls&) = delete;                 \
    cls& operator=(const cls&) = delete;      \
    cls(cls&&) noexcept;                      \
    cls& operator=(cls&&) noexcept;           \
private:

#define LIBBASE_DEFINE_HIDDEN_IMPL(cls)                                     \
    cls::cls() = default;                                                   \
    cls::~cls() = default;                                                  \
    cls::cls(cls&& other) noexcept : impl(std::exchange(other.impl, {})) {} \
    cls& cls::operator=(cls&& other) noexcept {                             \
        if (this == std::addressof(other)) return *this;                    \
        impl = std::exchange(other.impl, {});                               \
        return *this;                                                       \
    }

#define LIBBASE_DECLARE_HIDDEN_IMPL_IMMOVABLE(cls, ...) \
private:                                                \
    struct Impl;                                        \
    std::unique_ptr<Impl> impl;                         \
    cls(__VA_ARGS__);                                   \
public:                                                 \
    ~cls();                                             \
    LIBBASE_IMMOVABLE(cls);                             \
private:

#define LIBBASE_DEFINE_HIDDEN_IMPL_IMMOVABLE(cls) \
    cls::cls() = default;                         \
    cls::~cls() = default;

#define LIBBASE_VA_FIRST(first, ...) first

#define LIBBASE_FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

/// Macro that propagates errors up the call stack.
///
/// The second optional argument to the macro, if present, should be an
/// expression that evaluates to a string that will be propagated up the
/// call stack as the error; the original error is in scope as `$`.
///
/// Example usage: Given
///
///     auto foo() -> Result<Bar> { ... }
///
/// we can write
///
///     Bar res = Try(foo());
///     Bar res = Try(foo(), std::format("Failed to do X: {}", $));
///
/// to invoke `foo` and propagate the error up the call stack, if there
/// is one; this way, we don’t have to actually write any verbose error
/// handling code.
///
/// (Yes, I know this macro is an abomination, but this is what happens
/// if you don’t have access to this as a language feature...)
// clang-format off
#define Try(x, ...) ({                                                       \
    auto _res = x;                                                           \
    if (not _res) {                                                          \
        return std::unexpected(                                              \
            __VA_OPT__(                                                      \
                [&]([[maybe_unused]] std::string $) {                        \
                    return __VA_ARGS__;                                      \
                }                                                            \
            ) __VA_OPT__(LIBBASE_LPAREN_)                                    \
                std::move(_res.error())                                      \
            __VA_OPT__(LIBBASE_RPAREN_)                                      \
        );                                                                   \
    }                                                                        \
    using NonRef = std::remove_reference_t<decltype(_res._unsafe_unwrap())>; \
    static_cast<std::add_rvalue_reference_t<NonRef>>(_res._unsafe_unwrap()); \
})
// clang-format on

#define defer   ::base::detail::DeferImpl _ = [&]
#define tempset auto _ = ::base::detail::Tempset{}->*

namespace base::detail {
template <typename Callable>
class DeferImpl {
    Callable c;

public:
    DeferImpl(Callable c) : c(std::move(c)) {}
    ~DeferImpl() { c(); }
};

template <typename T>
class TempsetStage2 {
    T& lvalue;
    T old_value;

public:
    TempsetStage2(T& lvalue, auto&& value)
        : lvalue(lvalue),
          old_value(std::exchange(lvalue, LIBBASE_FWD(value))) {}
    ~TempsetStage2() { lvalue = std::move(old_value); }
};

#define DEFINE_ASSIGN_OP(op)                                        \
    auto operator op##=(auto&& value) {                             \
        return TempsetStage2{lvalue, lvalue op LIBBASE_FWD(value)}; \
    }

template <typename T>
struct TempsetStage1 {
    T& lvalue;
    auto operator=(auto&& value) {
        return TempsetStage2{lvalue, LIBBASE_FWD(value)};
    }

    DEFINE_ASSIGN_OP(|);
    DEFINE_ASSIGN_OP(&);
    DEFINE_ASSIGN_OP(^);
    DEFINE_ASSIGN_OP(<<);
    DEFINE_ASSIGN_OP(>>);
    DEFINE_ASSIGN_OP(+);
    DEFINE_ASSIGN_OP(-);
    DEFINE_ASSIGN_OP(*);
    DEFINE_ASSIGN_OP(/);
    DEFINE_ASSIGN_OP(%);
};

#undef DEFINE_ASSIGN_OP

struct Tempset {
    auto operator->*(auto& lvalue) {
        return TempsetStage1{lvalue};
    }
};
} // namespace base::detail

#endif // LIBBASE_MACROS_HH
