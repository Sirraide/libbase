#ifdef LIBBASE_ENABLE_PCRE2
#    include "TestCommon.hh"
#    include <base/Regex.hh>

using namespace base;

template <>
struct Catch::StringMaker<regex_match> {
    static std::string convert(regex_match m) {
        return std::format("({}, {})", m.start, m.end);
    }
};

TEST_CASE("Error on invalid regex") {
    CHECK(regex::create("+").error().contains("quantifier does not follow a repeatable item"));
    CHECK_THROWS_WITH(regex{"+"}, ContainsSubstring("quantifier does not follow a repeatable item"));
    CHECK(regex::create("[a-").error().contains("missing terminating ] for character class"));
    CHECK_THROWS_WITH(regex{"[a-"}, ContainsSubstring("missing terminating ] for character class"));
}

TEST_CASE("Basic regex matching works") {
    regex r = "a+b+"sv;
    u32regex r32 = U"a+b+"sv;

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
    regex r = "a+b+"sv;
    u32regex r32 = U"a+b+"sv;

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

TEST_CASE("Access captures by index") {
    static constexpr std::string_view input = "aaabbcc";
    static constexpr std::u32string_view input32 = U"aaabbcc";
    regex r = "a(a+(b+))(c+)"sv;
    u32regex r32 = U"a(a+(b+))(c+)"sv;

    REQUIRE(r.match(input));
    CHECK(r[0].value() == regex_match(0, 7));
    CHECK(r[1].value() == regex_match(1, 5));
    CHECK(r[2].value() == regex_match(3, 5));
    CHECK(r[3].value() == regex_match(5, 7));
    CHECK(r[4] == std::nullopt);

    REQUIRE(r32.match(input32));
    CHECK(r32[0].value() == regex_match(0, 7));
    CHECK(r32[1].value() == regex_match(1, 5));
    CHECK(r32[2].value() == regex_match(3, 5));
    CHECK(r32[3].value() == regex_match(5, 7));
    CHECK(r32[4] == std::nullopt);
}

TEST_CASE("Access captures by name") {
    regex r = "a(?<one>a+(?<two>b+))(?<three>c+)"sv;
    u32regex r32 = U"a(?<one>a+(?<two>b+))(?<three>c+)"sv;

    REQUIRE(r.match("aaabbcc"));
    CHECK(r["one"].value() == regex_match(1, 5));
    CHECK(r["two"].value() == regex_match(3, 5));
    CHECK(r["three"].value() == regex_match(5, 7));
    CHECK(r[""] == std::nullopt);
    CHECK(r["does not exist"] == std::nullopt);

    REQUIRE(r32.match(U"aaabbcc"));
    CHECK(r32[U"one"].value() == regex_match(1, 5));
    CHECK(r32[U"two"].value() == regex_match(3, 5));
    CHECK(r32[U"three"].value() == regex_match(5, 7));
    CHECK(r32[U""] == std::nullopt);
    CHECK(r32[U"does not exist"] == std::nullopt);
}

TEST_CASE("regex_match::extract") {
    static constexpr std::string_view input = "xxaabbyy";
    regex r = "a+(b+)"sv;

    REQUIRE(r.match(input));
    CHECK(r[0]->extract(input).data() == input.substr(2, 4).data());
    CHECK(r[0]->extract(input).size() == input.substr(2, 4).size());
    CHECK(r[1]->extract(input).data() == input.substr(4, 2).data());
    CHECK(r[1]->extract(input).size() == input.substr(4, 2).size());
}

TEST_CASE("str::matches()") {
    regex r = "a+b+"sv;

    CHECK(str("ab").matches(r));
    CHECK(str("aaab").matches(r));
    CHECK(str("aaabbb").matches(r));
    CHECK(not str("a").matches(r));
    CHECK(not str("b").matches(r));
    CHECK(not str("ba").matches(r));
    CHECK(not str("").matches(r));

    CHECK(str("ab").matches("a+b+"));
    CHECK(str("aaab").matches("a+b+"));
    CHECK(str("aaabbb").matches("a+b+"));
    CHECK(not str("a").matches("a+b+"));
    CHECK(not str("b").matches("a+b+"));
    CHECK(not str("ba").matches("a+b+"));
    CHECK(not str("").matches("a+b+"));
}

TEST_CASE("str::find()") {
    regex r = "a+b+"sv;

    CHECK(str("ab").find(r) == regex_match(0, 2));
    CHECK(str("aaab").find(r) == regex_match(0, 4));
    CHECK(str("qqqaaabbb").find(r) == regex_match(3, 9));
    CHECK(str("qqqaaabbbqqabq").find(r) == regex_match(3, 9));
    CHECK(str("qabqqaaabbbqqabq").find(r) == regex_match(1, 3));
    CHECK(str("ba").find(r) == std::nullopt);
    CHECK(str("").find(r) == std::nullopt);

    CHECK(str("ab").find_regex("a+b+") == regex_match(0, 2));
    CHECK(str("aaab").find_regex("a+b+") == regex_match(0, 4));
    CHECK(str("qqqaaabbb").find_regex("a+b+") == regex_match(3, 9));
    CHECK(str("qqqaaabbbqqabq").find_regex("a+b+") == regex_match(3, 9));
    CHECK(str("qabqqaaabbbqqabq").find_regex("a+b+") == regex_match(1, 3));
    CHECK(str("ba").find_regex("a+b+") == std::nullopt);
    CHECK(str("").find_regex("a+b+") == std::nullopt);
}

TEST_CASE("str::take_until()") {
    regex r = "a+b+"sv;

    CHECK(str("ab").take_until(r) == "");
    CHECK(str("aaab").take_until(r) == "");
    CHECK(str("qqqaaabbb").take_until(r) == "qqq");
    CHECK(str("qqqaaabbbqqabq").take_until(r) == "qqq");
    CHECK(str("qqqqqabqaaabbb").take_until(r) == "qqqqq");
    CHECK(str("qabqqaaabbbqqabq").take_until(r) == "q");
    CHECK(str("ba").take_until(r) == "ba");
    CHECK(str("foo").take_until(r) == "foo");
    CHECK(str("").take_until(r) == "");

    str s{"qabqqaaabbbqqabq"};
    CHECK(s.take_until(r) == "q");
    CHECK(s == "abqqaaabbbqqabq");
    s.drop(2);
    CHECK(s.take_until(r) == "qq");
    CHECK(s == "aaabbbqqabq");
    s.drop(6);
    CHECK(s.take_until(r) == "qq");
    CHECK(s == "abq");
    s.drop(2);
    CHECK(s.take_until(r) == "q");
    CHECK(s.empty());

    CHECK(str("ab").take_until_or_empty(r) == "");
    CHECK(str("aaab").take_until_or_empty(r) == "");
    CHECK(str("qqqaaabbb").take_until_or_empty(r) == "qqq");
    CHECK(str("qqqaaabbbqqabq").take_until_or_empty(r) == "qqq");
    CHECK(str("qqqqqabqaaabbb").take_until_or_empty(r) == "qqqqq");
    CHECK(str("qabqqaaabbbqqabq").take_until_or_empty(r) == "q");
    CHECK(str("ba").take_until_or_empty(r) == "");
    CHECK(str("foo").take_until_or_empty(r) == "");
    CHECK(str("").take_until_or_empty(r) == "");
}

#endif
