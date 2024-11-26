#include "TestCommon.hh"

#include <base/DSA.hh>

using namespace base;

struct Immovable {
    LIBBASE_IMMOVABLE(Immovable);
    int sum = 0;
    Immovable() = default;
    Immovable(int x, int y) { sum = x + y; }
};

TEST_CASE("StableVector is initially empty") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    REQUIRE(s1.empty());
    REQUIRE(s2.empty());
    CHECK(s1.size() == 0);
    CHECK(s2.size() == 0);
}

TEST_CASE("StableVector: Accessing the first or last element of an empty vector asserts") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    CHECK_THROWS(s1.front());
    CHECK_THROWS(s2.front());
    CHECK_THROWS(s1.back());
    CHECK_THROWS(s2.back());
}

TEST_CASE("StableVector: Iteration") {
    StableVector<int> s1;
    StableVector<Immovable> s2;
    std::string concat;

    s1.push_back(1);
    s1.push_back(2);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);

    for (int i : s1) concat += std::to_string(i);
    CHECK(concat == "12");
    for (Immovable& i : s2) concat += std::to_string(i.sum);
    CHECK(concat == "1237");
}

TEST_CASE("StableVector::clear()") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);

    REQUIRE(not s1.empty());
    REQUIRE(not s2.empty());
    REQUIRE(s1.size() == 2);
    REQUIRE(s2.size() == 2);

    s1.clear();
    s2.clear();

    CHECK(s1.empty());
    CHECK(s2.empty());
    CHECK(s1.size() == 0);
    CHECK(s2.size() == 0);
}

TEST_CASE("StableVector::emplace_back") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.emplace_back(1);
    CHECK(not s1.empty());
    CHECK(s1.size() == 1);
    CHECK(s1.front() == 1);
    CHECK(s1.back() == 1);
    CHECK(&s1.front() == &s1.back());

    s1.emplace_back(2);
    CHECK(not s1.empty());
    CHECK(s1.size() == 2);
    CHECK(s1.front() == 1);
    CHECK(s1.back() == 2);

    s2.emplace_back(1, 2);
    CHECK(not s2.empty());
    CHECK(s2.size() == 1);
    CHECK(s2.front().sum == 3);
    CHECK(s2.back().sum == 3);
    CHECK(&s2.front() == &s2.back());

    s2.emplace_back(3, 4);
    CHECK(not s2.empty());
    CHECK(s2.size() == 2);
    CHECK(s2.front().sum == 3);
    CHECK(s2.back().sum == 7);
}

TEST_CASE("StableVector::erase_if") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s1.push_back(3);
    s1.push_back(4);
    s2.emplace_back(1, 2);
    s2.emplace_back(2, 2);
    s2.emplace_back(3, 4);
    s2.emplace_back(4, 4);

    s1.erase_if([](int i) { return i % 2 == 0; });
    s2.erase_if([](const Immovable& i) { return i.sum % 2 == 0; });

    CHECK(s1.size() == 2);
    CHECK(s2.size() == 2);
    CHECK(s1[0] == 1);
    CHECK(s1[1] == 3);
    CHECK(s2[0].sum == 3);
    CHECK(s2[1].sum == 7);
}

TEST_CASE("StableVector::index_of") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);

    // This checks for address identity, so these should fail.
    CHECK(s1.index_of(1) == std::nullopt);
    CHECK(s1.index_of(2) == std::nullopt);
    CHECK(s2.index_of(Immovable(1, 2)) == std::nullopt);
    CHECK(s2.index_of(Immovable(3, 4)) == std::nullopt);

    // These should succeed.
    CHECK(s1.index_of(s1[0]) == 0);
    CHECK(s1.index_of(s1[1]) == 1);
    CHECK(s2.index_of(s2[0]) == 0);
    CHECK(s2.index_of(s2[1]) == 1);
}

TEST_CASE("StableVector::pop_back") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);

    CHECK(s1.size() == 2);
    CHECK(s2.size() == 2);

    CHECK(*s1.pop_back() == 2);
    CHECK(s2.pop_back()->sum == 7);

    CHECK(s1.size() == 1);
    CHECK(s2.size() == 1);
    CHECK(s1[0] == 1);
    CHECK(s2[0].sum == 3);
}

TEST_CASE("StableVector::push_back") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    CHECK(not s1.empty());
    CHECK(s1.size() == 1);
    CHECK(s1[0] == 1);

    s1.push_back(std::make_unique<int>(2));
    CHECK(not s1.empty());
    CHECK(s1.size() == 2);
    CHECK(s1[0] == 1);

    s2.push_back(std::make_unique<Immovable>(1, 2));
    CHECK(not s2.empty());
    CHECK(s2.size() == 1);
    CHECK(s2[0].sum == 3);

    s2.push_back(std::make_unique<Immovable>(3, 4));
    CHECK(not s2.empty());
    CHECK(s2.size() == 2);
    CHECK(s2[0].sum == 3);
}

TEST_CASE("StableVector::swap_indices") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s1.push_back(3);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);
    s2.emplace_back(5, 6);

    s1.swap_indices(0, 2);
    s2.swap_indices(0, 2);

    REQUIRE(s1.size() == 3);
    CHECK(s1[0] == 3);
    CHECK(s1[1] == 2);
    CHECK(s1[2] == 1);

    REQUIRE(s2.size() == 3);
    CHECK(s2[0].sum == 11);
    CHECK(s2[1].sum == 7);
    CHECK(s2[2].sum == 3);

    s1.swap_indices(1, 1);
    s2.swap_indices(1, 1);

    REQUIRE(s1.size() == 3);
    CHECK(s1[0] == 3);
    CHECK(s1[1] == 2);
    CHECK(s1[2] == 1);

    REQUIRE(s2.size() == 3);
    CHECK(s2[0].sum == 11);
    CHECK(s2[1].sum == 7);
    CHECK(s2[2].sum == 3);

    CHECK_THROWS(s1.swap_indices(0, 3));
    CHECK_THROWS(s2.swap_indices(0, 3));
    CHECK_THROWS(s1.swap_indices(3, 0));
    CHECK_THROWS(s2.swap_indices(3, 0));
}

TEST_CASE("StableVector::operator[]") {
    StableVector<int> s1;
    StableVector<Immovable> s2;

    s1.push_back(1);
    s1.push_back(2);
    s2.emplace_back(1, 2);
    s2.emplace_back(3, 4);

    CHECK(s1[0lu] == 1);
    CHECK(s1[1lu] == 2);
    CHECK_THROWS(s1[2lu]);
    CHECK(s2[0lu].sum == 3);
    CHECK(s2[1lu].sum == 7);
    CHECK_THROWS(s2[2lu]);

    CHECK(s1[0l] == 1);
    CHECK(s1[1l] == 2);
    CHECK_THROWS(s1[-1l]);
    CHECK_THROWS(s1[2l]);
    CHECK(s2[0l].sum == 3);
    CHECK(s2[1l].sum == 7);
    CHECK_THROWS(s2[-1l]);
    CHECK_THROWS(s2[2l]);
}
