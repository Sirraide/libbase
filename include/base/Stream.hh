#ifndef LIBBASE_STREAM_HH
#define LIBBASE_STREAM_HH

#include <algorithm>
#include <base/Assert.hh>
#include <base/Macros.hh>
#include <base/Regex.hh>
#include <base/Utils.hh>
#include <cstddef>
#include <format>
#include <optional>
#include <ranges>
#include <source_location>
#include <string_view>
#include <utility>

#define LIBBASE_STREAM_STRING_LITERAL(lit) [] {             \
    if constexpr (std::is_same_v<char_type, char>)          \
        return lit;                                         \
    else if constexpr (std::is_same_v<char_type, wchar_t>)  \
        return L"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char8_t>)  \
        return u8"" lit;                                    \
    else if constexpr (std::is_same_v<char_type, char16_t>) \
        return u"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char32_t>) \
        return U"" lit;                                     \
    else                                                    \
        static_assert(false, "Unsupported character type"); \
}()

namespace base {
/// \brief A stream of characters.
///
/// This is a non-owning wrapper around a blob of text intended for simple
/// parsing tasks. Streams are cheap to copy and should be passed by value.
template <typename CharType>
class basic_stream {
public:
    using char_type = CharType;
    using char_opt = std::optional<char_type>;
    using text_type = std::basic_string_view<char_type>;
    using size_type = std::size_t;
    using string_type = std::basic_string<char_type>;

private:
    text_type _m_text;

    static consteval auto _s_default_line_separator() -> text_type {
#ifndef _WIN32
        return LIBBASE_STREAM_STRING_LITERAL("\n");
#else
        return LIBBASE_STREAM_STRING_LITERAL("\r\n");
#endif
    }

public:
    /// Construct an empty stream.
    constexpr basic_stream() = default;

    /// Construct a new stream from text.
    constexpr basic_stream(text_type text) noexcept : _m_text(text) {}

    /// Construct a new stream from text.
    constexpr basic_stream(const string_type& text) noexcept : _m_text(text) {}

    // Prevent construction from temporary.
    constexpr basic_stream(const string_type&& text) = delete;

    /// Construct a new stream from characters.
    constexpr basic_stream(const char_type* chars, size_type size) noexcept
        : _m_text(chars, size) {}

    /// Construct a new stream from a string literal.
    template <usz n>
    constexpr basic_stream(const char_type (&chars)[n]) noexcept
        : _m_text(chars, n - 1) {}

    /// \return The last character of the stream, or an empty optional
    ///         if the stream is empty.
    [[nodiscard]] constexpr auto
    back() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.back();
    }

    /// \return The data pointer as a \c const \c char*.
    [[nodiscard]] constexpr auto
    char_ptr() const noexcept -> const char* {
        // Hack to avoid reinterpret_cast at compile time.
        if constexpr (std::same_as<char_type, char>) return _m_text.data();
        else return reinterpret_cast<const char*>(_m_text.data());
    }

    /// Iterate over the text in chunks.
    [[nodiscard]] constexpr auto
    chunks(size_type size) const noexcept {
        return _m_text | std::views::chunk(size) | std::views::transform([](auto r) { return basic_stream(text_type(r)); });
    }

    /// Skip a character.
    ///
    /// If the first character of the stream is the given character,
    /// remove it from the stream. Otherwise, do nothing.
    ///
    /// \param c The character to skip.
    /// \return True if the character was skipped.
    [[nodiscard]] constexpr auto
    consume(char_type c) noexcept -> bool {
        if (not starts_with(c)) return false;
        drop();
        return true;
    }

    /// Skip a list of characters.
    ///
    /// If the stream starts with the given list of characters, in that
    /// order, they are removed from the stream. Otherwise, do nothing.
    ///
    /// \param chars The characters to skip.
    /// \return True if the characters were skipped.
    [[nodiscard]] constexpr auto
    consume(text_type chars) noexcept -> bool {
        if (not starts_with(chars)) return false;
        drop(chars.size());
        return true;
    }

