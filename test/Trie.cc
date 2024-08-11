#include "TestCommon.hh"

import base.trie;
using namespace base;

Trie<std::string> t{{"foo", "bar"}};

TEST_CASE("Trie: Empty replacement.") {
    CHECK(t.replace("") == "");
}

TEST_CASE("Trie: No match") {
    CHECK(t.replace("does not match at all") == "does not match at all");
}

TEST_CASE("Trie: Simple match") {
    CHECK(t.replace("foo") == "bar");
}

TEST_CASE("Trie: Repeated match") {
    CHECK(t.replace("foofoofoo") == "barbarbar");
}

TEST_CASE("Trie: Interleaved") {
    CHECK(t.replace("fofoo") == "fobar");
    CHECK(t.replace("foofo") == "barfo");
    CHECK(t.replace("ffoo") == "fbar");
    CHECK(t.replace("ffooo") == "fbaro");
}

TEST_CASE("Trie: Recursion") {
    Trie<std::string> t{
        {"a", "aa"},
    };

    CHECK(t.replace("aa") == "aaaa");
}

TEST_CASE("Trie: Examples listed in comments") {
    Trie<std::string> a {
        {"fo", "X"},
        {"foo", "Y"},
    };

    CHECK(a.replace("foo") == "Y");
    CHECK(a.replace("fo") == "X");

    Trie<std::string> b {
        {"tree", "X"},
        {"reenact", "Y"},
    };

    CHECK(b.replace("treenact") == "Xnact");
}

TEST_CASE("Trie: Implementation supports any container") {
    Trie<std::vector<std::string>> trie;
    trie.add({"a", "b"}, {"c"});
    trie.add({"d", "e"}, {"f"});
    trie.update();

    std::vector<std::string> input = { "a", "b", "a", "d", "d", "e"};
    std::vector<std::string> expected = { "c", "a", "d", "f" };
    CHECK(trie.replace(input) == expected);
}
