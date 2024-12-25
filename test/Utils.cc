#include "TestCommon.hh"

#include <base/Macros.hh>
#include <base/Properties.hh>
#include <deque>
#include <string>
#include <array>

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

TEST_CASE("join(): non-strings") {
    std::vector vec{1, 2, 3};
    std::deque deq{4.0, 5.0, 6.0};
    std::array arr{vec, vec, vec};

    CHECK(utils::join(vec) == "1, 2, 3");
    CHECK(utils::join(vec, ", ") == "1, 2, 3");
    CHECK(utils::join(deq, "|") == "4|5|6");
    CHECK(utils::join(arr, "+") == "[1, 2, 3]+[1, 2, 3]+[1, 2, 3]");
}

TEST_CASE("join(): format string") {
    std::vector vec{1, 2, 3};
    CHECK(utils::join(vec, ", ", "{:#x}") == "0x1, 0x2, 0x3");
}

TEST_CASE("join(): projection") {
    std::vector vec{1, 2, 3};
    CHECK(utils::join(vec, ", ", "{}", [](int i) { return i * 2; }) == "2, 4, 6");
}

TEST_CASE("join(): format string and projection") {
    std::vector vec{1, 2, 3};
    CHECK(utils::join(vec, ", ", "{:#x}", [](int i) { return i * 2; }) == "0x2, 0x4, 0x6");
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

#if LIBBASE_PROPERTIES
struct S {
    std::string _foo, _bar;
    ComputedProperty(std::string, foo);
    ComputedProperty(std::string, bar, _bar);
    ComputedReadonly(std::string, baz);
    ComputedReadonly(std::string, quux, "quux");

    Property(std::string, trivial_foo);
    Property(std::string, trivial_bar, "tri_bar");
    Readonly(std::string, trivial_ro_foo);
    Readonly(std::string, trivial_ro_bar, "trivial_bar");

    Writeonly(std::string, writeonly);

public:
    S(std::string trivial_foo) : _trivial_ro_foo{std::move(trivial_foo)} {}
};

static_assert(__is_same(decltype(std::declval<S>().get_trivial_foo()), const std::string&));
static_assert(__is_same(decltype(std::declval<S>().get_trivial_bar()), const std::string&));
static_assert(__is_same(decltype(std::declval<S>().get_trivial_ro_foo()), const std::string&));
static_assert(__is_same(decltype(std::declval<S>().get_trivial_ro_bar()), const std::string&));

auto S::get_foo() const -> std::string { return _foo; }
void S::set_foo(std::string new_value) { _foo = std::move(new_value); }
void S::set_bar(std::string new_value) { _bar = std::move(new_value); }
void S::set_trivial_bar(std::string new_value) { _trivial_bar = std::move(new_value); }
void S::set_trivial_foo(std::string new_value) { _trivial_foo = std::move(new_value); }
auto S::get_baz() const -> std::string { return "foobarbaz"; }
void S::set_writeonly(std::string new_value) {
    CHECK(new_value == "writeonly");
}

TEST_CASE("Properties work") {
    S s{"trivial_foo"};
    s._foo = "foo";
    s._bar = "bar";
    s.trivial_foo = "tri_foo";
    s.writeonly = "writeonly";

    CHECK(s.foo == "foo");
    CHECK(s.bar == "bar");
    s.foo = "bar";
    s.bar = "foo";
    CHECK(s.foo == "bar");
    CHECK(s.bar == "foo");
    CHECK(s.baz == "foobarbaz");
    CHECK(s.quux == "quux");
    CHECK(s.trivial_foo == "tri_foo");
    CHECK(s.trivial_bar == "tri_bar");
    CHECK(s.trivial_ro_foo == "trivial_foo");
    CHECK(s.trivial_ro_bar == "trivial_bar");
}

// Check that reference properties are stored as pointers.
struct Ref {
    Property(std::string&, foo);
    Property(std::string&, bar, _foo);
    Readonly(const std::string&, baz);
    Readonly(const std::string&, quux, _baz);

public:
    Ref(std::string& a, std::string& b) : _foo{&a}, _baz{&b} {}
};

static_assert(__is_same(decltype(std::declval<Ref>().get_foo()), std::string&));
static_assert(__is_same(decltype(std::declval<Ref>().get_bar()), std::string&));
static_assert(__is_same(decltype(std::declval<Ref>().get_baz()), const std::string&));
static_assert(__is_same(decltype(std::declval<Ref>().get_quux()), const std::string&));

TEST_CASE("Reference properties") {
    std::string a = "foo", b = "bar";
    Ref r{a, b};

    CHECK(r.foo == "foo");
    CHECK(r.bar == "foo");
    CHECK(r.baz == "bar");
    CHECK(r.quux == "bar");
}

// Check that trivially copyable types are returned by value, so long as they’re small.
struct Trivial {
    using Big = std::array<int, 10000>;
    Readonly(int, bar);
    Property(int, foo);
    Readonly(std::string_view, baz);
    Property(std::string_view, quux);
    Readonly(Big, big);
    Property(Big, big2);
};

static_assert(__is_same(decltype(std::declval<Trivial>().get_bar()), int));
static_assert(__is_same(decltype(std::declval<Trivial>().get_foo()), int));
static_assert(__is_same(decltype(std::declval<Trivial>().get_baz()), std::string_view));
static_assert(__is_same(decltype(std::declval<Trivial>().get_quux()), std::string_view));
static_assert(__is_same(decltype(std::declval<Trivial>().get_big()), const std::array<int, 10000>&));
static_assert(__is_same(decltype(std::declval<Trivial>().get_big2()), const std::array<int, 10000>&));
#endif
