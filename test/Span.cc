#include <base/Span.hh>
#include <base/Str.hh>
#include "TestCommon.hh"

using namespace base;

template <typename T>
static auto Vec() -> T {
    T v;
    v.push_back(typename T::value_type('1'));
    v.push_back(typename T::value_type('2'));
    v.push_back(typename T::value_type('3'));
    v.push_back(typename T::value_type('4'));
    return v;
}

using Containers = utils::list<
    std::string,
    std::vector<char>,
    std::vector<signed char>,
    std::vector<unsigned char>,
    std::vector<std::byte>
>;

TEST_CASE("ByteSpan constructors") {
    ByteSpan b1{};
    CHECK(b1.empty());

    ByteSpan b2{"1234"};
    CHECK(b2.str() == str("1234"));

    ByteSpan b3{b2.data(), b2.size()};
    CHECK(b3.str() == str("1234"));

    ByteSpan b4{b2.str().data(), b2.size()};
    CHECK(b4.str() == str("1234"));

    Containers::each([]<typename T> {
        CHECK(ByteSpan(Vec<T>()).str() == str("1234"));
    });
}

TEST_CASE("MutableByteSpan constructors") {
    MutableByteSpan b1{};
    CHECK(b1.empty());

    std::string s = "1234";
    MutableByteSpan b2{s};
    CHECK(b2.str() == str("1234"));

    MutableByteSpan b3{b2.data(), b2.size()};
    CHECK(b3.str() == str("1234"));

    Containers::each([]<typename T> {
        auto v = Vec<T>();
        CHECK(MutableByteSpan(v).str() == str("1234"));
    });
}

TEST_CASE("ByteSpan: comparison") {
    ByteSpan b1{"abcd"};
    ByteSpan b2{"abce"};
    CHECK(b1 == b1);
    CHECK(b2 == b2);
    CHECK(b1 != b2);
    CHECK(b2 != b1);
    CHECK(b1 <= b1);
    CHECK(b2 <= b2);
    CHECK(b1 >= b1);
    CHECK(b2 >= b2);
    CHECK(b1 < b2);
    CHECK(b1 <= b2);
    CHECK(b2 > b1);
    CHECK(b2 >= b1);
}

TEST_CASE("MutableByteSpan: comparison") {
    std::string s1 = "abcd";
    std::string s2 = "abce";
    MutableByteSpan b1{s1};
    MutableByteSpan b2{s2};
    CHECK(b1 == b1);
    CHECK(b2 == b2);
    CHECK(b1 != b2);
    CHECK(b2 != b1);
    CHECK(b1 <= b1);
    CHECK(b2 <= b2);
    CHECK(b1 >= b1);
    CHECK(b2 >= b2);
    CHECK(b1 < b2);
    CHECK(b1 <= b2);
    CHECK(b2 > b1);
    CHECK(b2 >= b1);
}

namespace {
struct Eq { int x; friend bool operator==(Eq a, Eq b) { return a.x == b.x; } };
struct Lt { int x; friend bool operator<(Lt a, Lt b) { return a.x < b.x; } };
struct Spaceship {
    int x;
    friend auto operator<=>(Spaceship a, Spaceship b) = default;
};
}

TEST_CASE("Span: comparison") {
    Eq eq1{1};
    Eq eq2{2};
    CHECK(Span<Eq>(eq1) == Span<Eq>(eq1));
    CHECK(Span<Eq>(eq2) == Span<Eq>(eq2));
    CHECK_FALSE(Span<Eq>(eq1) == Span<Eq>(eq2));
    CHECK_FALSE(Span<Eq>(eq2) == Span<Eq>(eq1));
    CHECK_FALSE(Span<Eq>(eq1) != Span<Eq>(eq1));
    CHECK_FALSE(Span<Eq>(eq2) != Span<Eq>(eq2));
    CHECK(Span<Eq>(eq1) != Span<Eq>(eq2));
    CHECK(Span<Eq>(eq2) != Span<Eq>(eq1));

    Lt lt1{1};
    Lt lt2{2};
    CHECK_FALSE(Span<Lt>(lt1) < Span<Lt>(lt1));
    CHECK_FALSE(Span<Lt>(lt2) < Span<Lt>(lt2));
    CHECK_FALSE(Span<Lt>(lt2) < Span<Lt>(lt1));
    CHECK(Span<Lt>(lt1) < Span<Lt>(lt2));

    Spaceship b1{1};
    Spaceship b2{2};
    CHECK(Span<Spaceship>(b1) == Span<Spaceship>(b1));
    CHECK(Span<Spaceship>(b1) != Span<Spaceship>(b2));
    CHECK(Span<Spaceship>(b1) <= Span<Spaceship>(b1));
    CHECK(Span<Spaceship>(b1) >= Span<Spaceship>(b1));
    CHECK(Span<Spaceship>(b1) < Span<Spaceship>(b2));
    CHECK(Span<Spaceship>(b2) > Span<Spaceship>(b1));
    CHECK_FALSE(Span<Spaceship>(b1) == Span<Spaceship>(b2));
    CHECK_FALSE(Span<Spaceship>(b1) != Span<Spaceship>(b1));
    CHECK_FALSE(Span<Spaceship>(b2) <= Span<Spaceship>(b1));
    CHECK_FALSE(Span<Spaceship>(b1) >= Span<Spaceship>(b2));
    CHECK_FALSE(Span<Spaceship>(b2) < Span<Spaceship>(b1));
    CHECK_FALSE(Span<Spaceship>(b1) > Span<Spaceship>(b2));
}


TEST_CASE("Construct Span from std::span") {
    std::span<int> x;
    std::span<const int> x2;
    MutableSpan<int> y1 = x;
    MutableSpan<int> y2{x};
    Span<int> y3 = x;
    Span<int> y4{x};
    Span<int> y5 = x2;
    Span<int> y6{x2};
}
