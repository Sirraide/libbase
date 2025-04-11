#include "TestCommon.hh"
#include <base/Colours.hh>

using namespace base::text;

auto Render(std::string_view in) -> RawString {
    return {RenderColours(true, in)};
}

struct Formatter : ColourFormatter {
    bool use_colour() const { return true; }
};

TEST_CASE("Strip formatting codes if use_colour is false") {
    CHECK(RenderColours(false, "") == "");
    CHECK(RenderColours(false, "abc") == "abc");
    CHECK(RenderColours(false, "%r(abc%)") == "abc");
    CHECK(RenderColours(false, "%b(abc%)") == "abc");
    CHECK(RenderColours(false, "%b1(a%2(b%)c%)") == "abc");
}

TEST_CASE("Escape character") {
    CHECK(RenderColours(false, "%%1(") == "%1(");
    CHECK(RenderColours(false, "%%%%1(") == "%%1(");
    CHECK(RenderColours(true, "%%1(") == "%1(");
    CHECK(RenderColours(true, "%%%%1(") == "%%1(");
}

TEST_CASE("Closing paren") {
    CHECK(RenderColours(false, "%1((a)%)") == "(a)");
    CHECK(RenderColours(false, "%1(a)b)c)%)") == "a)b)c)");
    CHECK(RenderColours(true, "%1((a)%)") == "\033[31m(a)\033[m");
    CHECK(RenderColours(true, "%1(a)b)c)%)") == "\033[31ma)b)c)\033[m");
}

TEST_CASE("Invalid formatting codes") {
    CHECK_THROWS(RenderColours(false, "%"));
    CHECK_THROWS(RenderColours(false, "%x"));
    CHECK_THROWS_WITH(RenderColours(false, "%x(%)"), ContainsSubstring("Invalid formatting character in '%x(%)': 'x'"));
    CHECK_THROWS_WITH(RenderColours(false, "%q(%)"), ContainsSubstring("Invalid formatting character in '%q(%)': 'q'"));
    CHECK_THROWS_WITH(RenderColours(false, "%1()"), ContainsSubstring("Unterminated formatting sequence in '%1()'"));
    CHECK_THROWS_WITH(RenderColours(false, "%1("), ContainsSubstring("Unterminated formatting sequence in '%1('"));

    CHECK_THROWS(RenderColours(true, "%"));
    CHECK_THROWS(RenderColours(true, "%x"));
    CHECK_THROWS_WITH(RenderColours(true, "%x(%)"), ContainsSubstring("Invalid formatting character in '%x(%)': 'x'"));
    CHECK_THROWS_WITH(RenderColours(true, "%q(%)"), ContainsSubstring("Invalid formatting character in '%q(%)': 'q'"));
    CHECK_THROWS_WITH(RenderColours(true, "%1()"), ContainsSubstring("Unterminated formatting sequence in '%1()'"));
    CHECK_THROWS_WITH(RenderColours(true, "%1("), ContainsSubstring("Unterminated formatting sequence in '%1('"));
}

TEST_CASE("Simple formatting") {
    CHECK(Render("%r(abc%)") == "abc"_raw);
    CHECK(Render("%b(abc%)") == "\033[1mabc\033[m"_raw);
    CHECK(Render("%1(abc%)") == "\033[31mabc\033[m"_raw);
    CHECK(Render("%b1(abc%)") == "\033[1;31mabc\033[m"_raw);
    CHECK(Render("%1b(abc%)") == "\033[1;31mabc\033[m"_raw);
}

TEST_CASE("Collapse openers and closers") {
    CHECK(Render("%b(%1(abc%)%)") == "\033[1;31mabc\033[m"_raw);
    CHECK(Render("%1(%b(abc%)%)") == "\033[1;31mabc\033[m"_raw);
}

TEST_CASE("Collapsing doesn’t swallow '%%' or '%)'") {
    CHECK(Render("%b(%%1(abc%)%)") == "\033[1m%1(abc\033[m%)"_raw);
    CHECK(Render("%b(%)abc") == "\033[1m\033[mabc"_raw);
}

TEST_CASE("Subsumption") {
    CHECK(Render("%b(x%1(abc%)%)") == "\033[1mx\033[31mabc\033[m"_raw);
    CHECK(Render("%1(x%b(abc%)%)") == "\033[31mx\033[1mabc\033[m"_raw);
    CHECK(Render("%b(%1(abc%)y%)") == "\033[1;31mabc\033[m\033[1my\033[m"_raw);
    CHECK(Render("%1(%b(abc%)y%)") == "\033[1;31mabc\033[m\033[31my\033[m"_raw);
    CHECK(Render("%b(x%1(abc%)y%)") == "\033[1mx\033[31mabc\033[m\033[1my\033[m"_raw);
    CHECK(Render("%1(x%b(abc%)y%)") == "\033[31mx\033[1mabc\033[m\033[31my\033[m"_raw);
    CHECK(Render("%b1(%1(abc%)y%)") == "\033[1;31mabcy\033[m"_raw);
    CHECK(Render("%b1(%b1(abc%)y%)") == "\033[1;31mabcy\033[m"_raw);
}

TEST_CASE("Underlining") {
    CHECK(Render("%u(abc%)") == "\033[4:3mabc\033[m"_raw);
    CHECK(Render("%1u(abc%)") == "\033[31;4:3mabc\033[m"_raw);
    CHECK(Render("%u1(abc%)") == "\033[4:3;58:5:1mabc\033[m"_raw);
    CHECK(Render("%1u1(abc%)") == "\033[31;4:3;58:5:1mabc\033[m"_raw);
    CHECK(Render("%1u1b(abc%)") == "\033[1;31;4:3;58:5:1mabc\033[m"_raw);
}

