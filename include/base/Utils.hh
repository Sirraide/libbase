#ifndef LIBBASE_UTILS_HH_
#define LIBBASE_UTILS_HH_

namespace base::utils {
template <typename... Types>
struct Overloaded : Types... { using Types::operator()...; };

template <typename... Types>
Overloaded(Types...) -> Overloaded<Types...>;
} // namespace base::utils

#endif // LIBBASE_UTILS_HH_
