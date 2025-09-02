#ifndef LIBBASE_STR_HH
#define LIBBASE_STR_HH

#include <algorithm>
#include <base/Assert.hh>
#include <base/Regex.hh>
#include <base/Utils.hh>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#define LIBBASE_STR_LIT(lit) [] {                           \
    using namespace std::string_view_literals;              \
    if constexpr (std::is_same_v<char_type, char>)          \
        return lit ""sv;                                    \
    else if constexpr (std::is_same_v<char_type, wchar_t>)  \
        return L"" lit ""sv;                                \
    else if constexpr (std::is_same_v<char_type, char8_t>)  \
        return u8"" lit ""sv;                               \
    else if constexpr (std::is_same_v<char_type, char16_t>) \
        return u"" lit ""sv;                                \
    else if constexpr (std::is_same_v<char_type, char32_t>) \
        return U"" lit ""sv;                                \
    else                                                    \
        static_assert(false, "Unsupported character type"); \
}()

namespace base {
/// 'std::string_view' replacement.
///
/// This is basically a 'std::basic_string_view', except that it has a number
/// of additional helper functions mostly related to parsing and other text
/// transformations.
///
/// This used to be called 'stream' and was used *only* for parsing, but over
/// time I was finding myself writing 'stream(sv).something()' all the time
/// (where 'sv' is a std::string_view), so I decided it would be simpler to just
/// switch to using 'str' everywhere instead.
template <typename CharType>
class basic_str {
public:
    using char_type = CharType;
    using char_opt = std::optional<char_type>;
    using text_type = std::basic_string_view<char_type>;
    using size_type = std::size_t;
    using string_type = std::basic_string<char_type>;
    using iterator = text_type::iterator;
    using reverse_iterator = text_type::reverse_iterator;

private:
    text_type _m_text;

public:
    /// Construct an empty string.
    constexpr basic_str() = default;

    /// Construct a new string from text.
    constexpr basic_str(text_type text) noexcept : _m_text(text) {}

    /// Construct a new string from a span.
    constexpr basic_str(std::span<char_type> span) noexcept : _m_text(span.data(), span.size()) {}

    /// Construct a new string from text.
    constexpr basic_str(const string_type& text) noexcept : _m_text(text) {}

    /// Construct a new string from characters.
    constexpr basic_str(const char_type* chars, size_type size) noexcept
        : _m_text(chars, size) {}

    /// Construct a new string from a string literal.
    template <usz n>
    constexpr basic_str(const char_type (&chars)[n]) noexcept
        : _m_text(chars, n - 1) {}

    /// \return The last character of the string, or an empty optional
    ///         if the string is empty.
    [[nodiscard]] constexpr auto
    back() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.back();
    }

    /// Get an iterator to the start of the string.
    [[nodiscard]] constexpr auto
    begin() const noexcept -> iterator {
        return _m_text.begin();
    }

    /// Get an iterator to the start of the string.
    [[nodiscard]] constexpr auto
    cbegin() const noexcept -> iterator {
        return _m_text.begin();
    }

    /// Get an iterator to the end of the string.
    [[nodiscard]] constexpr auto
    cend() const noexcept -> iterator {
        return _m_text.end();
    }

    /// \return The data pointer as a \c const \c char*.
    [[nodiscard]] constexpr auto
    char_ptr() const noexcept -> const char* {
        // Hack to avoid reinterpret_cast at compile time.
        if constexpr (std::same_as<char_type, char>) return _m_text.data();
        else return reinterpret_cast<const char*>(_m_text.data());
    }

#ifdef __cpp_lib_ranges_chunk
    /// Iterate over the text in chunks.
    [[nodiscard]] constexpr auto
    chunks(size_type size) const noexcept {
        return _m_text | vws::chunk(size) | vws::transform([](auto r) { return basic_str(text_type(r)); });
    }
#endif

    /// Skip a character.
    ///
    /// If the first character of the string is the given character,
    /// remove it from the string. Otherwise, do nothing.
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
    /// If the string starts with the given list of characters, in that
    /// order, they are removed from the string. Otherwise, do nothing.
    ///
    /// \param chars The characters to skip.
    /// \return True if the characters were skipped.
    [[nodiscard]] constexpr auto
    consume(basic_str chars) noexcept -> bool {
        if (not starts_with(chars)) return false;
        drop(chars.size());
        return true;
    }

