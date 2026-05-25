#include "TestCommon.hh"

#include <base/Text.hh>
#include <print>
#include <string_view>

// clang-format off

using namespace base;
using namespace base::text;
using namespace std::literals;

template <>
struct Catch::StringMaker<c32> {
    static std::string convert(c32 value) {
        return std::format("U+{:04X}", value);
    }
};

#ifdef LIBBASE_ENABLE_UNICODE_SUPPORT
template <>
struct Catch::StringMaker<std::u32string> {
    static std::string convert(std::u32string_view value) {
        return std::format("{}", value);
    }
};

template <>
struct Catch::StringMaker<std::u32string_view> {
    static std::string convert(std::u32string_view value) {
        return std::format("{}", value);
    }
};
#endif

constexpr c32 Invalid = c32(c32::max().value + 1);
constexpr c32 Chars[] {
    0,
    U'\n',
    U' ',
    U'q',
    U'3',
    U'#',
    U'Ǘ',
    U'ȝ',
    U'Þ',
    U'X',
    Invalid,
};

#ifdef LIBBASE_ENABLE_UNICODE_SUPPORT
TEST_CASE("c32::category") {
    CHECK(Chars[0].category() == CharCategory::ControlChar);
    CHECK(Chars[1].category() == CharCategory::ControlChar);
    CHECK(Chars[2].category() == CharCategory::SpaceSeparator);
    CHECK(Chars[3].category() == CharCategory::LowercaseLetter);
    CHECK(Chars[4].category() == CharCategory::DecimalDigitNumber);
    CHECK(Chars[5].category() == CharCategory::OtherPunctuation);
    CHECK(Chars[6].category() == CharCategory::UppercaseLetter);
    CHECK(Chars[7].category() == CharCategory::LowercaseLetter);
    CHECK(Chars[8].category() == CharCategory::UppercaseLetter);
    CHECK(Chars[9].category() == CharCategory::UppercaseLetter);
    CHECK(Chars[10].category() == CharCategory::Unassigned);
}

TEST_CASE("c32::is_xid_continue") {
    CHECK(c32(U'x').is_xid_continue());
    CHECK(c32(U'_').is_xid_continue());
    CHECK(c32(U'1').is_xid_continue());
    CHECK(c32(U'5').is_xid_continue());
    CHECK(c32(U'δ').is_xid_continue());
    CHECK(c32(U'þ').is_xid_continue());
    CHECK(c32(U'Ε').is_xid_continue());
    CHECK(not c32(U',').is_xid_continue());
    CHECK(not c32(U'&').is_xid_continue());
}

TEST_CASE("c32::is_xid_start") {
    CHECK(c32(U'x').is_xid_start());
    CHECK(c32(U'δ').is_xid_start());
    CHECK(c32(U'þ').is_xid_start());
    CHECK(c32(U'Ε').is_xid_start());
    CHECK(not c32(U'_').is_xid_start());
    CHECK(not c32(U'1').is_xid_start());
    CHECK(not c32(U'5').is_xid_start());
    CHECK(not c32(U',').is_xid_start());
    CHECK(not c32(U'&').is_xid_start());
}

TEST_CASE("c32::name") {
    CHECK(Chars[0].name().error() == "Codepoint U+0000 has no name");
    CHECK(Chars[1].name().error() == "Codepoint U+000A has no name");
    CHECK(Chars[2].name().value() == "SPACE");
    CHECK(Chars[3].name().value() == "LATIN SMALL LETTER Q");
    CHECK(Chars[4].name().value() == "DIGIT THREE");
    CHECK(Chars[5].name().value() == "NUMBER SIGN");
    CHECK(Chars[6].name().value() == "LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE");
    CHECK(Chars[7].name().value() == "LATIN SMALL LETTER YOGH");
    CHECK(Chars[8].name().value() == "LATIN CAPITAL LETTER THORN");
    CHECK(Chars[9].name().value() == "LATIN CAPITAL LETTER X");
    CHECK(Chars[10].name().error() == "Codepoint U+110000 has no name");
}

