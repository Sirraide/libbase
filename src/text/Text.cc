#ifdef LIBBASE_ENABLE_UNICODE_SUPPORT
#    include <base/Base.hh>
#    include <base/Text.hh>
#    include <functional>
#    include <unicode/translit.h>
#    include <unicode/uchar.h>
#    include <unicode/unistr.h>

using namespace base;
using namespace base::text;

/// ====================================================================
///  General Helpers
/// ====================================================================
namespace {
template <typename CharTy>
[[nodiscard]] auto ExportUTF(const icu::UnicodeString& ustr) -> std::basic_string<CharTy>;

template <>
[[nodiscard]] auto ExportUTF<char>(const icu::UnicodeString& ustr) -> std::string {
    std::string res;
    ustr.toUTF8String(res);
    return res;
}

template <>
[[nodiscard]] auto ExportUTF<char16_t>(const icu::UnicodeString& ustr) -> std::u16string {
    return std::u16string(ustr.getBuffer(), usz(ustr.length()));
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

[[nodiscard]] auto UStr(std::string_view str) -> icu::UnicodeString {
    return icu::UnicodeString::fromUTF8(icu::StringPiece(str));
}

[[nodiscard]] auto UStr(std::u16string_view str) -> icu::UnicodeString {
    return icu::UnicodeString(str.data(), i32(str.size()));
}

[[nodiscard]] auto UStr(std::u32string_view str) -> icu::UnicodeString {
    return icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(str.data()), i32(str.size()));
}
} // namespace
/// ====================================================================
///  C32
/// ====================================================================
auto c32::category() const noexcept -> CharCategory {
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

bool c32::is_xid_continue() const noexcept {
    return u_hasBinaryProperty(UChar32(value), UCHAR_XID_CONTINUE);
}

bool c32::is_xid_start() const noexcept {
    return u_hasBinaryProperty(UChar32(value), UCHAR_XID_START);
}

auto c32::name() const noexcept -> Result<std::string> {
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

auto c32::swap_case() const noexcept -> c32 {
    auto cat = category();
    if (cat == CharCategory::UppercaseLetter) return to_lower();
    if (cat == CharCategory::LowercaseLetter) return to_upper();
    return *this;
}

auto c32::to_lower() const noexcept -> c32 {
    return char32_t(u_tolower(UChar32(value)));
}

auto c32::to_upper() const noexcept -> c32 {
    return char32_t(u_toupper(UChar32(value)));
}

auto c32::width() const noexcept -> unsigned {
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
        decltype(filter) f;
    } ctx{chars, std::move(filter)};

    auto Enum = []( // clang-format off
        void* context,
        UChar32 code,
        UCharNameChoice,
        const char* name,
        int32_t length
    ) -> UBool {
        auto& ctx = *static_cast<Ctx*>(context);
        if (ctx.f(c32(code), std::string_view{name, usz(length)}))
            ctx.chars.push_back(c32(code));
        return true;
    }; // clang-format on

    u_enumCharNames(UChar32(from), UChar32(to), Enum, &ctx, U_UNICODE_CHAR_NAME, &ec);
    return chars;
}

/// ====================================================================
///  Transliteration and Normalisation
/// ====================================================================
namespace {
[[nodiscard]] auto GetTransliterator(NormalisationForm nf) -> Transliterator& {
    // Cache transliterators so we can call Normalise() repeatedly w/o
    // creating a new one every time.
    thread_local std::unordered_map<NormalisationForm, Transliterator> cache;
    auto it = cache.find(nf);
    if (it != cache.end()) return it->second;

    // This transliterator doesn’t exist yet (for this thread); create a new one.
    auto form = [&] -> std::string_view {
        switch (nf) {
            case NormalisationForm::NFC: return "NFC";
            case NormalisationForm::NFD: return "NFD";
            case NormalisationForm::NFKC: return "NFKC";
            case NormalisationForm::NFKD: return "NFKD";
            default: return "Any-Normalization";
        }
    }();

    // If we fail to create a transliterator for any of the normalisation forms,
    // then something is seriously wrong with our ICU installation.
    auto t = Transliterator::Create(form);
    Assert(t.has_value(), "Failed to create transliterator for normalisation form {}", form);

    // And cache it.
    cache.emplace(nf, std::move(t.value()));
    return cache.at(nf);
}

template <typename CharTy>
[[nodiscard]] auto NormaliseImpl(
    std::basic_string_view<CharTy> str,
    NormalisationForm form
) -> std::basic_string<CharTy> {
    if (form == NormalisationForm::None) return std::basic_string<CharTy>{str};
    Transliterator& trans = GetTransliterator(form);
    return trans(str);
}

template <typename Char>
auto Transliterate(
    icu::Transliterator* trans,
    std::basic_string_view<Char> str
) -> std::basic_string<Char> {
    auto u = ::UStr(str);
    trans->transliterate(u);
    return ::ExportUTF<Char>(u);
}
} // namespace

auto Transliterator::Create(std::string_view rules) -> Result<Transliterator> {
    UErrorCode ec{U_ZERO_ERROR};
    auto trans = icu::Transliterator::createInstance(
        UStr(rules),
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

    return Transliterator(trans);
}

auto Transliterator::icu_ptr() const { return static_cast<icu::Transliterator*>(impl); }
void Transliterator::destroy() { delete icu_ptr(); }

auto Transliterator::operator()(std::string_view str) const -> std::string {
    return Transliterate(icu_ptr(), str);
}

auto Transliterator::operator()(std::u16string_view str) const -> std::u16string {
    return Transliterate(icu_ptr(), str);
}

auto Transliterator::operator()(std::u32string_view str) const -> std::u32string {
    return Transliterate(icu_ptr(), str);
}

auto text::Normalise(std::string_view str, NormalisationForm form) -> std::string {
    return NormaliseImpl(str, form);
}

auto text::Normalise(std::u32string_view str, NormalisationForm form) -> std::u32string {
    return NormaliseImpl(str, form);
}

/// ====================================================================
///  Case Conversion
/// ====================================================================
namespace {
template <typename CharTy>
[[nodiscard]] auto ToLowerImpl(std::basic_string_view<CharTy> str) -> std::basic_string<CharTy> {
    auto ustr = ::UStr(str);
    ustr.toLower();
    return ::ExportUTF<CharTy>(ustr);
}

template <typename CharTy>
[[nodiscard]] auto ToUpperImpl(std::basic_string_view<CharTy> str) -> std::basic_string<CharTy> {
    auto ustr = ::UStr(str);
    ustr.toUpper();
    return ::ExportUTF<CharTy>(ustr);
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
///  Conversion
/// ====================================================================
auto text::ToUTF8(std::u16string_view str) -> std::string { return ExportUTF<char>(UStr(str)); }
auto text::ToUTF8(std::u32string_view str) -> std::string { return ExportUTF<char>(UStr(str)); }
auto text::ToUTF8(c32 c) -> std::string { return ExportUTF<char>(UStr(std::u32string_view{&c.value, 1})); }

auto text::ToUTF16(std::string_view str) -> std::u16string { return ExportUTF<char16_t>(UStr(str)); }
auto text::ToUTF16(std::u32string_view str) -> std::u16string { return ExportUTF<char16_t>(UStr(str)); }
auto text::ToUTF16(c32 c) -> std::u16string { return ExportUTF<char16_t>(UStr(std::u32string_view{&c.value, 1})); }

auto text::ToUTF32(std::string_view str) -> std::u32string { return ExportUTF<char32_t>(UStr(str)); }
auto text::ToUTF32(std::u16string_view str) -> std::u32string { return ExportUTF<char32_t>(UStr(str)); }

#endif
