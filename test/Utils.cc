#include "TestCommon.hh"
#include <vector>
#include <deque>

import base;
using namespace base;

TEST_CASE("erase_unordered()") {
    std::vector vec{1, 2, 3, 4, 5};
    std::deque deq{1, 2, 3, 4, 5};

    utils::erase_unordered(vec, vec.begin());
    utils::erase_unordered(deq, deq.begin());
    REQUIRE(vec == std::vector{5, 2, 3, 4});
    REQUIRE(deq == std::deque{5, 2, 3, 4});

    utils::erase_unordered(vec, vec.begin() + 1);
    utils::erase_unordered(deq, deq.begin() + 1);
    REQUIRE(vec == std::vector{5, 4, 3});
    REQUIRE(deq == std::deque{5, 4, 3});

    utils::erase_unordered(vec, vec.begin() + 2);
    utils::erase_unordered(deq, deq.begin() + 2);
    REQUIRE(vec == std::vector{5, 4});
    REQUIRE(deq == std::deque{5, 4});

    utils::erase_unordered(vec, vec.begin());
    utils::erase_unordered(deq, deq.begin());
    REQUIRE(vec == std::vector{4});
    REQUIRE(deq == std::deque{4});

    utils::erase_unordered(vec, vec.begin());
    utils::erase_unordered(deq, deq.begin());
    REQUIRE(vec.empty());
    REQUIRE(deq.empty());
}

TEST_CASE("ReplaceAll") {
    std::string foo = "barbarbarbar";

    utils::ReplaceAll(foo, "x", "foo");
    CHECK(foo == "barbarbarbar");

    utils::ReplaceAll(foo, "bar", "foo");
    CHECK(foo == "foofoofoofoo");

    // Common pitfall: replacement contains the search string.
    utils::ReplaceAll(foo, "foo", "foofoo");
    CHECK(foo == "foofoofoofoofoofoofoofoo");

    utils::ReplaceAll(foo, "foo", "[foo]");
    CHECK(foo == "[foo][foo][foo][foo][foo][foo][foo][foo]");
}