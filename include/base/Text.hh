#ifndef LIBBASE_TEXT_HH
#define LIBBASE_TEXT_HH

#include <base/Result.hh>
#include <base/Types.hh>
#include <concepts>
#include <format>
#include <functional>
#include <string>

namespace base::text {
enum struct NormalisationForm {
    None, /// No normalisation.
    NFC,  /// Normalisation Form C.
    NFD,  /// Normalisation Form D.
    NFKC, /// Normalisation Form KC.
    NFKD, /// Normalisation Form KD.
};

enum struct CharCategory {
    Unassigned = 0,
    UppercaseLetter = 1,
    LowercaseLetter = 2,
    TitlecaseLetter = 3,
    ModifierLetter = 4,
    OtherLetter = 5,
    NonSpacingMark = 6,
    EnclosingMark = 7,
    CombiningSpacingMark = 8,
    DecimalDigitNumber = 9,
    LetterNumber = 10,
    OtherNumber = 11,
    SpaceSeparator = 12,
    LineSeparator = 13,
    ParagraphSeparator = 14,
    ControlChar = 15,
    FormatChar = 16,
    PrivateUseChar = 17,
    Surrogate = 18,
    DashPunctuation = 19,
    StartPunctuation = 20,
    EndPunctuation = 21,
    ConnectorPunctuation = 22,
    OtherPunctuation = 23,
    MathSymbol = 24,
    CurrencySymbol = 25,
    ModifierSymbol = 26,
    OtherSymbol = 27,
    InitialPunctuation = 28,
    FinalPunctuation = 29,
};

/// Unicode character with helper functions.
struct c32 {
    char32_t value;

    constexpr c32() = default;
    constexpr c32(char32_t value) : value(value) {}
    explicit constexpr c32(std::integral auto value) : value(char32_t(value)) {}

    /// Get the category of a character.
    [[nodiscard]] auto category() const -> CharCategory;

    /// Get the name of a character.
    [[nodiscard]] auto name() const -> Result<std::string>;

    /// Swap case.
    [[nodiscard]] auto swap_case() const -> c32;

    /// Convert to lower case.
    [[nodiscard]] auto to_lower() const -> c32;

    /// Convert to upper case.
    [[nodiscard]] auto to_upper() const -> c32;

    /// Get the width of this character.
    ///
    /// This is either 1 or 2. This property is only well-defined for
    /// characters that have a fixed width.
    [[nodiscard]] auto width() const -> unsigned;

    constexpr c32& operator++() {
        ++value;
        return *this;
    }

    constexpr c32 operator++(int) {
        auto tmp = *this;
        ++value;
        return tmp;
    }

    /// Convert to a char32_t.
    operator char32_t() const { return value; }

    /// Get the largest valid codepoint.
    [[nodiscard]] static constexpr auto max() -> c32 { return 0x10'FFFF; }
};

/// Find all characters whose name contains one of the given strings.
///
/// If the query is empty, the result is unspecified.
[[nodiscard]] auto FindCharsByName(
    std::function<bool(c32, std::string_view)> filter,
    c32 from = u' ',
    c32 to = c32::max()
) -> std::vector<c32>;

/// <cctype> functions.
///
/// We provide separate definition of these for several reasons. Unlike the C
/// standard library equivalents, these are constexpr, can be inlined by the
/// compiler, are not locale-dependent, and don’t experience undefined behaviour
/// if you pass in a signed char whose value is > 127.
///
/// We also provide a few more functions than cctype for convenience.
[[nodiscard]] constexpr bool IsDigit(char c) { return c >= '0' and c <= '9'; }
[[nodiscard]] constexpr bool IsLower(char c) { return c >= 'a' and c <= 'z'; }
[[nodiscard]] constexpr bool IsUpper(char c) { return c >= 'A' and c <= 'Z'; }
[[nodiscard]] constexpr bool IsAlpha(char c) { return IsLower(c) or IsUpper(c); }
[[nodiscard]] constexpr bool IsAlnum(char c) { return IsAlpha(c) or IsDigit(c); }
[[nodiscard]] constexpr bool IsSpace(char c) { return c == ' ' or c == '\t' or c == '\n' or c == '\r' or c == '\v' or c == '\f'; }
[[nodiscard]] constexpr bool IsPunct(char c) { return (c >= '!' and c <= '/') or (c >= ':' and c <= '@') or (c >= '[' and c <= '`') or (c >= '{' and c <= '~'); }
[[nodiscard]] constexpr bool IsGraph(char c) { return IsAlnum(c) or IsPunct(c); }
[[nodiscard]] constexpr bool IsPrint(char c) { return IsGraph(c) or c == ' '; }
[[nodiscard]] constexpr bool IsCntrl(char c) { return u8(c) < ' ' or c == 0x7F; }
[[nodiscard]] constexpr bool IsBlank(char c) { return c == ' ' or c == '\t'; }
[[nodiscard]] constexpr bool IsXDigit(char c) { return IsDigit(c) or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F'); }
[[nodiscard]] constexpr bool IsBinary(char c) { return c == '0' or c == '1'; }
[[nodiscard]] constexpr bool IsOctal(char c) { return c >= '0' and c <= '7'; }

/// Convert a string to a normalised form.
[[nodiscard]] auto Normalise(std::string_view str, NormalisationForm form) -> Result<std::string>;
[[nodiscard]] auto Normalise(std::u32string_view str, NormalisationForm form) -> Result<std::u32string>;

/// Convert a string to lowercase.
[[nodiscard]] auto ToLower(std::string_view str) -> std::string;
[[nodiscard]] auto ToLower(std::u32string_view str) -> std::u32string;

/// Convert a string to uppercase.
[[nodiscard]] auto ToUpper(std::string_view str) -> std::string;
[[nodiscard]] auto ToUpper(std::u32string_view str) -> std::u32string;

/// Convert UTF-8 to UTF-32.
[[nodiscard]] auto ToUTF32(std::string_view str) -> std::u32string;

/// Convert UTF-32 to UTF-8.
[[nodiscard]] auto ToUTF8(std::u32string_view str) -> std::string;
[[nodiscard]] auto ToUTF8(c32 str) -> std::string;
} // namespace base::text

namespace base {
using text::c32;
} // namespace base

template <>
struct std::formatter<base::c32> : formatter<base::u32> {
    template <typename FormatContext>
    auto format(base::c32 c, FormatContext& ctx) const {
        return formatter<base::u32>::format(base::u32(c.value), ctx);
    }
};

template <>
struct std::formatter<char32_t> : formatter<base::u32> {
    template <typename FormatContext>
    auto format(char32_t c, FormatContext& ctx) const {
        return formatter<base::u32>::format(base::u32(c), ctx);
    }
};

template <>
struct std::formatter<std::u32string_view> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(std::u32string_view s, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
            base::text::ToUTF8(s),
            ctx
        );
    }
};

template <>
struct std::formatter<std::u32string> : formatter<std::u32string_view> {
    template <typename FormatContext>
    auto format(std::u32string_view s, FormatContext& ctx) const {
        return formatter<std::u32string_view>::format(s, ctx);
    }
};

#endif // LIBBASE_TEXT_HH