TEST_CASE("c32::swap_case") {
    CHECK(Chars[0].swap_case().value == 0);
    CHECK(Chars[1].swap_case().value == U'\n');
    CHECK(Chars[2].swap_case().value == U' ');
    CHECK(Chars[3].swap_case().value == U'Q');
    CHECK(Chars[4].swap_case().value == U'3');
    CHECK(Chars[5].swap_case().value == U'#');
    CHECK(Chars[6].swap_case().value == U'ǘ');
    CHECK(Chars[7].swap_case().value == U'Ȝ');
    CHECK(Chars[8].swap_case().value == U'þ');
    CHECK(Chars[9].swap_case().value == U'x');
    CHECK(Chars[10].swap_case().value == Invalid);
}

TEST_CASE("c32::to_lower") {
    CHECK(Chars[0].to_lower().value == 0);
    CHECK(Chars[1].to_lower().value == U'\n');
    CHECK(Chars[2].to_lower().value == U' ');
    CHECK(Chars[3].to_lower().value == U'q');
    CHECK(Chars[4].to_lower().value == U'3');
    CHECK(Chars[5].to_lower().value == U'#');
    CHECK(Chars[6].to_lower().value == U'ǘ');
    CHECK(Chars[7].to_lower().value == U'ȝ');
    CHECK(Chars[8].to_lower().value == U'þ');
    CHECK(Chars[9].to_lower().value == U'x');
    CHECK(Chars[10].to_lower().value == Invalid);
}

TEST_CASE("c32::to_upper") {
    CHECK(Chars[0].to_upper().value == 0);
    CHECK(Chars[1].to_upper().value == U'\n');
    CHECK(Chars[2].to_upper().value == U' ');
    CHECK(Chars[3].to_upper().value == U'Q');
    CHECK(Chars[4].to_upper().value == U'3');
    CHECK(Chars[5].to_upper().value == U'#');
    CHECK(Chars[6].to_upper().value == U'Ǘ');
    CHECK(Chars[7].to_upper().value == U'Ȝ');
    CHECK(Chars[8].to_upper().value == U'Þ');
    CHECK(Chars[9].to_upper().value == U'X');
    CHECK(Chars[10].to_upper().value == Invalid);
}

TEST_CASE("c32::width") {
    CHECK(c32(U'x').width() == 1);
    CHECK(c32(U'3').width() == 1);
    CHECK(c32(U'桜').width() == 2);
    CHECK(c32(U'🌈').width() == 2);
}

TEST_CASE("FindCharsByName") {
    auto res = FindCharsByName([](c32 c, std::string_view name) {
        return std::ranges::contains(Chars, c) or (name.starts_with("LATIN") and c < 128);
    });

    CHECK(res == std::vector<c32> {
        U' ', U'#', U'3',
        U'A', U'B', U'C', U'D', U'E', U'F', U'G', U'H', U'I', U'J', U'K', U'L', U'M',
        U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X', U'Y', U'Z',
        U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k', U'l', U'm',
        U'n', U'o', U'p', U'q', U'r', U's', U't', U'u', U'v', U'w', U'x', U'y', U'z',
        U'Þ', U'Ǘ', U'ȝ',
    });
}

TEST_CASE("Normalise") {
    CHECK(Normalise( "áéẹḍțöźȕ", NormalisationForm::NFC) ==  "áéẹḍțöźȕ"sv);
    CHECK(Normalise(U"áéẹḍțöźȕ", NormalisationForm::NFC) == U"áéẹḍțöźȕ"sv);
    CHECK(Normalise( "áéẹḍțöźȕ", NormalisationForm::NFD) ==  "áéẹḍțöźȕ"sv);
    CHECK(Normalise(U"áéẹḍțöźȕ", NormalisationForm::NFD) == U"áéẹḍțöźȕ"sv);
    CHECK(Normalise(U"ﬁﬁﬁﬁ", NormalisationForm::NFKC) == U"fifififi"sv);
    CHECK(Normalise(U"ﬁﬁﬁﬁ", NormalisationForm::NFKD) == U"fifififi"sv);

    // Examples from the unicode documentation.
    CHECK(Normalise(U"\u1e9b\u0323", NormalisationForm::NFD) == U"\u017f\u0323\u0307"sv);
    CHECK(Normalise(U"\u1e9b\u0323", NormalisationForm::NFC) == U"\u1e9b\u0323"sv);
    CHECK(Normalise(U"\u1e9b\u0323", NormalisationForm::NFKD) == U"s\u0323\u0307"sv);
    CHECK(Normalise(U"\u1e9b\u0323", NormalisationForm::NFKC) == U"\u1e69"sv);
    CHECK(Normalise(U"\u212B", NormalisationForm::NFKC) == U"\u00C5"sv);
    CHECK(Normalise(U"\u212B", NormalisationForm::NFKD) == U"A\u030A"sv);
}

