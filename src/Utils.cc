#include <base/Base.hh>
#include <base/Text.hh>
#include <source_location>
#include <stdexcept>
#include <base/StringUtils.hh>

#if !defined(LIBBASE_USE_LIBASSERT) && __has_include(<unistd.h>)
#   include <unistd.h>
#endif

auto base::utils::Escape(
    str s,
    bool escape_double_quotes,
    bool escape_per_cent_signs
) -> std::string {
    std::string out;
    for (auto c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\v': out += "\\v"; break;
            case '\f': out += "\\f"; break;
            case '\a': out += "\\a"; break;
            case '\b': out += "\\b"; break;
            case '\\': out += "\\\\"; break;
            case '\0': out += "\\0"; break;
            case '\033': out += "\\e"; break;
            case '"':
                if (escape_double_quotes) out += "\\\"";
                else out += c;
                break;
            case '%':
                if (escape_per_cent_signs) out += "%%";
                else out += c;
                break;
            default:
                if (text::IsPrint(c)) out += c;
                else out += std::format("\\x{:02x}", static_cast<u8>(c));
        }
    }
    return out;
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

auto base::utils::Indent(str s, u32 width) -> std::string {
    std::string out;
    std::string id(width, ' ');
    bool first = true;
    for (auto l : s.lines()) {
        if (not first) out += "\n";
        else first = false;
        if (l.empty()) continue;
        out += id;
        out += l;
    }
    return out;
}

static bool StreamSupportsColours([[maybe_unused]] int fd) {
#ifdef LIBBASE_USE_LIBASSERT
    // If libassert is available, use it to figure this out.
    return libassert::isatty(fd);
#elif __has_include(<unistd.h>)
    // Use isatty() if available.
    isatty(fd);
#else
    // Conservatively assume colours are not available on Windows.
    return false;
#endif
}

bool base::utils::StderrSupportsColours() {
    return StreamSupportsColours(2);
}

bool base::utils::StdoutSupportsColours() {
    return StreamSupportsColours(1);
}

void base::utils::ThrowOrAbort(std::string_view message, std::source_location loc) {
#ifdef __cpp_exceptions
    auto m = std::format("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
    throw std::runtime_error(m);
#else
    Fatal("Error at {}:{}: {}", loc.file_name(), loc.line(), message);
#endif
}

auto base::ByteSpan::str() const -> basic_str<char> {
    return {reinterpret_cast<const char*>(data()), size()};
}

auto base::MutableByteSpan::str() const -> basic_str<char> {
    return {reinterpret_cast<const char*>(data()), size()};
}
