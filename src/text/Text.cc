#include <base/Base.hh>
#include <base/Text.hh>
#include <unicode/translit.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <functional>

using namespace base;
using namespace base::text;

/// ====================================================================
///  General Helpers
/// ====================================================================
template <typename CharTy>
[[nodiscard]] auto ExportUTF(const icu::UnicodeString& ustr) -> std::basic_string<CharTy>;

template <>
[[nodiscard]] auto ExportUTF<char>(const icu::UnicodeString& ustr) -> std::string {
    std::string res;
    ustr.toUTF8String(res);
    return res;
}

template <>
[[nodiscard]] auto ExportUTF<char32_t>(const icu::UnicodeString& ustr) -> std::u32string {
    auto Convert = [&](char32_t* out, usz n) noexcept -> usz {
        UErrorCode ec{U_ZERO_ERROR}; // Ignored.
        return usz(std::max(0, ustr.toUTF32(reinterpret_cast<UChar32*>(out), i32(n), ec)));
    };

    std::u32string res;
    res.resize_and_overwrite(sizeof(char32_t) * usz(ustr.length()), Convert);
    return res;
}

/// ====================================================================
///  C32
/// ====================================================================
auto c32::category() const -> CharCategory {
    // This conversion is actually a no-op. The values chosen
    // by us are the same ones that ICU uses, and I don’t think
    // they’re going to change.
    switch (UCharCategory(u_charType(UChar32(value)))) {
        default: return CharCategory::Unassigned;
        case U_UPPERCASE_LETTER: return CharCategory::UppercaseLetter;
        case U_LOWERCASE_LETTER: return CharCategory::LowercaseLetter;
        case U_TITLECASE_LETTER: return CharCategory::TitlecaseLetter;
        case U_MODIFIER_LETTER: return CharCategory::ModifierLetter;
        case U_OTHER_LETTER: return CharCategory::OtherLetter;
        case U_NON_SPACING_MARK: return CharCategory::NonSpacingMark;
        case U_ENCLOSING_MARK: return CharCategory::EnclosingMark;
        case U_COMBINING_SPACING_MARK: return CharCategory::CombiningSpacingMark;
        case U_DECIMAL_DIGIT_NUMBER: return CharCategory::DecimalDigitNumber;
        case U_LETTER_NUMBER: return CharCategory::LetterNumber;
        case U_OTHER_NUMBER: return CharCategory::OtherNumber;
        case U_SPACE_SEPARATOR: return CharCategory::SpaceSeparator;
        case U_LINE_SEPARATOR: return CharCategory::LineSeparator;
        case U_PARAGRAPH_SEPARATOR: return CharCategory::ParagraphSeparator;
        case U_CONTROL_CHAR: return CharCategory::ControlChar;
        case U_FORMAT_CHAR: return CharCategory::FormatChar;
        case U_PRIVATE_USE_CHAR: return CharCategory::PrivateUseChar;
        case U_SURROGATE: return CharCategory::Surrogate;
        case U_DASH_PUNCTUATION: return CharCategory::DashPunctuation;
        case U_START_PUNCTUATION: return CharCategory::StartPunctuation;
        case U_END_PUNCTUATION: return CharCategory::EndPunctuation;
        case U_CONNECTOR_PUNCTUATION: return CharCategory::ConnectorPunctuation;
        case U_OTHER_PUNCTUATION: return CharCategory::OtherPunctuation;
        case U_MATH_SYMBOL: return CharCategory::MathSymbol;
        case U_CURRENCY_SYMBOL: return CharCategory::CurrencySymbol;
        case U_MODIFIER_SYMBOL: return CharCategory::ModifierSymbol;
        case U_OTHER_SYMBOL: return CharCategory::OtherSymbol;
        case U_INITIAL_PUNCTUATION: return CharCategory::InitialPunctuation;
        case U_FINAL_PUNCTUATION: return CharCategory::FinalPunctuation;
    }
}

auto c32::name() const -> Result<std::string> {
    UErrorCode ec{U_ZERO_ERROR};
    std::array<char, 1'024> char_name{};

    auto len = u_charName(
        UChar32(value),
        U_UNICODE_CHAR_NAME,
        char_name.data(),
        i32(char_name.size()),
        &ec
    );

    if (U_FAILURE(ec)) return Error(
        "Failed to get name for codepoint U+{:04X}: {}",
        u32(value),
        u_errorName(ec)
    );

    // Treat an empty string as an error.
    if (len == 0) return Error(
        "Codepoint U+{:04X} has no name",
        u32(value)
    );

    return std::string{char_name.data(), usz(len)};
}

auto c32::swap_case() const -> c32 {
    auto cat = category();
    if (cat == CharCategory::UppercaseLetter) return to_lower();
    if (cat == CharCategory::LowercaseLetter) return to_upper();
    return *this;
}

auto c32::to_lower() const -> c32 {
    return u_tolower(UChar32(value));
}

auto c32::to_upper() const -> c32 {
    return u_toupper(UChar32(value));
}

