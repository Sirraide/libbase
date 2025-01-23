#include "TestCommon.hh"

#include <array>
#include <base/Macros.hh>
#include <base/Properties.hh>
#include <base/Text.hh>
#include <deque>
#include <string>
#include <base/StringUtils.hh>

using namespace base;
using Catch::Matchers::RangeEquals;

TEST_CASE("Escape()") {
    // 'sv' suffix is required here because of the \0 in the middle of the string.
    static constexpr auto special = "\n\r\t\v\f\b\a\0\033"sv;

    CHECK(utils::Escape("", true) == "");
    CHECK(utils::Escape("", false) == "");
    CHECK(utils::Escape("\"", false) == "\"");
    CHECK(utils::Escape("\"", true) == "\\\"");

    CHECK(utils::Escape(special, false) == "\\n\\r\\t\\v\\f\\b\\a\\0\\e");
    CHECK(utils::Escape(special, true) == "\\n\\r\\t\\v\\f\\b\\a\\0\\e");
    CHECK(utils::Escape("\x7f", false) == "\\x7f");
    CHECK(utils::Escape("\x7f", true) == "\\x7f");

    for (char c = 0; c < 32; c++) {
        if (not text::IsPrint(c) and not special.contains(c)) {
            CHECK(utils::Escape(std::string_view{&c, 1}, false) == std::format("\\x{:02x}", c));
            CHECK(utils::Escape(std::string_view{&c, 1}, true) == std::format("\\x{:02x}", c));
        }
    }
}

TEST_CASE("escaped()") {
    using Vec = std::vector<std::string_view>;
    CHECK_THAT(utils::escaped(Vec{}, false), RangeEquals(Vec{}));
    CHECK_THAT(utils::escaped(Vec{""}, false), RangeEquals(Vec{""}));
    CHECK_THAT(utils::escaped(Vec{"a", "b", "c"}, false), RangeEquals(Vec{"a", "b", "c"}));
    CHECK_THAT(utils::escaped(Vec{"a", "b", "c"}, true), RangeEquals(Vec{"a", "b", "c"}));
    CHECK_THAT(utils::escaped(Vec{"\0"sv, "\b", "\n\r"}, false), RangeEquals(Vec{"\\0", "\\b", "\\n\\r"}));
    CHECK_THAT(utils::escaped(Vec{"\0"sv, "\b", "\n\r"}, true), RangeEquals(Vec{"\\0", "\\b", "\\n\\r"}));
    CHECK_THAT(utils::escaped(Vec{"\x7f", "\x80", "\xff"}, false), RangeEquals(Vec{"\\x7f", "\\x80", "\\xff"}));
    CHECK_THAT(utils::escaped(Vec{"\x7f", "\x80", "\xff"}, true), RangeEquals(Vec{"\\x7f", "\\x80", "\\xff"}));
    CHECK_THAT(
        utils::escaped("\n\r\t\v\f\b\a\0\033"sv | vws::transform([](char c) { return std::format("[{}]", c); }), false),
        RangeEquals(Vec{"[\\n]", "[\\r]", "[\\t]", "[\\v]", "[\\f]", "[\\b]", "[\\a]", "[\\0]", "[\\e]"})
    );
}

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

TEST_CASE("join(): member projection") {
    struct S {
        std::string text;
    };

    std::vector vec {S{"a"}, S{"b"}, S{"c"}};
    CHECK(utils::join(vec, ", ", "{}", &S::text) == "a, b, c");
}

TEST_CASE("join(): transform view of const objects") {
    struct S {
        std::string value;
    };

    const std::vector<S> vec{{"a"}, {"b"}, {"c"}};
    CHECK(utils::join(vec | vws::transform(&S::value), ", ") == "a, b, c");
}

TEST_CASE("join(): rvalues") {
    std::vector v{1, 2, 3};
    CHECK(utils::join(std::vector{1, 2, 3}) == "1, 2, 3");
    CHECK(utils::join(std::move(v)) == "1, 2, 3");
}

TEST_CASE("join(): quote_escaped() range") {
    std::vector<std::string_view> vec{"a", "b c", " d"};
    CHECK(utils::join(utils::quote_escaped(vec), "|") == "a|\"b c\"|\" d\"");
}

TEST_CASE("quoted()") {
    using Vec = std::vector<std::string_view>;

    CHECK_THAT(utils::quoted(Vec{"a", "b", "c"}), RangeEquals(Vec{"a", "b", "c"}));
    CHECK_THAT(utils::quoted(Vec{"a", "b", "c"}, true), RangeEquals(Vec{"\"a\"", "\"b\"", "\"c\""}));

    CHECK_THAT(
        utils::quoted(Vec{"a b c", "a b", "", " a b c", "a b c ", " ab", "ab "}),
        RangeEquals(Vec{"\"a b c\"", "\"a b\"", "", "\" a b c\"", "\"a b c \"", "\" ab\"", "\"ab \""})
    );

    CHECK_THAT(
        utils::quoted(Vec{"a b c", "a b", "", " a b c", "a b c ", " ab", "ab "}, true),
        RangeEquals(Vec{"\"a b c\"", "\"a b\"", "\"\"", "\" a b c\"", "\"a b c \"", "\" ab\"", "\"ab \""})
    );
}

TEST_CASE("quote_escaped()") {
    using Vec = std::vector<std::string_view>;

    CHECK_THAT(utils::quote_escaped(Vec{"a", "b", "c"}), RangeEquals(Vec{"a", "b", "c"}));
    CHECK_THAT(utils::quote_escaped(Vec{"a", "b", "c"}, true), RangeEquals(Vec{"\"a\"", "\"b\"", "\"c\""}));

    CHECK_THAT(
        utils::quote_escaped(Vec{"a b c", "a b", "", " a b c", "a b c ", " ab", "ab "}),
        RangeEquals(Vec{"\"a b c\"", "\"a b\"", "", "\" a b c\"", "\"a b c \"", "\" ab\"", "\"ab \""})
    );

    CHECK_THAT(
        utils::quote_escaped(Vec{"a b c", "a b", "", " a b c", "a b c ", " ab", "ab "}, true),
        RangeEquals(Vec{"\"a b c\"", "\"a b\"", "\"\"", "\" a b c\"", "\"a b c \"", "\" ab\"", "\"ab \""})
    );

    CHECK_THAT(
        utils::quote_escaped(Vec{"\"", "\r\n", " \r", "\n ", "\\", "\\\\", "\r", " \r"}),
        RangeEquals(Vec{"\\\"", "\\r\\n", "\" \\r\"", "\"\\n \"", "\\\\", "\\\\\\\\", "\\r", "\" \\r\""})
    );

    CHECK_THAT(
        utils::quote_escaped(Vec{"\"", "\r\n", " \r", "\n ", "\\", "\\\\", "\r", " \r"}, true),
        RangeEquals(Vec{"\"\\\"\"", "\"\\r\\n\"", "\" \\r\"", "\"\\n \"", "\"\\\\\"", "\"\\\\\\\\\"", "\"\\r\"", "\" \\r\""})
    );
}

TEST_CASE("ReplaceAll()") {
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