    /// Skip any of a set of characters.
    ///
    /// \param chars The characters to skip.
    /// \return True if any of the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_any(basic_str chars) noexcept -> bool {
        if (not starts_with_any(chars)) return false;
        drop();
        return true;
    }

    /// Skip a character.
    ///
    /// If the first character of the string is the given character,
    /// remove it from the string. Otherwise, do nothing.
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
    /// If the string starts with the given list of characters, in that
    /// order, they are removed from the string. Otherwise, do nothing.
    ///
    /// \param chars The characters to skip.
    /// \return True if the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_back(basic_str chars) noexcept -> bool {
        if (not ends_with(chars)) return false;
        drop_back(chars.size());
        return true;
    }

    /// Skip any of a set of characters.
    ///
    /// \param chars The characters to skip.
    /// \return True if any of the characters were skipped.
    [[nodiscard]] constexpr auto
    consume_back_any(basic_str chars) noexcept -> bool {
        if (not ends_with_any(chars)) return false;
        drop_back();
        return true;
    }

    /// Check if the string contains a character.
    [[nodiscard]] constexpr auto
    contains(char_type c) const noexcept -> bool {
        auto pos = _m_text.find(c);
        return pos != text_type::npos;
    }

    /// Check if the text contains a string.
    [[nodiscard]] constexpr auto
    contains(basic_str s) const noexcept -> bool {
        auto pos = _m_text.find(s._m_text);
        return pos != text_type::npos;
    }

    /// Check if the string contains any of a set of characters.
    [[nodiscard]] constexpr auto
    contains_any(basic_str chars) const noexcept -> bool {
        auto pos = _m_text.find_first_of(chars._m_text);
        return pos != text_type::npos;
    }

    /// Count the number of occurrences of a character.
    [[nodiscard]] constexpr auto
    count(char_type c) const noexcept -> size_type {
        return size_type(std::ranges::count(_m_text, c));
    }

    /// Count the number of occurrences of a substring.
    [[nodiscard]] constexpr auto
    count(basic_str s) const noexcept -> size_type {
        size_type count = 0;
        for (
            size_type pos = 0;
            (pos = _m_text.find(s._m_text, pos)) != text_type::npos;
            pos += s.size()
        ) ++count;
        return count;
    }

    /// Count the number of any of a set of characters.
    [[nodiscard]] constexpr auto
    count_any(basic_str chars) const noexcept -> size_type {
        return size_type(std::ranges::count_if(_m_text, [chars](char_type c) {
            return chars.contains(c);
        }));
    }

    /// \return The data pointer.
    [[nodiscard]] constexpr auto
    data() const noexcept -> const char_type* {
        return _m_text.data();
    }

    /// Discard N characters from the string.
    ///
    /// If the string contains fewer than N characters, this clears
    /// the entire string.
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
    drop(size_ty n = 1) noexcept -> basic_str& {
        if constexpr (not std::unsigned_integral<size_ty>) {
            if (n < 0) return drop_back(static_cast<size_type>(-n));
        }

        (void) take(static_cast<size_type>(n));
        return *this;
    }

    /// Discard N characters from the back of the string.
    ///
    /// If the string contains fewer than N characters, this clears
    /// the entire string.
    ///
    /// \param n The number of characters to drop. If negative, calls
    ///          drop() instead.
    ///
    /// \return This.
    template <std::integral size_ty = size_type>
    requires (not std::same_as<std::remove_cvref_t<size_ty>, char_type>)
    constexpr auto
    drop_back(size_ty n = 1) noexcept -> basic_str& {
        if constexpr (not std::unsigned_integral<size_ty>) {
            if (n < 0) return drop(static_cast<size_type>(-n));
        }

        if (static_cast<size_type>(n) >= _m_text.size()) _m_text = {};
        else _m_text.remove_suffix(static_cast<size_type>(n));
        return *this;
    }

    ///@{
    /// \brief Drop characters from the back string conditionally.
    ///
    /// Same as \c take_back_until() and friends, but discards the
    /// characters instead and returns the string.
    ///
    /// \see take_back_until(char_type)
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return This
    constexpr auto
    drop_back_until(char_type c) noexcept -> basic_str& {
        (void) take_back_until(c);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until(basic_str s) noexcept -> basic_str& {
        (void) take_back_until(s);
        return *this;
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_back_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str& {
        (void) take_back_until(std::move(c));
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_any(basic_str chars) noexcept -> basic_str& {
        (void) take_back_until_any(chars);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_any_or_empty(basic_str chars) noexcept -> basic_str& {
        (void) take_back_until_any_or_empty(chars);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_or_empty(char_type c) noexcept -> basic_str& {
        (void) take_back_until_or_empty(c);
        return *this;
    }

    /// \see take_back_until(char_type)
    constexpr auto
    drop_back_until_or_empty(basic_str s) noexcept -> basic_str& {
        (void) take_back_until_or_empty(s);
        return *this;
    }

    /// \see take_back_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_back_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str& {
        (void) take_back_until_or_empty(std::move(c));
        return *this;
    }
    ///@}

    ///@{
    /// \brief Drop characters from the string conditionally.
    ///
    /// Same as \c take_until() and friends, but discards the
    /// characters instead and returns the string.
    ///
    /// \see take_until(char_type)
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return This
    constexpr auto
    drop_until(char_type c) noexcept -> basic_str& {
        (void) take_until(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until(basic_str s) noexcept -> basic_str& {
        (void) take_until(s);
        return *this;
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str& {
        (void) take_until(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any(basic_str chars) noexcept -> basic_str& {
        (void) take_until_any(chars);
        return *this;
    }

    constexpr auto
    drop_until_any(utils::ConvertibleRange<basic_str> auto&& strings) noexcept -> basic_str {
        (void) take_until_any(LIBBASE_FWD(strings));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any_or_empty(basic_str chars) noexcept -> basic_str& {
        (void) take_until_any_or_empty(chars);
        return *this;
    }

    constexpr auto
    drop_until_any_or_empty(utils::ConvertibleRange<basic_str> auto&& strings) noexcept -> basic_str {
        (void) take_until_any_or_empty(LIBBASE_FWD(strings));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(char_type c) noexcept -> basic_str& {
        (void) take_until_or_empty(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(basic_str s) noexcept -> basic_str& {
        (void) take_until_or_empty(s);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str& {
        (void) take_until_or_empty(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_ws() noexcept -> basic_str& {
        (void) take_until_ws();
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while(char_type c) noexcept -> basic_str& {
        (void) take_while(c);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_while(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str& {
        (void) take_while(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while_any(basic_str chars) noexcept -> basic_str& {
        (void) take_while_any(chars);
        return *this;
    }
    ///@}

    /// Check if this string is empty.
    [[nodiscard]] constexpr auto
    empty() const noexcept -> bool {
        return _m_text.empty();
    }

    /// Get an iterator to the end of the string.
    [[nodiscard]] constexpr auto
    end() const noexcept -> iterator {
        return _m_text.end();
    }

    ///@{
    /// \return True if the string ends with the given character(s).
    [[nodiscard]] constexpr auto
    ends_with(char_type suffix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.back() == suffix;
    }

    /// \see ends_with(char_type) const
    [[nodiscard]] constexpr auto
    ends_with(basic_str suffix) const noexcept -> bool {
        return _m_text.ends_with(suffix);
    }
    ///@}

    /// \return True if the string ends with any of the given characters.
    [[nodiscard]] constexpr auto
    ends_with_any(basic_str suffix) const -> bool {
        return not _m_text.empty() and basic_str{suffix}.contains(_m_text.back());
    }

    /// Escape a set of characters.
    ///
    /// \param chars The characters to escape in the string.
    /// \param escape The string to use for escaping (default: "\")
    [[nodiscard]] constexpr auto
    escape(
        basic_str chars,
        basic_str escape = LIBBASE_STR_LIT("\\")
    ) const -> string_type {
        string_type escaped;
        basic_str s{*this};
        for (;;) {
            escaped += s.take_until_any(chars)._m_text;
            if (s.empty()) return escaped;
            escaped += escape._m_text;
            escaped += s.take()._m_text;
        }
    }

    /// Extract a series of characters from a string.
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
    /// Find the first occurrence of a regular expression in the string.
    [[nodiscard]] auto
    find(basic_regex<CharType>& regex) const noexcept -> std::optional<regex_match> {
        return regex.find(*this);
    }

    /// Find the first occurrence of a regular expression in the string.
    ///
    /// Prefer the overload that takes a \c basic_regex object as this
    /// will recompile the regular expression every time it is called.
    ///
    /// If the regular expression fails to compile, this returns \c std::nullopt.
    [[nodiscard]] auto find_regex(
        basic_str regex_string,
        regex_flags flags = regex_flags::jit
    ) const -> std::optional<regex_match> {
        auto r = basic_regex<CharType>::create(regex_string, flags);
        if (not r) return std::nullopt;
        return find(r.value());
    }
#endif

    /// Find the first occurrence of a character in this string.
    [[nodiscard]] constexpr auto
    first(char_type c) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.find(c);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    /// Find the first occurrence of a character in this string.
    [[nodiscard]] constexpr auto
    first(basic_str str) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.find(str._m_text);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    /// Find the first occurrence of a character in this string that
    /// satisfies a predicate.
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    first(UnaryPredicate pred) const noexcept -> std::optional<size_type> {
        auto pos = rgs::find_if(_m_text, std::move(pred));
        if (pos == _m_text.end()) return std::nullopt;
        return size_type(pos - _m_text.begin());
    }

    /// Find the first occurrence of any of a list of characters.
    [[nodiscard]] constexpr auto
    first_any(basic_str str) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.find_first_of(str._m_text);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    ///@{
    /// Delete all occurrences of consecutive characters, replacing them
    /// with another string.
    [[nodiscard]] constexpr auto
    fold_any(basic_str chars, basic_str replacement) const noexcept -> string_type {
        string_type ret;
        basic_str s{*this};
        while (not s.empty()) {
            ret += s.take_until_any(chars);
            if (not s.take_while_any(chars).empty()) ret += replacement;
        }
        return ret;
    }

    /// Delete all occurrences of consecutive characters, replacing them
    /// with another character.
    [[nodiscard]] constexpr auto
    fold_any(basic_str chars, char_type replacement) const noexcept -> string_type {
        return fold_any(chars, basic_str{&replacement, 1});
    }
    ///@}

    /// Equivalent to `fold(basic_str::whitespace())`.
    [[nodiscard]] constexpr auto
    fold_ws(basic_str replacement = LIBBASE_STR_LIT(" ")) const noexcept -> string_type {
        return fold_any(whitespace(), replacement);
    }

    /// \return The first character of the string, or an empty optional
    ///         if the string is empty.
    [[nodiscard]] constexpr auto
    front() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.front();
    }

    /// \return Whether this string contains at least N characters.
    [[nodiscard]] constexpr auto
    has(size_type n) const noexcept -> bool {
        return size() >= n;
    }

    /// Find the last occurrence of a character in this string.
    [[nodiscard]] constexpr auto
    last(char_type c) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.rfind(c);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    /// Find the last occurrence of a character in this string.
    [[nodiscard]] constexpr auto
    last(basic_str str) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.rfind(str._m_text);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    /// Find the last occurrence of a character in this string that
    /// satisfies a predicate.
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    last(UnaryPredicate pred) const noexcept -> std::optional<size_type> {
        auto pos = rgs::find_if(_m_text | vws::reverse, std::move(pred));
        if (pos == _m_text.rend()) return std::nullopt;
        return size_type(pos.base() - _m_text.begin() - 1);
    }

    /// Find the last occurrence of any of a list of characters.
    [[nodiscard]] constexpr auto
    last_any(basic_str str) const noexcept -> std::optional<size_type> {
        auto pos = _m_text.find_last_of(str._m_text);
        if (pos == text_type::npos) return std::nullopt;
        return pos;
    }

    /// Iterate over all lines in the string.
    ///
    /// Returns a range that yields each line in the string; the line
    /// separators are not included. The string is not modified.
    ///
    /// This always splits on '\n', irrespective of platform.
    [[nodiscard]] constexpr auto lines() const noexcept {
        return split(LIBBASE_STR_LIT("\n"));
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Check if this string matches a regular expression.
    [[nodiscard]] bool matches(basic_regex<CharType>& regex) const noexcept {
        return regex.match(_m_text);
    }

    /// Check if this string matches a regular expression.
    ///
    /// Prefer the overload that takes a \c basic_regex object as this
    /// will recompile the regular expression every time it is called.
    ///
    /// If the regular expression fails to compile, this returns \c false.
    [[nodiscard]] bool matches(
        basic_str regex_string,
        regex_flags flags = regex_flags::jit
    ) const {
        auto r = basic_regex<CharType>::create(regex_string, flags);
        if (not r) return false;
        return r.value().match(_m_text);
    }
#endif

    /// Get the maximum number of characters that this can store.
    [[nodiscard]] constexpr auto
    max_size() const noexcept { return _m_text.max_size(); }

    /// Shrink this string to a substring of itself.
    constexpr auto
    narrow(size_type start, size_type n) noexcept -> basic_str& {
        *this = slice(start, n);
        return *this;
    }

    /// Remove all instances of a character from the string.
    template <typename Buffer = string_type>
    [[nodiscard]] auto remove(this basic_str s, char_type c) -> Buffer {
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

    /// Remove all instances of a number of characters from the string.
    template <typename Buffer = string_type>
    [[nodiscard]]
    auto remove_all(this basic_str s, basic_str chars) -> Buffer {
        Buffer out;
        while (not s.empty()) {
            auto pos = s._m_text.find_first_of(chars._m_text);
            if (pos == text_type::npos) {
                out += s._m_text;
                break;
            }

            out += s.take(pos);
            s.drop();
        }
        return out;
    }

    /// Replace all occurrences of a string in the string and
    /// return the resulting string.
    ///
    /// \see base::trie for replacing multiple strings at once.
    [[nodiscard]] constexpr auto
    replace(basic_str from, basic_str to) const -> string_type {
        string_type str;
        usz pos = 0;
        for (;;) {
            auto next = _m_text.find(from._m_text, pos);
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

    /// \see replace(basic_str from, basic_str to) const
    [[nodiscard]] constexpr auto
    replace(char_type from, basic_str to) const -> string_type {
        return replace(basic_str{&from, 1}, to);
    }

    /// \see replace(basic_str from, basic_str to) const
    [[nodiscard]] constexpr auto
    replace(basic_str from, char_type to) const -> string_type {
        return replace(from, basic_str{&to, 1});
    }

    /// \see replace(basic_str from, basic_str to) const
    [[nodiscard]] constexpr auto
    replace(char_type from, char_type to) const -> string_type {
        return replace(basic_str{&from, 1}, basic_str{&to, 1});
    }

    /// Match and replace individual characters.
    ///
    /// This is similar to the Unix 'tr' utility.
    ///
    /// Given two strings of length N, this replaces every occurrence
    /// of the I-th character of 'from' in this string with the I-th
    /// character of 'to'. If 'to' is shorter 'from', excess characters
    /// will simply be deleted.
    ///
    /// If 'from' contains multiple instances of the same character,
    /// all but the first will never be matched.
    ///
    /// Example:
    ///
    ///   str("ababcd").replace_many("abc", "ef") // Yields "efefd".
    ///
    /// \see base::trie for replacing multiple (multi-character) strings
    /// in one pass.
    [[nodiscard]] constexpr auto
    replace_many(basic_str from, basic_str to) const -> string_type {
        string_type out;
        basic_str s{*this};
        for (;;) {
            out += s.take_until_any(from);
            if (s.empty()) return out;
            auto idx = from.first(s.take()[0]);
            if (idx.has_value() and idx.value() < to.size())
                out += to[idx.value()];
        }
    }

    /// Get a reverse iterator to the start of the string.
    [[nodiscard]] constexpr auto
    rbegin() const noexcept -> reverse_iterator {
        return _m_text.rbegin();
    }

    /// Get a reverse iterator to the end of the string.
    [[nodiscard]] constexpr auto
    rend() const noexcept -> reverse_iterator {
        return _m_text.rend();
    }

    /// Reverse this string.
    [[nodiscard]] constexpr auto
    reverse() const -> string_type {
        return _m_text | vws::reverse | rgs::to<string_type>();
    }

    /// \return The size (= number of characters) of this string.
    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type {
        return _m_text.size();
    }

    /// \return The number of bytes in this string.
    [[nodiscard]] constexpr auto
    size_bytes() const noexcept -> size_type {
        return size() * sizeof(char_type);
    }

    /// Get a slice of characters from the string.
    ///
    /// The \p start and \p end parameters are clamped between
    /// 0 and the length of the string.
    ///
    /// \param start The index of the first character to get.
    /// \param end The index of the last character to get.
    /// \return The characters in the given range.
    [[nodiscard]] constexpr auto
    slice(size_type start, size_type n) const noexcept -> basic_str {
        return basic_str(*this).drop(start).take(n);
    }

    /// Split the string into parts.
    [[nodiscard]] constexpr auto
    split(basic_str delimiter) const noexcept {
        return _m_text
            | vws::split(delimiter)
            | vws::transform([](auto r) { return basic_str(text_type(r)); });
    }

    ///@{
    /// \return True if the string starts with the given character(s).
    [[nodiscard]] constexpr auto
    starts_with(char_type prefix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.front() == prefix;
    }

    /// \see starts_with(char_type) const
    [[nodiscard]] constexpr auto
    starts_with(basic_str prefix) const noexcept -> bool {
        return _m_text.starts_with(prefix._m_text);
    }
    ///@}

    /// \return True if the string starts with any of the given characters.
    [[nodiscard]] constexpr auto
    starts_with_any(basic_str prefix) const -> bool {
        return not _m_text.empty() and basic_str{prefix}.contains(_m_text.front());
    }

    /// Prefer to use slice() instead; this exists for compatibility w/ std::string_view.
    [[nodiscard]] constexpr auto
    substr(size_type start, size_type n) const noexcept -> basic_str {
        return slice(start, n);
    }

    /// Swap two strings.
    constexpr void swap(basic_str& other) noexcept {
        _m_text.swap(other._m_text);
    }

    /// Get N characters from the string.
    ///
    /// If the string contains fewer than N characters, this returns only
    /// the characters that are available. The string is advanced by N
    /// characters.
    ///
    /// \param n The number of characters to get.
    /// \return The characters.
    [[nodiscard]] constexpr auto
    take(size_type n = 1) noexcept -> basic_str {
        if (n > size()) n = size();
        return _m_advance(n);
    }

    /// @{
    /// Get a delimited character sequence from the string.
    ///
    /// If the string starts with a delimiter, the delimiter is removed
    /// and characters are read into a string view until a matching delimiter
    /// is found. Everything up to and including the matching delimiter is
    /// removed from the string.
    ///
    /// If no opening or closing delimiter is found, the string is not advanced
    /// at all and the \p string parameter is not written to.
    ///
    /// The \c _any overload instead looks for any one character in \p string to
    /// use as the delimiter. The closing delimiter must match the opening delimiter,
    /// however.
    ///
    /// \param string str view that will be set to the delimited character
    ///        range, excluding the delimiters.
    ///
    /// \param delimiter A string view to use as or containing the delimiters.
    ///
    /// \return True if a delimited sequence was found.
    [[nodiscard]] constexpr auto
    take_delimited(basic_str& string, basic_str delimiter) -> bool {
        Assert(not delimiter.empty(), "Delimiter must not be empty");

        // Discard the first delimiter and find the second delimiter; take care
        // not to modify this string until we’ve actually found both delimiters.
        if (not starts_with(delimiter)) return false;
        auto pos = _m_text.substr(delimiter.size()).find(delimiter._m_text, delimiter.size());
        if (pos == text_type::npos) return false;

        // We’ve found both.
        string = _m_text.substr(delimiter.size(), pos);
        _m_text.remove_prefix(pos + delimiter.size() * 2);
        return true;
    }

    /// \see take_delimited(basic_str&, basic_str)
    [[nodiscard]] constexpr auto
    take_delimited(char_type c, basic_str& string) -> bool {
        return take_delimited(string, basic_str{&c, 1});
    }

    /// \see take_delimited(basic_str&, basic_str)
    [[nodiscard]] constexpr auto
    take_delimited_any(basic_str& string, basic_str delimiters) -> bool {
        Assert(not delimiters.empty(), "At least one delimiter is required");

        // Discard the first delimiter and find the second delimiter; take care
        // not to modify this string until we’ve actually found both delimiters.
        if (not starts_with_any(delimiters)) return false;
        char_type delim = _m_text[0];
        auto pos = _m_text.substr(1).find(delim);
        if (pos == text_type::npos) return false;

        // We’ve found both.
        string = _m_text.substr(1, pos);
        _m_text.remove_prefix(pos + 2);
        return true;
    }

    /// @}

    ///@{
    /// \brief Get characters from the end of the string conditionally.
    ///
    /// These take either
    ///
    ///     1. a single character,
    ///     2. a string view, or
    ///     3. a unary predicate.
    ///
    /// The string is advanced until (in the case of the \c _until overloads)
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
    /// If the condition is not \c true for any of the characters in the string,
    /// the entire text is returned (excepting the _while overloads, which return
    /// an empty string instead). The \c _or_empty overloads, return an empty
    /// string instead, and the string is not advanced at all.
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return The matched characters.
    [[nodiscard]] constexpr auto
    take_back_until(char_type c) noexcept -> basic_str {
        return _m_take_back<false>(_m_text.rfind(c));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until(basic_str s) noexcept -> basic_str {
        return _m_take_back<false>(_m_text.rfind(s._m_text), s.size());
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_any(basic_str chars) noexcept -> basic_str {
        return _m_take_back<false>(_m_text.find_last_of(chars._m_text));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_any_or_empty(basic_str chars) noexcept -> basic_str {
        return _m_take_back<true>(_m_text.find_last_of(chars._m_text));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_or_empty(char_type c) noexcept -> basic_str {
        return _m_take_back<true>(_m_text.rfind(c));
    }

    /// \see take_back_until(char_type)
    [[nodiscard]] constexpr auto
    take_back_until_or_empty(basic_str s) noexcept -> basic_str {
        return _m_take_back<true>(_m_text.rfind(s._m_text), s.size());
    }
    ///@}

    ///@{
    /// \brief Get characters from the string conditionally.
    ///
    /// These take either
    ///
    ///     1. a single character,
    ///     2. a string view (or possibly a range of string views), or
    ///     3. a unary predicate.
    ///
    /// The string is advanced until (in the case of the \c _until overloads)
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
    /// If the condition is not \c true for any of the characters in the string,
    /// the entire text is returned (excepting the _while overloads, which return
    /// nothing instead). The \c _or_empty overloads, return an empty string instead,
    /// and the string is not advanced at all. The \c _and_drop overloads additionally
    /// drop the delimiter afterwards if it is present.
    ///
    /// If an empty string view or range of string views is passed to the \c _any
    /// overloads, the entire string is returned for the regular overloads, and
    /// nothing is returned for the \c _or_empty overloads (the choice as to what
    /// should happen in the former case is mostly arbitrary, but it seems like a
    /// reasonable counterpart what the \c _or_empty overloads naturally do in that
    /// case).
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return The matched characters.
    [[nodiscard]] constexpr auto
    take_until(char_type c) noexcept -> basic_str {
        return _m_advance(std::min(_m_text.find(c), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until(basic_str s) noexcept -> basic_str {
        return _m_advance(std::min(_m_text.find(s._m_text), size()));
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str {
        return _m_take_until_cond<false>(std::move(c));
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Overload of take_until() that uses a regex.
    [[nodiscard]] auto
    take_until(basic_regex<CharType>& regex) noexcept -> basic_str {
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
    take_until_and_drop(char_type c)
    noexcept -> basic_str {
        // There are two cases to handle here. If the delimiter was found, the string
        // now starts with that delimiter, so consume() it will drop it; otherwise, we
        // have grabbed the entire string, and the string is now empty, so consume() is
        // a no-op.
        auto res = take_until(c);
        (void) consume(c);
        return res;
    }

    /// \see take_until_and_drop(char_type)
    [[nodiscard]] constexpr auto
    take_until_and_drop(basic_str c)
    noexcept -> basic_str {
        auto res = take_until(c);
        (void) consume(c);
        return res;
    }

    /// \see take_until(char_type)
    ///
    /// As \c take_until, but also includes the delimiter.
    [[nodiscard]] constexpr auto
    take_until_and_take(char_type c)
    noexcept -> basic_str {
        auto res = take_until(c);
        if (consume(c)) res = {res.data(), res.size() + 1};
        return res;
    }

    [[nodiscard]] constexpr auto
    take_until_and_take(basic_str str)
    noexcept -> basic_str {
        auto res = take_until(str);
        if (consume(str)) res = {res.data(), res.size() + str.size()};
        return res;
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any(basic_str chars) noexcept -> basic_str {
        return _m_advance(std::min(_m_text.find_first_of(chars._m_text), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any(utils::ConvertibleRange<basic_str> auto&& strings) noexcept -> basic_str {
        if (utils::Empty(strings)) return _m_advance(size());
        auto idx_range = vws::transform(LIBBASE_FWD(strings), [this](basic_str t) { return _m_text.find(t._m_text); });
        auto pos = rgs::min(idx_range);
        return _m_advance(std::min(pos, size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any_or_empty(basic_str chars) noexcept -> basic_str {
        auto pos = _m_text.find_first_of(chars._m_text);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any_or_empty(utils::ConvertibleRange<basic_str> auto&& strings) noexcept -> basic_str {
        if (utils::Empty(strings)) return {};
        auto idx_range = vws::transform(LIBBASE_FWD(strings), [this](basic_str t) { return _m_text.find(t._m_text); });
        auto pos = rgs::min(idx_range);
        if (pos == text_type::npos) return {};
        return _m_advance(std::min(pos, size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(char_type c) noexcept -> basic_str {
        auto pos = _m_text.find(c);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(basic_str s) noexcept -> basic_str {
        auto pos = _m_text.find(s._m_text);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until_or_empty(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str {
        return _m_take_until_cond<true>(std::move(c));
    }

#ifdef LIBBASE_ENABLE_PCRE2
    /// Overload of take_until_or_empty() that uses a regex.
    [[nodiscard]] auto
    take_until_or_empty(basic_regex<CharType>& regex) noexcept -> basic_str {
        auto match = find(regex);
        return match ? _m_advance(match->start) : basic_str{};
    }
#endif

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_ws() noexcept -> basic_str {
        return take_until_any(whitespace());
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while(char_type c) noexcept -> basic_str {
        return _m_take_while(c);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_while(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str {
        return _m_take_while_cond(std::move(c));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while_any(basic_str chars) noexcept -> basic_str {
        return _m_take_while_any(chars);
    }
    ///@}

    /// Get the underlying string view for this string.
    ///
    /// This function is deprecated because str is implicitly convertible
    /// to string_view and should also just be used instead of string_view.
    [[deprecated("Try just not calling text()")]]
    [[nodiscard]] constexpr auto
    text() const noexcept -> text_type { return _m_text; }

    ///@{
    /// \brief Remove characters from either end of the string.
    ///
    /// Characters are deleted so long as the last/first character
    /// matches any of the given characters.
    ///
    /// \param cs The characters to remove.
    /// \return A reference to this string.
    constexpr auto
    trim(basic_str chars = whitespace()) noexcept -> basic_str& {
        return trim_front(chars).trim_back(chars);
    }

    /// \see trim(basic_str)
    constexpr auto
    trim_front(basic_str chars = whitespace()) noexcept -> basic_str& {
        auto pos = _m_text.find_first_not_of(chars._m_text);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_prefix(pos);
        return *this;
    }

    /// \see trim(basic_str)
    constexpr auto
    trim_back(basic_str chars = whitespace()) noexcept -> basic_str& {
        auto pos = _m_text.find_last_not_of(chars._m_text);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_suffix(size() - pos - 1);
        return *this;
    }

    ///@}

    /// Convert this to a string, copying the underlying data.
    [[nodiscard]] constexpr auto
    string() const -> string_type { return string_type(_m_text); }

    /// \return A string view containing ASCII whitespace characters
    /// as appropriate for the character type of this string.
    [[nodiscard]] static consteval auto whitespace() -> basic_str {
        return LIBBASE_STR_LIT(" \t\n\r\v\f");
    }

    /// Get a character from the string.
    ///
    /// \param index The index of the character to get.
    /// \return The character at the given index.
    [[nodiscard]] constexpr auto
    operator[](size_type index) const -> char_type {
        Assert(index < size(), "Index out of bounds");
        return _m_text.at(index);
    }

    /// Equality comparison operator.
    [[nodiscard]] constexpr bool
    operator==(const std::convertible_to<basic_str> auto& other) const noexcept {
        return basic_str(other)._m_text == _m_text;
    }

    /// Three-way comparison operator.
    ///
    /// We can’t simply '= default' this since the implicit conversion to a string
    /// view means it’d be ambiguous with an overload in the standard library for
    /// the case of comparing a str w/ a string view. Since we’d have to provide
    /// overloads for that case anyway, just do this instead.
    [[nodiscard]] constexpr auto
    operator<=>(const std::convertible_to<basic_str> auto& other) const noexcept -> std::strong_ordering {
        return basic_str(other)._m_text <=> _m_text;
    }

    /// Implicit conversion to a string view.
    /* implicit */ operator text_type() const noexcept { return _m_text; }

private:
    // Return characters until position `n` (exclusive)
    // and remove them from the string.
    constexpr auto _m_advance(size_type n) LIBBASE_NOEXCEPT_UNLESS_TESTING -> basic_str {
        Assert(n <= size(), "Should never get an out-of-bounds index here");
        auto txt = _m_text.substr(0, n);
        _m_text.remove_prefix(n);
        return txt;
    }

    // Return characters starting from position `n` (exclusive)
    // and remove them from the string.
    //
    // Note: `skip` must be set to the number of characters we matched,
    // i.e. 1 for char_type and `_any` overloads (because we match at
    // most one character in the string view in that case), and the
    // length of the needle otherwise.
    template <bool _or_empty>
    constexpr auto _m_take_back(size_type pos, size_type skip = 1) noexcept -> basic_str {
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
    _m_take_until_cond(UnaryPredicate c) noexcept(noexcept(c(char_type{}))) -> basic_str {
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
    _m_take_while(char_type c) noexcept -> basic_str {
        return _m_take_while_cond(
            [c](char_type ch) { return ch == c; }
        );
    }

    [[nodiscard]] constexpr auto
    _m_take_while_any(basic_str chars) noexcept -> basic_str {
        return _m_take_while_cond([chars](char_type ch) {
            return chars.contains(ch);
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

using str = basic_str<char>;
using str8 = basic_str<char8_t>;
using str16 = basic_str<char16_t>;
using str32 = basic_str<char32_t>;
} // namespace base

template <>
struct std::formatter<base::str> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(base::str s, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(std::string_view(s), ctx);
    }
};

template <typename CharType>
struct std::hash<base::basic_str<CharType>> :
    std::hash<basic_string_view<CharType>> {};

template <typename CharType>
inline constexpr bool std::ranges::enable_borrowed_range<base::basic_str<CharType>> = true;

template <typename CharType>
inline constexpr bool std::ranges::enable_view<base::basic_str<CharType>> = true;

#endif // LIBBASE_STR_HH
