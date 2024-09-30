#include "TestCommon.hh"

import base.colours;
using namespace base::text;
using Catch::Matchers::ContainsSubstring;

auto Render(std::string_view in) -> RawString {
    return {RenderColours(true, in)};
}

struct Formatter : ColourFormatter {
    bool use_colour() const { return true; }
};

TEST_CASE("Strip formatting codes if use_colour is false") {
    CHECK(RenderColours(false, "") == "");
    CHECK(RenderColours(false, "abc") == "abc");
    CHECK(RenderColours(false, "%r(abc)") == "abc");
    CHECK(RenderColours(false, "%b(abc)") == "abc");
    CHECK(RenderColours(false, "%b1(a%2(b)c)") == "abc");
}

TEST_CASE("Escape character escapes") {
    CHECK(RenderColours(false, "\033x") == "x");
    CHECK(RenderColours(false, "\033%\033)\033\033") == "%)\033");
    CHECK(RenderColours(false, "\033") == "");

    CHECK(RenderColours(true, "\033x") == "x");
    CHECK(RenderColours(true, "\033%\033)\033\033") == "%)\033");
    CHECK(RenderColours(true, "\033") == "");
}

TEST_CASE("Invalid formatting codes") {
    CHECK_THROWS(RenderColours(false, "%"));
    CHECK_THROWS(RenderColours(false, "%x"));
    CHECK_THROWS_WITH(RenderColours(false, "%x()"), ContainsSubstring("Invalid formatting character in '%x()': 'x'"));
    CHECK_THROWS_WITH(RenderColours(false, "%q()"), ContainsSubstring("Invalid formatting character in '%q()': 'q'"));
    CHECK_THROWS_WITH(RenderColours(false, "%1("), ContainsSubstring("Unterminated formatting sequence in '%1('"));

    CHECK_THROWS(RenderColours(true, "%"));
    CHECK_THROWS(RenderColours(true, "%x"));
    CHECK_THROWS_WITH(RenderColours(true, "%x()"), ContainsSubstring("Invalid formatting character in '%x()': 'x'"));
    CHECK_THROWS_WITH(RenderColours(true, "%q()"), ContainsSubstring("Invalid formatting character in '%q()': 'q'"));
    CHECK_THROWS_WITH(RenderColours(true, "%1("), ContainsSubstring("Unterminated formatting sequence in '%1('"));
}

TEST_CASE("Simple formatting") {
    CHECK(Render("%r(abc)") == "abc"_raw);
    CHECK(Render("%b(abc)") == "\033[1mabc\033[m"_raw);
    CHECK(Render("%1(abc)") == "\033[31mabc\033[m"_raw);
    CHECK(Render("%b1(abc)") == "\033[1;31mabc\033[m"_raw);
    CHECK(Render("%1b(abc)") == "\033[1;31mabc\033[m"_raw);
}

TEST_CASE("Collapse openers and closers") {
    CHECK(Render("%b(%1(abc))") == "\033[1;31mabc\033[m"_raw);
    CHECK(Render("%1(%b(abc))") == "\033[1;31mabc\033[m"_raw);
}

TEST_CASE("Subsumption") {
    CHECK(Render("%b(x%1(abc))") == "\033[1mx\033[31mabc\033[m"_raw);
    CHECK(Render("%1(x%b(abc))") == "\033[31mx\033[1mabc\033[m"_raw);
    CHECK(Render("%b(%1(abc)y)") == "\033[1;31mabc\033[m\033[1my\033[m"_raw);
    CHECK(Render("%1(%b(abc)y)") == "\033[1;31mabc\033[m\033[31my\033[m"_raw);
    CHECK(Render("%b(x%1(abc)y)") == "\033[1mx\033[31mabc\033[m\033[1my\033[m"_raw);
    CHECK(Render("%1(x%b(abc)y)") == "\033[31mx\033[1mabc\033[m\033[31my\033[m"_raw);
    CHECK(Render("%b1(%1(abc)y)") == "\033[1;31mabcy\033[m"_raw);
    CHECK(Render("%b1(%b1(abc)y)") == "\033[1;31mabcy\033[m"_raw);
}

TEST_CASE("Underlining") {
    CHECK(Render("%u(abc)") == "\033[4:3mabc\033[m"_raw);
    CHECK(Render("%1u(abc)") == "\033[31;4:3mabc\033[m"_raw);
    CHECK(Render("%u1(abc)") == "\033[4:3;58:5:1mabc\033[m"_raw);
    CHECK(Render("%1u1(abc)") == "\033[31;4:3;58:5:1mabc\033[m"_raw);
    CHECK(Render("%1u1b(abc)") == "\033[1;31;4:3;58:5:1mabc\033[m"_raw);
}

TEST_CASE("Literal text") {
    CHECK(Render("\002%aea%h5%q%qa)(%())))))))\003") == "%aea%h5%q%qa)(%())))))))"_raw);
}
