#ifndef LIBBASE_STRING_UTILS_HH
#define LIBBASE_STRING_UTILS_HH

#include <base/Utils.hh>
#include <base/Stream.hh>

namespace base::utils {
/// Escape non-printable and formatting characters in a string.
auto Escape(
    std::string_view str,
    bool escape_double_quotes = false,
    bool escape_per_cent_signs = false
) -> std::string;

/// Escape elements in a range that need escaping.
template <typename Range>
auto escaped(Range&& r, bool escape_double_quotes) {
    return vws::transform(std::forward<Range>(r), [escape_double_quotes](auto&& el) {
        return Escape(el, escape_double_quotes);
    });
}

/// Indent a string.
auto Indent(std::string_view str, u32 width) -> std::string;

/// Join a range of strings.
///
/// \param range The range whose elements should be joined.
/// \param sep The separator to insert between elements.
/// \param fmt The format string to use for each element.
/// \param proj A projection to apply to each element before formatting.
template <typename Range, typename Proj = std::identity>
requires std::is_invocable_v<Proj, rgs::range_value_t<Range>>
std::string join(
    Range&& range,
    std::string_view sep = ", ",
    std::format_string<decltype(std::invoke(
        std::declval<Proj>(),
        std::declval<const rgs::range_value_t<Range>&>()
    ))> fmt = "{}",
    Proj proj = {}
) {
    std::string result;
    bool first = true;
    for (const auto& val : range) {
        if (first) first = false;
        else result += sep;
        result += std::format(fmt, std::invoke(proj, val));
    }
    return result;
}

template <typename Range, typename Proj = std::identity>
requires std::is_invocable_v<Proj, rgs::range_value_t<Range>>
std::string join_as(
    Range&& range,
    Proj proj
) { return utils::join(LIBBASE_FWD(range), ", ", "{}", proj); }

/// Surround each element of a range (that contains whitespace) with quotes.
template <typename Range>
auto quoted(Range&& r, bool quote_always = false) {
    return vws::transform(LIBBASE_FWD(r), [quote_always]<typename T>(T&& el) {
        if (quote_always or stream{std::string_view{el}}.contains_any(stream::whitespace())) return std::format("\"{}\"", el);
        return std::string{std::forward<T>(el)};
    });
}

/// Escape and quote elements in a range.
template <typename Range>
auto quote_escaped(Range&& r, bool quote_always = false) {
    return quoted(escaped(LIBBASE_FWD(r), true), quote_always);
}

/// Replace all occurrences of `from` with `to` in `str`.
[[deprecated("Use `stream(str).replace(from, to)` instead")]]
inline void ReplaceAll(std::string& str, std::string_view from, std::string_view to) {
    str = stream(str).replace(from, to);
}
} // namespace base::utils

#endif //LIBBASE_STRING_UTILS_HH
