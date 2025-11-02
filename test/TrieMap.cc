#include "TestCommon.hh"
#include <base/TrieMap.hh>

using namespace base;

TEST_CASE("TrieMap: Basic") {
    const TrieMap<int> t{
        {"a", 1},
        {"ab", 2},
        {"abc", 3},
    };

    str s = "aaabababcababa";
    CHECK(s.size() == 14);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.size() == 13);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.size() == 12);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 10);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 8);
    CHECK(s.match_prefix(t) == 3); REQUIRE(s.size() == 5);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 3);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 1);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.empty());
}

TEST_CASE("TrieMap: add()") {
    TrieMap<int> t;
    t.add("a", 1);
    t.add("ab", 2);
    t.add("abc", 3);

    str s = "aaabababcababa";
    CHECK(s.size() == 14);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.size() == 13);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.size() == 12);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 10);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 8);
    CHECK(s.match_prefix(t) == 3); REQUIRE(s.size() == 5);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 3);
    CHECK(s.match_prefix(t) == 2); REQUIRE(s.size() == 1);
    CHECK(s.match_prefix(t) == 1); REQUIRE(s.empty());

    str s2 = "q";
    CHECK(s2.match_prefix(t) == std::nullopt); REQUIRE(s2.size() == 1);
    t.add("q", 4);
    CHECK(s2.match_prefix(t) == 4); REQUIRE(s2.empty());
}

TEST_CASE("TrieMap: No match") {
    CHECK(str("123").match_prefix(TrieMap<int>()) == std::nullopt);
    CHECK(str("123").match_prefix(TrieMap<int>{{"23", 1}}) == std::nullopt);
    CHECK(str("123").match_prefix(TrieMap<int>{{"3", 1}}) == std::nullopt);
    CHECK(str("123").match_prefix(TrieMap<int>{{"1234", 1}}) == std::nullopt);
}

TEST_CASE("TrieMap: Empty string") {
    str s = "123";
    CHECK(s.match_prefix(TrieMap<int>{{"", 1}}) == 1);
    CHECK(s.size() == 3);
}

TEST_CASE("TrieMap: Keep track of last match") {
    str s;
    TrieMap<int> t{
        {"123", 1},
        {"12345", 2},
        {"12345678", 3},
    };

    s = "";
    CHECK(s.match_prefix(t) == std::nullopt); CHECK(s.size() == 0);

    s = "1";
    CHECK(s.match_prefix(t) == std::nullopt); CHECK(s.size() == 1);

    s = "12";
    CHECK(s.match_prefix(t) == std::nullopt); CHECK(s.size() == 2);

    s = "123";
    CHECK(s.match_prefix(t) == 1); CHECK(s.size() == 0);

    s = "1234";
    CHECK(s.match_prefix(t) == 1); CHECK(s.size() == 1);

    s = "12345";
    CHECK(s.match_prefix(t) == 2); CHECK(s.size() == 0);

    s = "123456";
    CHECK(s.match_prefix(t) == 2); CHECK(s.size() == 1);

    s = "1234567";
    CHECK(s.match_prefix(t) == 2); CHECK(s.size() == 2);

    s = "123456X";
    CHECK(s.match_prefix(t) == 2); CHECK(s.size() == 2);

    s = "12345678";
    CHECK(s.match_prefix(t) == 3); CHECK(s.size() == 0);

    s = "123456789";
    CHECK(s.match_prefix(t) == 3); CHECK(s.size() == 1);
}
