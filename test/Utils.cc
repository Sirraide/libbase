#include "TestCommon.hh"

#include <base/Macros.hh>
#include <deque>
#include <string>

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

TEST_CASE("join()") {
    std::vector vec{"a"s, "b"s, "c"s};
    std::deque deq{"a"sv, "b"sv, "c"sv};
    std::array arr{"a", "b", "c"};

    CHECK(utils::join(vec, ", ") == "a, b, c");
    CHECK(utils::join(deq, "|") == "a|b|c");
    CHECK(utils::join(arr, "") == "abc");
    CHECK(utils::join(std::span{arr}, "") == "abc");
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

struct S {
    std::string _foo, _bar;
    ComputedProperty(std::string, foo);
    ComputedProperty(std::string, bar);
    ComputedReadonly(std::string, baz);

    Property(std::string, trivial_foo);
    Property(std::string, trivial_bar, "tri_bar");
    Readonly(std::string, trivial_ro_foo);
    Readonly(std::string, trivial_ro_bar, "trivial_bar");

public:
    S(std::string trivial_foo) : _trivial_ro_foo{std::move(trivial_foo)} {}
};

auto S::get_foo() const -> std::string { return _foo; }
void S::set_foo(std::string new_value) { _foo = std::move(new_value); }
auto S::get_bar() const -> std::string { return _bar; }
void S::set_bar(std::string new_value) { _bar = std::move(new_value); }
void S::set_trivial_bar(std::string new_value) { _trivial_bar = std::move(new_value); }
void S::set_trivial_foo(std::string new_value) { _trivial_foo = std::move(new_value); }
auto S::get_baz() const -> std::string { return "foobarbaz"; }

TEST_CASE("Properties work") {
    S s{"trivial_foo"};
    s._foo = "foo";
    s._bar = "bar";
    s.trivial_foo = "tri_foo";

    CHECK(s.foo == "foo");
    CHECK(s.bar == "bar");
    s.foo = "bar";
    s.bar = "foo";
    CHECK(s.foo == "bar");
    CHECK(s.bar == "foo");
    CHECK(s.baz == "foobarbaz");
    CHECK(s.trivial_foo == "tri_foo");
    CHECK(s.trivial_bar == "tri_bar");
    CHECK(s.trivial_ro_foo == "trivial_foo");
    CHECK(s.trivial_ro_bar == "trivial_bar");
}
