#ifndef LIBBASE_STRING_UTILS_HH
#define LIBBASE_STRING_UTILS_HH

#include <base/Utils.hh>
#include <base/Stream.hh>

namespace base::utils {
/// Escape non-printable characters in a string.
auto Escape(std::string_view str, bool escape_double_quotes) -> std::string;

/// Escape elements in a range that need escaping.
template <typename Range>
auto escaped(Range&& r, bool escape_double_quotes) {
    return vws::transform(std::forward<Range>(r), [escape_double_quotes](auto&& el) {
        return Escape(el, escape_double_quotes);
    });
}

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
    std::format_string<decltype(
        std::invoke(
            std::declval<Proj>(),
            *rgs::begin(range)
        )
    )> fmt = "{}",
    Proj proj = {}
) {
    std::string result;
    auto begin = rgs::begin(range);
    auto end = rgs::end(range);
    for (auto it = begin; it != end; ++it) {
        if (it != begin) result += sep;
        result += std::format(fmt, std::invoke(proj, *it));
    }
    return result;
}

/// Surround each element of a range (that contains whitespace) with quotes.
template <typename Range>
auto quoted(Range&& r, bool quote_always = false) {
    return vws::transform(std::forward<Range>(r), [quote_always]<typename T>(T&& el) {
        if (quote_always or stream{std::string_view{el}}.contains_any(stream::whitespace())) return std::format("\"{}\"", el);
        return std::string{std::forward<T>(el)};
    });
}

/// Escape and quote elements in a range.
template <typename Range>
auto quote_escaped(Range&& r, bool quote_always = false) {
    return quoted(escaped(std::forward<Range>(r), true), quote_always);
}

/// Replace all occurrences of `from` with `to` in `str`.
void ReplaceAll(std::string& str, std::string_view from, std::string_view to);

} // namespace base::utils

#endif //LIBBASE_STRING_UTILS_HH
