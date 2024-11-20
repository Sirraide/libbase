#include <base/Base.hh>
#include <print>
#include <source_location>
#include <stdexcept>

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
#ifdef __cpp_exceptions
    auto m = std::format("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
    throw std::runtime_error(m);
#else
    Fatal("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
#endif
}
