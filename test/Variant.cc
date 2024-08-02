#include "TestCommon.hh"

import base;
using namespace base;

TEST_CASE("Variant::is") {
    Variant<int, float> v = 42;
    CHECK(v.is<int>());
    CHECK(v.is<int, float>());
    CHECK(v.is<float, int>());
    CHECK(not v.is<float>());

    v = 42.0f;
    CHECK(v.is<float>());
    CHECK(v.is<int, float>());
    CHECK(v.is<float, int>());
    CHECK(not v.is<int>());

    const Variant<int, float> cv = 42;
    CHECK(cv.is<int>());
    CHECK(cv.is<int, float>());
    CHECK(cv.is<float, int>());
    CHECK(not cv.is<float>());
}

TEST_CASE("Variant::get") {
    Variant<int, float> v = 42;
    CHECK(v.get<int>() == 42);
    CHECK_THROWS(v.get<float>() == 42.0f);

    v = 43.0f;
    CHECK(v.get<float>() == 43.0f);
    CHECK_THROWS(v.get<int>() == 43);

    const Variant<int, float> cv = 42;
    CHECK(cv.get<int>() == 42);
    CHECK_THROWS(cv.get<float>() == 42.0f);

    static_assert(std::is_same_v<
        decltype(v.get<int>()),
        int&
    >);

    static_assert(std::is_same_v<
        decltype(v.get<float>()),
        float&
    >);

    static_assert(std::is_same_v<
        decltype(Variant<int, float>{}.get<int>()),
        int&&
    >);

    static_assert(std::is_same_v<
        decltype(Variant<int, float>{}.get<float>()),
        float&&
    >);
}

TEST_CASE("Variant::get_if") {
    Variant<int, float> v = 42;
    REQUIRE(v.get_if<int>() != nullptr);
    CHECK(*v.get_if<int>() == 42);
    CHECK(v.get_if<float>() == nullptr);

    v = 42.0f;
    REQUIRE(v.get_if<float>() != nullptr);
    CHECK(*v.get_if<float>() == 42.0f);
    CHECK(v.get_if<int>() == nullptr);

    const Variant<int, float> cv = 42;
    REQUIRE(cv.get_if<int>() != nullptr);
    CHECK(*cv.get_if<int>() == 42);

    static_assert(std::is_same_v<
        decltype(v.get_if<int>()),
        int*
    >);

    static_assert(std::is_same_v<
        decltype(v.get_if<float>()),
        float*
    >);
}

TEST_CASE("Variant::visit") {
    Variant<int, float> v;
    utils::Overloaded V {
        [](int& i) { i = 42; },
        [](float& f) { f = 43.f; }
    };

    v = 0;
    v.visit(V);
    CHECK(v.get<int>() == 42);

    v = 0.0f;
    v.visit(V);
    CHECK(v.get<float>() == 43.f);

    const Variant<int, float> cv = 0;
    bool visited = false;
    utils::Overloaded CV {
        [&](int) { visited = true; },
        [](float) { }
    };
    cv.visit(CV);
    CHECK(visited);
}