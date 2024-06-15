#ifndef LIBBASE_UTILS_HH_
#define LIBBASE_UTILS_HH_

#include <source_location>
#include <string>
#include <variant>

namespace base::utils {
template <typename... Types>
struct Overloaded : Types... {
    using Types::operator()...;
};

template <typename... Types>
Overloaded(Types...) -> Overloaded<Types...>;

[[noreturn]] void ThrowOrAbort(
    std::string message,
    std::source_location loc = std::source_location::current()
);

/// std::visit, but with a better order of arguments.
template <typename Variant, typename Visitor>
constexpr decltype(auto) Visit(Visitor&& visitor, Variant&& variant) {
    return std::visit(std::forward<Variant>(variant), std::forward<Visitor>(visitor));
}
} // namespace base::utils

#endif // LIBBASE_UTILS_HH_
