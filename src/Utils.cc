#include <base/Base.hh>
#include <base/Text.hh>
#include <source_location>
#include <stdexcept>
#include <base/StringUtils.hh>

auto base::utils::Escape(
    std::string_view str,
    bool escape_double_quotes,
    bool escape_per_cent_signs
) -> std::string {
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
            case '%':
                if (escape_per_cent_signs) s += "%%";
                else s += c;
                break;
            default:
                if (text::IsPrint(c)) s += c;
                else s += std::format("\\x{:02x}", static_cast<u8>(c));
        }
    }
    return s;
}

auto base::utils::HumanReadable(u64 value) -> std::string {
    if (value < u64(1) << 10) return std::format("{}", value);
    if (value < u64(1) << 20) return std::format("{}K", value / (u64(1) << 10));
    if (value < u64(1) << 30) return std::format("{}M", value / (u64(1) << 20));
    if (value < u64(1) << 40) return std::format("{}G", value / (u64(1) << 30));
    if (value < u64(1) << 50) return std::format("{}T", value / (u64(1) << 40));
    if (value < u64(1) << 60) return std::format("{}P", value / (u64(1) << 50));
    return std::format("{}E", value / (u64(1) << 60));
}

auto base::utils::HumanReadable(i64 value) -> std::string {
    if (value < 0) return "-" + HumanReadable(static_cast<u64>(-value));
    return HumanReadable(static_cast<u64>(value));
}

auto base::utils::Indent(std::string_view str, u32 width) -> std::string {
    std::string out;
    std::string id(width, ' ');
    bool first = true;
    for (auto l : stream(str).lines()) {
        if (not first) out += "\n";
        else first = false;
        if (l.empty()) continue;
        out += id;
        out += l.text();
    }
    return out;
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
