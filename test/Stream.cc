#include "TestCommon.hh"

import base;
using namespace base;

using Catch::Matchers::RangeEquals;

struct StringWrapper {
    std::string data;
    void operator +=(std::string_view s) { data += s; }
    bool operator==(std::string_view s) const { return data == s; }
};

TEST_CASE("stream::remove()") {
    std::string s = "hello world";

    CHECK(stream{s}.remove('l') == "heo word");
    CHECK(stream{s}.remove_all("l") == "heo word");
    CHECK(stream{s}.remove_all("lo") == "he wrd");
    CHECK(stream{s}.remove_all("ol") == "he wrd");
    CHECK(stream{s}.remove('x') == "hello world");
    CHECK(stream{s}.remove_all("xyz") == "hello world");

    CHECK(stream{s}.remove<StringWrapper>('l') == "heo word");
    CHECK(stream{s}.remove_all<StringWrapper>("l") == "heo word");
    CHECK(stream{s}.remove_all<StringWrapper>("lo") == "he wrd");
    CHECK(stream{s}.remove_all<StringWrapper>("ol") == "he wrd");
    CHECK(stream{s}.remove<StringWrapper>('x') == "hello world");
    CHECK(stream{s}.remove_all<StringWrapper>("xyz") == "hello world");
}

TEST_CASE("stream::split()") {
    std::string s = "aa\nbb\ncc\nbb\ncc";

    SECTION ("1") {
        auto lines = stream{s}.split("\n");
        REQUIRE(rgs::distance(lines) == 5);
        CHECK_THAT(lines, RangeEquals(std::vector{"aa"sv, "bb"sv, "cc"sv, "bb"sv, "cc"sv}));
    }

    SECTION ("2") {
        auto bb = stream{s}.split("bb");
        REQUIRE(rgs::distance(bb) == 3);
        CHECK_THAT(bb, RangeEquals(std::vector{"aa\n"sv, "\ncc\n"sv, "\ncc"sv}));
    }

    SECTION ("3") {
        auto cc = stream{s}.split("cc");
        REQUIRE(rgs::distance(cc) == 3);
        CHECK_THAT(cc, RangeEquals(std::vector{"aa\nbb\n"sv, "\nbb\n"sv, ""sv}));
    }
}
