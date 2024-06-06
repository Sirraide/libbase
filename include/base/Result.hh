#ifndef LIBBASE_RESULT_HH_
#define LIBBASE_RESULT_HH_

#include <expected>
#include <format>
#include <string>
#include <type_traits>

#define LIBBASE_LPAREN_ (
#define LIBBASE_RPAREN_ )

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
#define Try(x, ...) ({                                       \
    auto _res = x;                                           \
    if (not _res) {                                          \
        return std::unexpected(                              \
            __VA_OPT__(                                      \
                [&]([[maybe_unused]] std::string $) {        \
                    return __VA_ARGS__;                      \
                }                                            \
            ) __VA_OPT__(LIBBASE_LPAREN_)                    \
                std::move(_res.error())                      \
            __VA_OPT__(LIBBASE_RPAREN_)                      \
        );                                                   \
    }                                                        \
    using NonRef = std::remove_reference_t<decltype(*_res)>; \
    static_cast<std::add_rvalue_reference_t<NonRef>>(*_res); \
})
// clang-format on

namespace base::detail {
template <typename Ty>
requires (not std::is_reference_v<Ty>)
class ReferenceWrapper {
    Ty* ptr;
public:
    ReferenceWrapper(Ty& ref) : ptr(&ref) {}
    operator Ty&() const { return *ptr; }
    auto operator&() const -> Ty* { return ptr; }
    auto operator->() const -> Ty* { return ptr; }
};

template <typename Ty>
concept Reference = std::is_reference_v<Ty>;

template <typename Ty>
concept NotReference = not Reference<Ty>;

template <typename Ty>
struct ResultImpl;

template <Reference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<ReferenceWrapper<std::remove_reference_t<Ty>>, std::string>;
};

template <NotReference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<Ty, std::string>;
};

template <typename Ty>
struct ResultImpl<std::reference_wrapper<Ty>> {
    using type = typename ResultImpl<Ty&>::type;
    static_assert(false, "Use Result<T&> instead of Result<reference_wrapper<T>>");
};

template <typename Ty>
struct ResultImpl<ReferenceWrapper<Ty>> {
    using type = typename ResultImpl<Ty&>::type;
    static_assert(false, "Use Result<T&> instead of Result<ReferenceWrapper<T>>");
};
} // namespace base::detail

namespace base {
/// A result type that stores either a value or an error message.
///
/// You can create a new result using the 'Error()' function and
/// unwrap results using 'Try()' (or check them manually if you
/// want to handle the error).
///
/// Result<T&> is valid and is handled correctly.
template <typename T = void>
struct [[nodiscard]] Result : detail::ResultImpl<T>::type {
    using detail::ResultImpl<T>::type::type;
};

/// Create an error message.
template <typename... Args>
[[nodiscard]] auto Error(
    std::format_string<Args...> fmt,
    Args&&... args
) -> std::unexpected<std::string> {
    return std::unexpected(std::format(fmt, std::forward<Args>(args)...));
}

} // namespace base

#endif // LIBBASE_RESULT_HH_
