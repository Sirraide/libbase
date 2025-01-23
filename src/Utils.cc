#include <base/Base.hh>
#include <base/Text.hh>
#include <source_location>
#include <stdexcept>
#include <base/StringUtils.hh>

auto base::utils::Escape(std::string_view str, bool escape_double_quotes) -> std::string {
    std::string s;
    for (auto c : str) {
        switch (c) {
            case '\n': s += "\\n"; break;
            case '\r': s += "\\r"; break;
            case '\t': s += "\\t"; break;
            case '\v': s += "\\v"; break;
            case '\f': s += "\\f"; break;
            case '\a': s += "\\a"; break;
            case '\b': s += "\\b"; break;
            case '\\': s += "\\\\"; break;
            case '\0': s += "\\0"; break;
            case '\033': s += "\\e"; break;
            case '"':
                if (escape_double_quotes) s += "\\\"";
                else s += c;
            break;
            default:
                if (text::IsPrint(c)) s += c;
                else s += std::format("\\x{:02x}", static_cast<u8>(c));
        }
    }
    return s;
}

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
