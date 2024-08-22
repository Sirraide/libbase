#include "TestCommon.hh"

import base;
using namespace base;

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