TEST_CASE("ToLower") {
    CHECK(ToLower( "AdðÐtÞewéÉȜ") ==  "adððtþewééȝ"sv);
    CHECK(ToLower(U"AdðÐtÞewéÉȜ") == U"adððtþewééȝ"sv);
}

TEST_CASE("ToUpper") {
    CHECK(ToUpper( "adððtþewééȝ") ==  "ADÐÐTÞEWÉÉȜ"sv);
    CHECK(ToUpper(U"adððtþewééȝ") == U"ADÐÐTÞEWÉÉȜ"sv);
}

TEST_CASE("8<->16<->32 Conversion") {
    std::string_view s1 = "áéẹḍțöźȕ";
    std::u16string_view s2 = u"áéẹḍțöźȕ";
    std::u32string_view s3 = U"áéẹḍțöźȕ";

    CHECK(ToUTF8(s2) == s1);
    CHECK(ToUTF8(s3) == s1);
    CHECK(ToUTF16(s1) == s2);
    CHECK(ToUTF16(s3) == s2);
    CHECK(ToUTF32(s1) == s3);
    CHECK(ToUTF32(s2) == s3);

    CHECK(ToUTF8(ToUTF16(s1)) == s1);
    CHECK(ToUTF8(ToUTF32(s1)) == s1);
    CHECK(ToUTF16(ToUTF8(s2)) == s2);
    CHECK(ToUTF16(ToUTF32(s2)) == s2);
    CHECK(ToUTF32(ToUTF8(s3)) == s3);
    CHECK(ToUTF32(ToUTF16(s3)) == s3);

    char32_t c = U'🌈';
    CHECK(ToUTF8(c) == "🌈"sv);
    CHECK(ToUTF16(c) == u"🌈"sv);
}

TEST_CASE("Transliterator") {
    Transliterator t("NFC; [:Punctuation:] Remove; Lower;");
    CHECK(t(".,.:AERÁ") == "aerá");
    CHECK(t(u".,.:AERÁ") == u"aerá");
    CHECK(t(U".,.:AERÁ") == U"aerá");
}
#endif

TEST_CASE("CCType functions") {
    for (u8 c = 0;; c++) {
        CHECK(text::IsAlnum(u8(c)) == !!std::isalnum(c));
        CHECK(text::IsAlpha(u8(c)) == !!std::isalpha(c));
        CHECK(text::IsBlank(u8(c)) == !!std::isblank(c));
        CHECK(text::IsCntrl(u8(c)) == !!std::iscntrl(c));
        CHECK(text::IsDigit(u8(c)) == !!std::isdigit(c));
        CHECK(text::IsGraph(u8(c)) == !!std::isgraph(c));
        CHECK(text::IsLower(u8(c)) == !!std::islower(c));
        CHECK(text::IsPrint(u8(c)) == !!std::isprint(c));
        CHECK(text::IsPunct(u8(c)) == !!std::ispunct(c));
        CHECK(text::IsSpace(u8(c)) == !!std::isspace(c));
        CHECK(text::IsUpper(u8(c)) == !!std::isupper(c));
        CHECK(text::IsXDigit(u8(c)) == !!std::isxdigit(c));
        if (c == std::numeric_limits<u8>::max()) break;
    }
}

#if defined(LIBBASE_ENABLE_UNICODE_SUPPORT) || defined(LIBBASE_ENABLE_LLVM_BINDINGS_UNICODE)
TEST_CASE("Text: ColumnWidth(UTF-8)") {
    CHECK(text::ColumnWidth("") == 0);
    CHECK(text::ColumnWidth("1234") == 4);
    CHECK(text::ColumnWidth("\033[33m1234\033[m") == 4);
    CHECK(text::ColumnWidth("x\033[33m1234\033[my") == 6);
}
#endif

TEST_CASE("Text: ColumnWidth(UTF-32)") {
    CHECK(text::ColumnWidth(U"") == 0);
    CHECK(text::ColumnWidth(U"1234") == 4);
    CHECK(text::ColumnWidth(U"\033[33m1234\033[m") == 4);
    CHECK(text::ColumnWidth(U"x\033[33m1234\033[my") == 6);
}
