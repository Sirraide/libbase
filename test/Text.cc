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

template <>
struct Catch::StringMaker<char32_t> {
    static std::string convert(char32_t value) {
        return std::format("U+{:04X}", u32(value));
    }
};

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

constexpr c32 Invalid = c32(c32::max().value + 1);
constexpr c32 Chars[] {
    0,
    u'\n',
    u' ',
    u'q',
    u'3',
    u'#',
    u'Ǘ',
    u'ȝ',
    u'Þ',
    u'X',
    Invalid,
};

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
    CHECK(Chars[1].swap_case().value == u'\n');
    CHECK(Chars[2].swap_case().value == u' ');
    CHECK(Chars[3].swap_case().value == u'Q');
    CHECK(Chars[4].swap_case().value == u'3');
    CHECK(Chars[5].swap_case().value == u'#');
    CHECK(Chars[6].swap_case().value == u'ǘ');
    CHECK(Chars[7].swap_case().value == u'Ȝ');
    CHECK(Chars[8].swap_case().value == u'þ');
    CHECK(Chars[9].swap_case().value == u'x');
    CHECK(Chars[10].swap_case().value == Invalid);
}

TEST_CASE("c32::to_lower") {
    CHECK(Chars[0].to_lower().value == 0);
    CHECK(Chars[1].to_lower().value == u'\n');
    CHECK(Chars[2].to_lower().value == u' ');
    CHECK(Chars[3].to_lower().value == u'q');
    CHECK(Chars[4].to_lower().value == u'3');
    CHECK(Chars[5].to_lower().value == u'#');
    CHECK(Chars[6].to_lower().value == u'ǘ');
    CHECK(Chars[7].to_lower().value == u'ȝ');
    CHECK(Chars[8].to_lower().value == u'þ');
    CHECK(Chars[9].to_lower().value == u'x');
    CHECK(Chars[10].to_lower().value == Invalid);
}

TEST_CASE("c32::to_upper") {
    CHECK(Chars[0].to_upper().value == 0);
    CHECK(Chars[1].to_upper().value == u'\n');
    CHECK(Chars[2].to_upper().value == u' ');
    CHECK(Chars[3].to_upper().value == u'Q');
    CHECK(Chars[4].to_upper().value == u'3');
    CHECK(Chars[5].to_upper().value == u'#');
    CHECK(Chars[6].to_upper().value == u'Ǘ');
    CHECK(Chars[7].to_upper().value == u'Ȝ');
    CHECK(Chars[8].to_upper().value == u'Þ');
    CHECK(Chars[9].to_upper().value == u'X');
    CHECK(Chars[10].to_upper().value == Invalid);
}

TEST_CASE("c32::width") {
    CHECK(c32(U'x').width() == 1);
    CHECK(c32(U'3').width() == 1);
    CHECK(c32(U'桜').width() == 2);
    CHECK(c32(U'🌈').width() == 2);
}

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

TEST_CASE("FindCharsByName") {
    auto res = FindCharsByName([](c32 c, std::string_view name) {
        return std::ranges::contains(Chars, c) or (name.starts_with("LATIN") and c < 128);
    });

    CHECK(res == std::vector<c32> {
        u' ', u'#', u'3',
        u'A', u'B', u'C', u'D', u'E', u'F', u'G', u'H', u'I', u'J', u'K', u'L', u'M',
        u'N', u'O', u'P', u'Q', u'R', u'S', u'T', u'U', u'V', u'W', u'X', u'Y', u'Z',
        u'a', u'b', u'c', u'd', u'e', u'f', u'g', u'h', u'i', u'j', u'k', u'l', u'm',
        u'n', u'o', u'p', u'q', u'r', u's', u't', u'u', u'v', u'w', u'x', u'y', u'z',
        u'Þ', u'Ǘ', u'ȝ',
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

TEST_CASE("8<->32 Conversion") {
    std::string_view s1 = "áéẹḍțöźȕ";
    std::u32string_view s2 = U"áéẹḍțöźȕ";
    char32_t c = U'🌈';

    CHECK(ToUTF8(s2) == s1);
    CHECK(ToUTF32(s1) == s2);
    CHECK(ToUTF8(ToUTF32(s1)) == s1);
    CHECK(ToUTF32(ToUTF8(s2)) == s2);
    CHECK(ToUTF8(c) == "🌈"sv);
}
