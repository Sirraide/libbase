#include <base/Utils.hh>
#include <format>
#include <stdexcept>

void base::utils::ThrowOrAbort(std::string message, std::source_location loc) {
    auto m = std::format("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
#if __EXCEPTIONS
    throw std::runtime_error(m);
#else
    std::println(stderr, "{}", m);
    std::abort();
#endif
}