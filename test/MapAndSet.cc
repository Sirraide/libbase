#include "TestCommon.hh"
#include <base/DSA.hh>

using namespace base;

TEST_CASE("StringMap: Access using different string types") {
    StringMap<int> s;
    s["foo"] = 1;
    s["bar"s] = 2;
    s["baz"sv] = 3;
    CHECK(s["foo"] == 1);
    CHECK(s["bar"s] == 2);
    CHECK(s["baz"sv] == 3);
    CHECK(s.at("foo") == 1);
    CHECK(s.at("bar"s) == 2);
    CHECK(s.at("baz"sv) == 3);
    CHECK(s.get("foo") == 1);
    CHECK(s.get("bar"s) == 2);
    CHECK(s.get("baz"sv) == 3);
    CHECK(s.get_or("foo", 0) == 1);
    CHECK(s.get_or("bar"s, 0) == 2);
    CHECK(s.get_or("baz"sv, 0) == 3);
    CHECK(s.find("foo") != s.end());
    CHECK(s.find("bar"s) != s.end());
    CHECK(s.find("baz"sv) != s.end());
}

TEST_CASE("StringSet: Access using different string types") {
    StringSet s;
    s.insert("foo");
    s.insert("bar"s);
    s.insert("baz"sv);
    CHECK(s.contains("foo"));
    CHECK(s.contains("bar"s));
    CHECK(s.contains("baz"sv));
    CHECK(s.find("foo") != s.end());
    CHECK(s.find("bar"s) != s.end());
    CHECK(s.find("baz"sv) != s.end());
}
