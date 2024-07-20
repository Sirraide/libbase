module;

#include <print>
#include <stdexcept>
#include <source_location>

module base;

void base::utils::ReplaceAll(
    std::string& str,
    std::string_view from,
    std::string_view to
) {
    if (from.empty()) return;
    for (usz i = 0; i = str.find(from, i), i != std::string::npos; i += to.size())
        str.replace(i, from.size(), to);
}


void base::utils::ThrowOrAbort(const std::string& message, std::source_location loc) {
    auto m = std::format("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
#if __EXCEPTIONS
    throw std::runtime_error(m);
#else
    std::println(stderr, "{}", m);
    std::abort();
#endif
}
