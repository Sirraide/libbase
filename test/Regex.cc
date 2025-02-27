#include "TestCommon.hh"

#ifdef LIBBASE_ENABLE_PCRE2

#include <base/Regex.hh>

using namespace base;

TEST_CASE("Basic regex matching works") {
    auto r = regex::create("a+b+").value();
    auto r32 = u32regex::create(U"a+b+").value();

    CHECK(r.match("ab"));
    CHECK(r.match("aaab"));
    CHECK(r.match("aaabbb"));
    CHECK(not r.match("a"));
    CHECK(not r.match("b"));
    CHECK(not r.match("ba"));
    CHECK(not r.match(""));

    CHECK(r32.match(U"ab"));
    CHECK(r32.match(U"aaab"));
    CHECK(r32.match(U"aaabbb"));
    CHECK(not r32.match(U"a"));
    CHECK(not r32.match(U"b"));
    CHECK(not r32.match(U"ba"));
    CHECK(not r32.match(U""));
}

TEST_CASE("regex::find()") {
    auto r = regex::create("a+b+").value();
    auto r32 = u32regex::create(U"a+b+").value();

    CHECK(r.find("ab") == regex_match(0, 2));
    CHECK(r.find("aaab") == regex_match(0, 4));
    CHECK(r.find("qqqaaabbb") == regex_match(3, 9));
    CHECK(r.find("qqqaaabbbqqabq") == regex_match(3, 9));
    CHECK(r.find("qabqqaaabbbqqabq") == regex_match(1, 3));
    CHECK(r.find("ba") == std::nullopt);
    CHECK(r.find("") == std::nullopt);

    CHECK(r32.find(U"ab") == regex_match(0, 2));
    CHECK(r32.find(U"aaab") == regex_match(0, 4));
    CHECK(r32.find(U"qqqaaabbb") == regex_match(3, 9));
    CHECK(r32.find(U"qqqaaabbbqqabq") == regex_match(3, 9));
    CHECK(r32.find(U"qabqqaaabbbqqabq") == regex_match(1, 3));
    CHECK(r32.find(U"ba") == std::nullopt);
    CHECK(r32.find(U"") == std::nullopt);
}

TEST_CASE("stream::matches()") {
    auto r = regex::create("a+b+").value();

    CHECK(stream("ab").matches(r));
    CHECK(stream("aaab").matches(r));
    CHECK(stream("aaabbb").matches(r));
    CHECK(not stream("a").matches(r));
    CHECK(not stream("b").matches(r));
    CHECK(not stream("ba").matches(r));
    CHECK(not stream("").matches(r));

    CHECK(stream("ab").matches("a+b+"));
    CHECK(stream("aaab").matches("a+b+"));
    CHECK(stream("aaabbb").matches("a+b+"));
    CHECK(not stream("a").matches("a+b+"));
    CHECK(not stream("b").matches("a+b+"));
    CHECK(not stream("ba").matches("a+b+"));
    CHECK(not stream("").matches("a+b+"));
}

TEST_CASE("stream::find()") {
    auto r = regex::create("a+b+").value();

    CHECK(stream("ab").find(r) == regex_match(0, 2));
    CHECK(stream("aaab").find(r) == regex_match(0, 4));
    CHECK(stream("qqqaaabbb").find(r) == regex_match(3, 9));
    CHECK(stream("qqqaaabbbqqabq").find(r) == regex_match(3, 9));
    CHECK(stream("qabqqaaabbbqqabq").find(r) == regex_match(1, 3));
    CHECK(stream("ba").find(r) == std::nullopt);
    CHECK(stream("").find(r) == std::nullopt);

    CHECK(stream("ab").find("a+b+") == regex_match(0, 2));
    CHECK(stream("aaab").find("a+b+") == regex_match(0, 4));
    CHECK(stream("qqqaaabbb").find("a+b+") == regex_match(3, 9));
    CHECK(stream("qqqaaabbbqqabq").find("a+b+") == regex_match(3, 9));
    CHECK(stream("qabqqaaabbbqqabq").find("a+b+") == regex_match(1, 3));
    CHECK(stream("ba").find("a+b+") == std::nullopt);
    CHECK(stream("").find("a+b+") == std::nullopt);
}

#endif