auto c32::width() const -> unsigned {
    auto val = u_getIntPropertyValue(UChar32(value), UCHAR_EAST_ASIAN_WIDTH);
    switch (UEastAsianWidth(val)) {
        default:
            return 1;

        // Ambiguous characters are not treated as wide as per Unicode Standard Annex #11.
        case U_EA_FULLWIDTH:
        case U_EA_WIDE:
            return 2;
    }
}

/// ====================================================================
///  Miscellaneous
/// ====================================================================
auto text::FindCharsByName(
    std::function<bool(c32, std::string_view)> filter,
    c32 from,
    c32 to
) -> std::vector<c32> {
    UErrorCode ec{U_ZERO_ERROR};
    std::vector<c32> chars;
    struct Ctx {
        std::vector<c32>& chars; // Not the actual vector to support NRVO.
        decltype(filter) filter;
    } ctx{chars, std::move(filter)};

    auto Enum = []( // clang-format off
        void* context,
        UChar32 code,
        UCharNameChoice,
        const char* name,
        int32_t length
    ) -> UBool {
        auto& ctx = *static_cast<Ctx*>(context);
        if (ctx.filter(c32(code), std::string_view{name, usz(length)}))
            ctx.chars.push_back(c32(code));
        return true;
    }; // clang-format on

    u_enumCharNames(UChar32(from), UChar32(to), Enum, &ctx, U_UNICODE_CHAR_NAME, &ec);
    return chars;
}

/// ====================================================================
///  Normalisation
/// ====================================================================
namespace {
[[nodiscard]] auto GetTransliterator(
    const char* spec
) -> Result<icu::Transliterator*> {
    UErrorCode ec{U_ZERO_ERROR};
    auto trans = icu::Transliterator::createInstance(
        spec,
        UTRANS_FORWARD,
        ec
    );

    // If an NFC/NFD transliterator is not available, then that is a pretty
    // serious problem with the ICU installation; that likely means there is
    // nothing else we can do here, so just give up.

    if (U_FAILURE(ec)) return Error(
        "Failed to create transliterator: {}\n",
        u_errorName(ec)
    );

    return trans;
}

[[nodiscard]] auto GetTransliterator(
    NormalisationForm nf
) -> Result<icu::Transliterator*> {
    return GetTransliterator(nf == NormalisationForm::NFC ? "NFC" : "NFD");
}

[[nodiscard]] auto UStr(std::string_view str) -> icu::UnicodeString {
    return icu::UnicodeString::fromUTF8(icu::StringPiece(str));
}

[[nodiscard]] auto UStr(std::u32string_view str) -> icu::UnicodeString {
    return icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(str.data()), i32(str.size()));
}

template <typename CharTy>
[[nodiscard]] auto NormaliseImpl(
    std::basic_string_view<CharTy> str,
    NormalisationForm form
) -> Result<std::basic_string<CharTy>> {
    if (form == NormalisationForm::None) return std::basic_string<CharTy>{str};
    auto trans = Try(GetTransliterator(form));
    auto ustr = UStr(str);
    trans->transliterate(ustr);
    return ExportUTF<CharTy>(ustr);
}
} // namespace

auto text::Normalise(std::string_view str, NormalisationForm form) -> Result<std::string> {
    return NormaliseImpl(str, form);
}

auto text::Normalise(std::u32string_view str, NormalisationForm form) -> Result<std::u32string> {
    return NormaliseImpl(str, form);
}

/// ====================================================================
///  Case Conversion
/// ====================================================================
namespace {
template <typename CharTy>
[[nodiscard]] auto ToLowerImpl(std::basic_string_view<CharTy> str) -> std::basic_string<CharTy> {
    auto ustr = UStr(str);
    ustr.toLower();
    return ExportUTF<CharTy>(ustr);
}

template <typename CharTy>
[[nodiscard]] auto ToUpperImpl(std::basic_string_view<CharTy> str) -> std::basic_string<CharTy> {
    auto ustr = UStr(str);
    ustr.toUpper();
    return ExportUTF<CharTy>(ustr);
}
} // namespace

auto text::ToLower(std::string_view str) -> std::string {
    return ToLowerImpl(str);
}

auto text::ToLower(std::u32string_view str) -> std::u32string {
    return ToLowerImpl(str);
}

auto text::ToUpper(std::string_view str) -> std::string {
    return ToUpperImpl(str);
}

auto text::ToUpper(std::u32string_view str) -> std::u32string {
    return ToUpperImpl(str);
}

/// ====================================================================
///  8<->32 Conversion
/// ====================================================================
auto text::ToUTF32(std::string_view str) -> std::u32string {
    return ExportUTF<char32_t>(UStr(str));
}

auto text::ToUTF8(std::u32string_view str) -> std::string {
    return ExportUTF<char>(UStr(str));
}

auto text::ToUTF8(c32 c) -> std::string {
    return ExportUTF<char>(UStr(std::u32string_view{&c.value, 1}));
}