    /// Skip any of a set of characters.
    ///
    /// \param chars The characters to skip.
    /// \return True if any of the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_any(text_type chars) noexcept -> bool {
        if (not starts_with_any(chars)) return false;
        drop();
        return true;
    }

    /// Skip a character.
    ///
    /// If the first character of the stream is the given character,
    /// remove it from the stream. Otherwise, do nothing.
    ///
    /// \param c The character to skip.
    /// \return True if the character was skipped.
    [[nodiscard]] constexpr auto
    consume_back(char_type c) noexcept -> bool {
        if (not ends_with(c)) return false;
        drop_back();
        return true;
    }

    /// Skip a list of characters.
    ///
    /// If the stream starts with the given list of characters, in that
    /// order, they are removed from the stream. Otherwise, do nothing.
    ///
    /// \param chars The characters to skip.
    /// \return True if the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_back(text_type chars) noexcept -> bool {
        if (not ends_with(chars)) return false;
        drop_back(chars.size());
        return true;
    }

    /// Skip any of a set of characters.
    ///
    /// \param chars The characters to skip.
    /// \return True if any of the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_back_any(text_type chars) noexcept -> bool {
        if (not ends_with_any(chars)) return false;
        drop_back();
        return true;
    }

    /// Check if the stream contains a character.
    [[nodiscard]] constexpr auto
    contains(char_type c) noexcept -> bool {
        auto pos = _m_text.find(c);
        return pos != text_type::npos;
    }

    /// Check if the text contains a string.
    [[nodiscard]] constexpr auto
    contains(text_type s) noexcept -> bool {
        auto pos = _m_text.find(s);
        return pos != text_type::npos;
    }

    /// Check if the string contains any of a set of characters.
    [[nodiscard]] constexpr auto
    contains_any(text_type chars) noexcept -> bool {
        auto pos = _m_text.find_first_of(chars);
        return pos != text_type::npos;
    }

    /// Count the number of occurrences of a character.
    [[nodiscard]] constexpr auto
    count(char_type c) const noexcept -> size_type {
        return size_type(std::ranges::count(_m_text, c));
    }

    /// Count the number of occurrences of a substring.
    [[nodiscard]] constexpr auto
    count(text_type s) const noexcept -> size_type {
        size_type count = 0;
        for (
            size_type pos = 0;
            (pos = _m_text.find(s, pos)) != text_type::npos;
            pos += s.size()
        ) ++count;
        return count;
    }

    /// Count the number of any of a set of characters.
    [[nodiscard]] constexpr auto
    count_any(text_type chars) const noexcept -> size_type {
        return size_type(std::ranges::count_if(_m_text, [chars](char_type c) {
            return chars.contains(c);
        }));
    }

    /// \return The data pointer.
    [[nodiscard]] constexpr auto
    data() const noexcept -> const char_type* {
        return _m_text.data();
    }

    /// Discard N characters from the stream.
    ///
    /// If the stream contains fewer than N characters, this clears
    /// the entire stream.
    ///
    /// To skip a certain character if it is present, use \c consume().
    ///
    /// \see consume(char_type)
    ///
    /// \param n The number of characters to drop. If negative, calls
    ///          drop_back() instead.
    ///
    /// \return This.
    template <std::integral size_ty = size_type>
    requires (not std::same_as<std::remove_cvref_t<size_ty>, char_type>)
    constexpr auto
    drop(size_ty n = 1) noexcept -> basic_stream& {
        if constexpr (not std::unsigned_integral<size_ty>) {
            if (n < 0) return drop_back(static_cast<size_type>(-n));
        }

        (void) take(static_cast<size_type>(n));
        return *this;
    }

    /// Discard N characters from the back of the stream.
    ///
    /// If the stream contains fewer than N characters, this clears
    /// the entire stream.
    ///
    /// \param n The number of characters to drop. If negative, calls
    ///          drop() instead.
    ///
    /// \return This.
    template <std::integral size_ty = size_type>
    requires (not std::same_as<std::remove_cvref_t<size_ty>, char_type>)
    constexpr auto
    drop_back(size_ty n = 1) noexcept -> basic_stream& {
        if constexpr (not std::unsigned_integral<size_ty>) {
            if (n < 0) return drop(static_cast<size_type>(-n));
        }

        if (static_cast<size_type>(n) >= _m_text.size()) _m_text = {};
        else _m_text.remove_suffix(static_cast<size_type>(n));
        return *this;
    }

    ///@{
    /// \brief Drop characters from the back stream conditionally.
    ///
    /// Same as \c take_back_until() and friends, but discards the
    /// characters instead and returns the stream.
    ///
    /// \see take_back_until(char_type)
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return This
    constexpr auto
    drop_back_until(char_type c) noexcept -> basic_stream& {
        (void) take_back_until(c);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until(text_type s) noexcept -> basic_stream& {
        (void) take_back_until(s);
        return *this;
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_back_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_back_until(std::move(c));
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_any(text_type chars) noexcept -> basic_stream& {
        (void) take_back_until_any(chars);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_any_or_empty(text_type chars) noexcept -> basic_stream& {
        (void) take_back_until_any_or_empty(chars);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_or_empty(char_type c) noexcept -> basic_stream& {
        (void) take_back_until_or_empty(c);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_or_empty(text_type s) noexcept -> basic_stream& {
        (void) take_back_until_or_empty(s);
        return *this;
    }

    /// \see take_back_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_back_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_back_until_or_empty(std::move(c));
        return *this;
    }
    ///@}

    ///@{
    /// \brief Drop characters from the stream conditionally.
    ///
    /// Same as \c take_until() and friends, but discards the
    /// characters instead and returns the stream.
    ///
    /// \see take_until(char_type)
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return This
    constexpr auto
    drop_until(char_type c) noexcept -> basic_stream& {
        (void) take_until(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until(text_type s) noexcept -> basic_stream& {
        (void) take_until(s);
        return *this;
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_until(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any(text_type chars) noexcept -> basic_stream& {
        (void) take_until_any(chars);
        return *this;
    }

    constexpr auto
    drop_until_any(utils::ConvertibleRange<text_type> auto&& strings) noexcept -> basic_stream {
        (void) take_until_any(LIBBASE_FWD(strings));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any_or_empty(text_type chars) noexcept -> basic_stream& {
        (void) take_until_any_or_empty(chars);
        return *this;
    }

    constexpr auto
    drop_until_any_or_empty(utils::ConvertibleRange<text_type> auto&& strings) noexcept -> basic_stream {
        (void) take_until_any_or_empty(LIBBASE_FWD(strings));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(char_type c) noexcept -> basic_stream& {
        (void) take_until_or_empty(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(text_type s) noexcept -> basic_stream& {
        (void) take_until_or_empty(s);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_until_or_empty(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_ws() noexcept -> basic_stream& {
        (void) take_until_ws();
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while(char_type c) noexcept -> basic_stream& {
        (void) take_while(c);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_while(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_while(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while_any(text_type chars) noexcept -> basic_stream& {
        (void) take_while_any(chars);
        return *this;
    }
    ///@}

    /// Check if this stream is empty.
    [[nodiscard]] constexpr auto
    empty() const noexcept -> bool {
        return _m_text.empty();
    }

    ///@{
    /// \return True if the stream ends with the given character(s).
    [[nodiscard]] constexpr auto
    ends_with(char_type suffix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.back() == suffix;
    }

    /// \see ends_with(char_type) const
    [[nodiscard]] constexpr auto
    ends_with(text_type suffix) const noexcept -> bool {
        return _m_text.ends_with(suffix);
    }
    ///@}

    /// \return True if the stream ends with any of the given characters.
    [[nodiscard]] constexpr auto
    ends_with_any(text_type suffix) const -> bool {
        return not _m_text.empty() and basic_stream{suffix}.contains(_m_text.back());
    }

    /// Extract a series of characters from a stream.
    template <std::same_as<CharType>... Chars>
    [[nodiscard]] constexpr auto
    extract(Chars&... chars) noexcept -> bool {
        if (not has(sizeof...(Chars))) return false;
        auto extracted = take(sizeof...(Chars));
        auto it = extracted.begin();
        ((chars = *it++), ...);
        return true;
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Find the first occurrence of a regular expression in the stream.
    [[nodiscard]] auto
    find(basic_regex<CharType>& regex) const noexcept -> std::optional<regex_match> {
        return regex.find(text());
    }

    /// Find the first occurrence of a regular expression in the stream.
    ///
    /// Prefer the overload that takes a \c basic_regex object as this
    /// will recompile the regular expression every time it is called.
    ///
    /// If the regular expression fails to compile, this returns \c std::nullopt.
    [[nodiscard]] auto find(
        text_type regex_string,
        regex_flags flags = regex_flags::jit
    ) const -> std::optional<regex_match> {
        auto r = basic_regex<CharType>::create(regex_string, flags);
        if (not r) return std::nullopt;
        return r.value().find(text());
    }
#endif

    ///@{
    /// Delete all occurrences of consecutive characters, replacing them
    /// with another string.
    [[nodiscard]] constexpr auto
    fold_any(text_type chars, text_type replacement) const noexcept -> string_type {
        string_type ret;
        basic_stream s{*this};
        while (not s.empty()) {
            ret += s.take_until_any(chars);
            if (not s.take_while_any(chars).empty()) ret += replacement;
        }
        return ret;
    }

    /// Delete all occurrences of consecutive characters, replacing them
    /// with another character.
    [[nodiscard]] constexpr auto
    fold_any(text_type chars, char_type replacement) const noexcept -> string_type {
        return fold_any(chars, text_type{&replacement, 1});
    }
    ///@}

    /// Equivalent to `fold(basic_stream::whitespace())`.
    [[nodiscard]] constexpr auto
    fold_ws(text_type replacement = " ") const noexcept -> string_type {
        return fold_any(whitespace(), replacement);
    }

    /// \return The first character of the stream, or an empty optional
    ///         if the stream is empty.
    [[nodiscard]] constexpr auto
    front() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.front();
    }

    /// \return Whether this stream contains at least N characters.
    [[nodiscard]] constexpr auto
    has(size_type n) const noexcept -> bool {
        return size() >= n;
    }

    /// Iterate over all lines in the stream.
    ///
    /// Returns a range that yields each line in the stream; the line
    /// separators are not included. The stream is not modified.
    [[nodiscard]] constexpr auto lines() const noexcept {
        return split(_s_default_line_separator());
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Check if this stream matches a regular expression.
    [[nodiscard]] bool matches(basic_regex<CharType>& regex) const noexcept {
        return regex.match(text());
    }

    /// Check if this stream matches a regular expression.
    ///
    /// Prefer the overload that takes a \c basic_regex object as this
    /// will recompile the regular expression every time it is called.
    ///
    /// If the regular expression fails to compile, this returns \c false.
    [[nodiscard]] bool matches(
        text_type regex_string,
        regex_flags flags = regex_flags::jit
    ) const {
        auto r = basic_regex<CharType>::create(regex_string, flags);
        if (not r) return false;
        return r.value().match(text());
    }
#endif

    /// Remove all instances of a character from the stream.
    template <typename Buffer = string_type>
    [[nodiscard]] auto remove(this basic_stream s, char_type c) -> Buffer {
        Buffer out;
        while (not s.empty()) {
            auto pos = s._m_text.find(c);
            if (pos == text_type::npos) {
                out += s._m_text;
                break;
            }

            out += s.take(pos);
            s.drop();
        }
        return out;
    }

    /// Remove all instances of a number of characters from the stream.
    template <typename Buffer = string_type>
    [[nodiscard]]
    auto remove_all(this basic_stream s, text_type chars) -> Buffer {
        Buffer out;
        while (not s.empty()) {
            auto pos = s._m_text.find_first_of(chars);
            if (pos == text_type::npos) {
                out += s._m_text;
                break;
            }

            out += s.take(pos);
            s.drop();
        }
        return out;
    }

    /// Replace all occurrences of a string in the stream and
    /// return the resulting string.
    [[nodiscard]] constexpr auto
    replace(text_type from, text_type to) const noexcept -> string_type {
        string_type str;
        usz pos = 0;
        for (;;) {
            auto next = _m_text.find(from, pos);
            if (next == text_type::npos) {
                str += _m_text.substr(pos);
                break;
            }

            str += _m_text.substr(pos, next - pos);
            str += to;
            pos = next + from.size();
        }
        return str;
    }

    [[nodiscard]] constexpr auto
    replace(char_type from, text_type to) const noexcept -> string_type {
        return replace(text_type{&from, 1}, to);
    }

    [[nodiscard]] constexpr auto
    replace(text_type from, char_type to) const noexcept -> string_type {
        return replace(from, text_type{&to, 1});
    }

    [[nodiscard]] constexpr auto
    replace(char_type from, char_type to) const noexcept -> string_type {
        return replace(text_type{&from, 1}, text_type{&to, 1});
    }

    /// \return The size (= number of characters) of this stream.
    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type {
        return _m_text.size();
    }

    /// \return The number of bytes in this stream.
    [[nodiscard]] constexpr auto
    size_bytes() const noexcept -> size_type {
        return size() * sizeof(char_type);
    }

    /// Split the stream into parts.
    [[nodiscard]] constexpr auto
    split(text_type delimiter) const noexcept {
        return _m_text | std::views::split(delimiter) | std::views::transform([](auto r) { return basic_stream(text_type(r)); });
    }

    ///@{
    /// \return True if the stream starts with the given character(s).
    [[nodiscard]] constexpr auto
    starts_with(char_type prefix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.front() == prefix;
    }

    /// \see starts_with(char_type) const
    [[nodiscard]] constexpr auto
    starts_with(text_type prefix) const noexcept -> bool {
        return _m_text.starts_with(prefix);
    }
    ///@}

    /// \return True if the stream starts with any of the given characters.
    [[nodiscard]] constexpr auto
    starts_with_any(text_type prefix) const -> bool {
        return not _m_text.empty() and basic_stream{prefix}.contains(_m_text.front());
    }

    /// Get N characters from the stream.
    ///
    /// If the stream contains fewer than N characters, this returns only
    /// the characters that are available. The stream is advanced by N
    /// characters.
    ///
    /// \param n The number of characters to get.
    /// \return The characters.
    [[nodiscard]] constexpr auto
    take(size_type n = 1) noexcept -> text_type {
        if (n > size()) n = size();
        return _m_advance(n);
    }

    /// @{
    /// Get a delimited character sequence from the stream.
    ///
    /// If the stream starts with a delimiter, the delimiter is removed
    /// and characters are read into a string view until a matching delimiter
    /// is found. Everything up to and including the matching delimiter is
    /// removed from the stream.
    ///
    /// If no opening or closing delimiter is found, the stream is not advanced
    /// at all and the \p string parameter is not written to.
    ///
    /// The \c _any overload instead looks for any one character in \p string to
    /// use as the delimiter. The closing delimiter must match the opening delimiter,
    /// however.
    ///
    /// \param string String view that will be set to the delimited character
    ///        range, excluding the delimiters.
    ///
    /// \param delimiter A string view to use as or containing the delimiters.
    ///
    /// \return True if a delimited sequence was found.
    [[nodiscard]] constexpr auto
    take_delimited(text_type& string, text_type delimiter) noexcept -> bool {
        if (not starts_with(delimiter)) return false;
        drop(delimiter.size());
        if (auto pos = _m_text.find(delimiter, delimiter.size()); pos != text_type::npos) {
            string = _m_text.substr(0, pos);
            _m_text.remove_prefix(pos + delimiter.size());
            return true;
        }
        return false;
    }

    /// \see take_delimited(text_type&, text_type)
    [[nodiscard]] constexpr auto
    take_delimited(char_type c, text_type& string) noexcept -> bool {
        return take_delimited(string, text_type{&c, 1});
    }

    /// \see take_delimited(text_type&, text_type)
    [[nodiscard]] constexpr auto
    take_delimited_any(text_type& string, text_type delimiters) noexcept -> bool {
        if (not starts_with_any(delimiters)) return false;
        char_type delim = take().front();
        if (auto pos = _m_text.find(delim); pos != text_type::npos) {
            string = _m_text.substr(0, pos);
            _m_text.remove_prefix(pos + 1);
            return true;
        }
        return false;
    }

    /// @}

    ///@{
    /// \brief Get characters from the end of the stream conditionally.
    ///
    /// These take either
    ///
    ///     1. a single character,
    ///     2. a string view, or
    ///     3. a unary predicate.
    ///
    /// The stream is advanced until (in the case of the \c _until overloads)
    /// or while (in the case of the \c _while overloads) we find a character
    ///
    ///     1. equal to the given character, or
    ///     2. equal to the given string (or any of the characters in the
    ///        string for the \c _any overloads), or
    ///     3. for which the given predicate returns \c true.
    ///
    /// If a matching character is found, all characters skipped over this way are
    /// returned as text.
    ///
    /// If the condition is not \c true for any of the characters in the stream,
    /// the entire text is returned (excepting the _while overloads, which return
    /// an empty string instead). The \c _or_empty overloads, return an empty
    /// string instead, and the stream is not advanced at all.
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return The matched characters.
    [[nodiscard]] constexpr auto
    take_back_until(char_type c) noexcept -> text_type {
        return _m_take_back<false>(_m_text.rfind(c));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until(text_type s) noexcept -> text_type {
        return _m_take_back<false>(_m_text.rfind(s), s.size());
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_any(text_type chars) noexcept -> text_type {
        return _m_take_back<false>(_m_text.find_last_of(chars));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_any_or_empty(text_type chars) noexcept -> text_type {
        return _m_take_back<true>(_m_text.find_last_of(chars));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_or_empty(char_type c) noexcept -> text_type {
        return _m_take_back<true>(_m_text.rfind(c));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_or_empty(text_type s) noexcept -> text_type {
        return _m_take_back<true>(_m_text.rfind(s), s.size());
    }
    ///@}

    ///@{
    /// \brief Get characters from the stream conditionally.
    ///
    /// These take either
    ///
    ///     1. a single character,
    ///     2. a string view (or possibly a range of string views), or
    ///     3. a unary predicate.
    ///
    /// The stream is advanced until (in the case of the \c _until overloads)
    /// or while (in the case of the \c _while overloads) we find a character
    ///
    ///     1. equal to the given character, or
    ///     2. equal to the given string (or any of the characters in the
    ///        string for the \c _any overloads), or
    ///     3. for which the given predicate returns \c true.
    ///
    /// If a matching character is found, all characters skipped over this way are
    /// returned as text.
    ///
    /// If the condition is not \c true for any of the characters in the stream,
    /// the entire text is returned (excepting the _while overloads, which return
    /// nothing instead). The \c _or_empty overloads, return an empty string instead,
    /// and the stream is not advanced at all. The \c _and_drop overloads additionally
    /// drop the delimiter afterwards if it is present.
    ///
    /// If an empty string view or range of string views is passed to the \c _any
    /// overloads, the entire stream is returned for the regular overloads, and
    /// nothing is returned for the \c _or_empty overloads (the choice as to what
    /// should happen in the former case is mostly arbitrary, but it seems like a
    /// reasonable counterpart what the \c _or_empty overloads naturally do in that
    /// case).
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return The matched characters.
    [[nodiscard]] constexpr auto
    take_until(char_type c) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find(c), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until(text_type s) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find(s), size()));
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_until_cond<false>(std::move(c));
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Overload of take_until() that uses a regex.
    [[nodiscard]] auto
    take_until(basic_regex<CharType>& regex) noexcept -> text_type {
        auto match = find(regex);
        return _m_advance(match ? match->start : size());
    }
#endif

    /// \see take_until(char_type)
    ///
    /// There currently is no _any overload for this since with those you typically
    /// *do* care what you actually found, unlike with this one where you might want
    /// to simply discard something that only serves as a delimiter.
    [[nodiscard]] constexpr auto
    take_until_and_drop(utils::convertible_to_any<char_type, text_type> auto c)
    noexcept -> text_type {
        // There are two cases to handle here. If the delimiter was found, the string
        // now starts with that delimiter, so consume() it will drop it; otherwise, we
        // have grabbed the entire string, and the stream is now empty, so consume() is
        // a no-op.
        auto res = take_until(c);
        (void) consume(c);
        return res;
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any(text_type chars) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find_first_of(chars), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any(utils::ConvertibleRange<text_type> auto&& strings) noexcept -> text_type {
        if (utils::Empty(strings)) return _m_advance(size());
        auto idx_range = vws::transform(LIBBASE_FWD(strings), [this](text_type t) { return _m_text.find(t); });
        auto pos = rgs::min(idx_range);
        return _m_advance(std::min(pos, size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any_or_empty(text_type chars) noexcept -> text_type {
        auto pos = _m_text.find_first_of(chars);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any_or_empty(utils::ConvertibleRange<text_type> auto&& strings) noexcept -> text_type {
        if (utils::Empty(strings)) return {};
        auto idx_range = vws::transform(LIBBASE_FWD(strings), [this](text_type t) { return _m_text.find(t); });
        auto pos = rgs::min(idx_range);
        if (pos == text_type::npos) return {};
        return _m_advance(std::min(pos, size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(char_type c) noexcept -> text_type {
        auto pos = _m_text.find(c);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(text_type s) noexcept -> text_type {
        auto pos = _m_text.find(s);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_until_cond<true>(std::move(c));
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Overload of take_until_or_empty() that uses a regex.
    [[nodiscard]] auto
    take_until_or_empty(basic_regex<CharType>& regex) noexcept -> text_type {
        auto match = find(regex);
        return match ? _m_advance(match->start) : text_type{};
    }
#endif

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_ws() noexcept -> text_type {
        return take_until_any(whitespace());
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while(char_type c) noexcept -> text_type {
        return _m_take_while(c);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_while(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_while_cond(std::move(c));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while_any(text_type chars) noexcept -> text_type {
        return _m_take_while_any(chars);
    }
    ///@}

    /// Get the underlying text for this stream.
    [[nodiscard]] constexpr auto
    text() const noexcept -> text_type { return _m_text; }

    ///@{
    /// \brief Remove characters from either end of the stream.
    ///
    /// Characters are deleted so long as the last/first character
    /// matches any of the given characters.
    ///
    /// \param cs The characters to remove.
    /// \return A reference to this stream.
    constexpr auto
    trim(text_type chars = whitespace()) noexcept -> basic_stream& {
        return trim_front(chars).trim_back(chars);
    }

    /// \see trim(text_type)
    constexpr auto
    trim_front(text_type chars = whitespace()) noexcept -> basic_stream& {
        auto pos = _m_text.find_first_not_of(chars);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_prefix(pos);
        return *this;
    }

    /// \see trim(text_type)
    constexpr auto
    trim_back(text_type chars = whitespace()) noexcept -> basic_stream& {
        auto pos = _m_text.find_last_not_of(chars);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_suffix(size() - pos - 1);
        return *this;
    }

    ///@}

    /// \return A string view containing ASCII whitespace characters
    /// as appropriate for the character type of this stream.
    [[nodiscard]] static consteval auto whitespace() -> text_type {
        return LIBBASE_STREAM_STRING_LITERAL(" \t\n\r\v\f");
    }

    /// Get a character from the stream.
    ///
    /// \param index The index of the character to get.
    /// \return The character at the given index.
    [[nodiscard]] constexpr auto
    operator[](size_type index) const noexcept -> char_opt {
        if (index >= size()) [[unlikely]]
            return std::nullopt;
        return _m_text.at(index);
    }

    /// Get a slice of characters from the stream.
    ///
    /// The \p start and \p end parameters are clamped between
    /// 0 and the length of the stream.
    ///
    /// \param start The index of the first character to get.
    /// \param end The index of the last character to get.
    /// \return The characters in the given range.
    [[nodiscard]] constexpr auto
    operator[](size_type start, size_type end) const noexcept -> basic_stream {
        if (start > size()) [[unlikely]]
            start = size();
        if (end > size()) end = size();
        return basic_stream{_m_text.substr(start, end - start)};
    }

    [[nodiscard]] constexpr auto
    operator<=>(const basic_stream& other) const noexcept = default;
    /// @}

private:
    // Return characters until position `n` (exclusive)
    // and remove them from the stream.
    constexpr auto _m_advance(size_type n) LIBBASE_NOEXCEPT_UNLESS_TESTING -> text_type {
        Assert(n <= size(), "Should never get an out-of-bounds index here");
        auto txt = _m_text.substr(0, n);
        _m_text.remove_prefix(n);
        return txt;
    }

    // Return characters starting from position `n` (exclusive)
    // and remove them from the stream.
    //
    // Note: `skip` must be set to the number of characters we matched,
    // i.e. 1 for char_type and `_any` overloads (because we match at
    // most one character in the string view in that case), and the
    // length of the needle otherwise.
    template <bool _or_empty>
    constexpr auto _m_take_back(size_type pos, size_type skip = 1) noexcept -> text_type {
        if (skip == 0 or pos == text_type::npos) {
            if constexpr (_or_empty) return {};
            else return std::exchange(_m_text, {});
        }

        pos += skip;
        auto tail = _m_text.substr(pos);
        _m_text.remove_suffix(size() - pos);
        return tail;
    }

    template <bool _or_empty, typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    _m_take_until_cond(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> text_type {
        std::size_t i = 1;
        for (; i <= size(); ++i)
            if (c(_m_text[i - 1]))
                break;

        if (i > size()) {
            if constexpr (_or_empty) return {};
            else return std::exchange(_m_text, {});
        }

        return _m_advance(i - 1);
    }

    [[nodiscard]] constexpr auto
    _m_take_while(char_type c) noexcept -> text_type {
        return _m_take_while_cond(
            [c](char_type ch) { return ch == c; }
        );
    }

    [[nodiscard]] constexpr auto
    _m_take_while_any(text_type chars) noexcept -> text_type {
        return _m_take_while_cond([chars](char_type ch) {
            return chars.find(ch) != text_type::npos;
        });
    }

    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    _m_take_while_cond(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> text_type {
        std::size_t i = 1;
        for (; i <= size(); ++i)
            if (not c(_m_text[i - 1]))
                break;

        if (i > size()) return std::exchange(_m_text, {});
        return _m_advance(i - 1);
    }
};

using stream = basic_stream<char>;
using wstream = basic_stream<wchar_t>;
using u8stream = basic_stream<char8_t>;
using u16stream = basic_stream<char16_t>;
using u32stream = basic_stream<char32_t>;
} // namespace base

#endif
