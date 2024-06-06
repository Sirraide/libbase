#ifndef LIBBASE_UTILS_HH_
#define LIBBASE_UTILS_HH_

#include <source_location>
#include <string>

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
} // namespace base::utils

#endif // LIBBASE_UTILS_HH_
