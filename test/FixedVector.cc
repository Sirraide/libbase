#include <base/FixedVector.hh>
#include "TestCommon.hh"

using namespace base;

namespace base {
template class FixedVector<int, 10>;
}

TEST_CASE("FixedVector: Basic construction") {
    FixedVector<int, 10> v1;
    FixedVector v2{v1};
    FixedVector v3{std::move(v1)};
    FixedVector<int, 10> v4{1, 2, 3};
    v1 = v2;
    (void) v1;
    v1 = std::move(v2);
    (void) v1;
}

TEST_CASE("FixedVector: Basic operations") {
    FixedVector<int, 10> v;
    static_assert(std::is_same_v<decltype(v._m_sz), u8>);
    static_assert(v.capacity() == 10);

    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == 10);
    REQUIRE(v.empty());

    v.push_back(1);
    REQUIRE(v.size() == 1);
    CHECK(v.front() == 1);
    CHECK(v[0] == 1);
    CHECK(v.back() == 1);

    v.push_back(2);
    REQUIRE(v.size() == 2);
    CHECK(v.front() == 1);
    CHECK(v.back() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);

    auto it = v.begin();
    REQUIRE(it != v.end());
    CHECK(*it == 1);

    ++it;
    REQUIRE(it != v.end());
    CHECK(*it == 2);

    ++it;
    REQUIRE(it == v.end());

    v.emplace_back(3);
    REQUIRE(v.size() == 3);
    CHECK(v.back() == 3);

    v.erase(v.begin() + 1);
    CHECK(v == FixedVector<int, 10>{1, 3});

    v.erase(v.end() - 1);
    CHECK(v == FixedVector<int, 10>{1});

    v.erase(v.begin());
    CHECK(v.empty());

    v = FixedVector<int, 10>{1, 2, 3, 4, 5};
    CHECK(v == FixedVector<int, 10>{1, 2, 3, 4, 5});

    v.erase(v.begin() + 1, v.begin() + 4);
    CHECK(v == FixedVector<int, 10>{1, 5});

    v.erase(v.end() - 1, v.end());
    CHECK(v == FixedVector<int, 10>{1});

    v = FixedVector<int, 10>{1, 2, 3, 4, 5};
    v.erase(v.begin(), v.end());
    CHECK(v.empty());

    v.insert(v.begin(), 4);
    REQUIRE(v.size() == 1);
    CHECK(v.front() == 4);

    v.insert(v.begin(), 5);
    CHECK(v == FixedVector<int, 10>{5, 4});

    v.insert(v.end(), 6);
    CHECK(v == FixedVector<int, 10>{5, 4, 6});

    v.insert(v.begin() + 2, 7);
    CHECK(v == FixedVector<int, 10>{5, 4, 7, 6});

    CHECK(v.data() == reinterpret_cast<int*>(v._m_data));
    CHECK(v.size() == usz(v._m_sz));

    erase_if(v, [](int i) { return i >= 6; });
    CHECK(v == FixedVector<int, 10>{5, 4});

    v.push_back(8);
    v.push_back(9);
    CHECK(v == FixedVector<int, 10>{5, 4, 8, 9});

    v.pop_back();
    CHECK(v == FixedVector<int, 10>{5, 4, 8});

    v.pop_back();
    CHECK(v == FixedVector<int, 10>{5, 4});

    v.clear();
    CHECK(v.size() == 0);
}

#define CHECK_LOG(...) CHECK(ctor_log == __VA_ARGS__); ctor_log.clear()
#define CTOR "I"
#define COPY "C"
#define MOVE "M"
#define COPY_ASS "c"
#define MOVE_ASS "m"
#define DTOR "D"

std::string ctor_log;
struct S {
    int i = 0;
    S() { ctor_log += CTOR; }
    S(int i) : i{i} { ctor_log += CTOR; }
    S(const S& s) : i{s.i} { ctor_log += COPY; }
    S(S&& s) : i{std::exchange(s.i, -1)} { ctor_log += MOVE; }
    S& operator=(const S& s) { i = s.i; ctor_log += COPY_ASS; return *this; }
    S& operator=(S&& s) { i = std::exchange(s.i, -1); ctor_log += MOVE_ASS; return *this; }
    ~S() { i = -2; ctor_log += DTOR; }
    bool operator==(const S& s) const = default;
};

struct InputIterator {
    using difference_type = ptrdiff_t;
    using value_type = S;
    mutable int i = 0;
    S operator*() const { return S(++i); }
    InputIterator& operator++() { return *this; }
    void operator++(int) {}
    bool operator!=(std::default_sentinel_t) const { return i != 5; }
};

template <>
struct Catch::StringMaker<S> {
    static std::string convert(const S& s) {
        return std::to_string(s.i);
    }
};

TEST_CASE("Declaring a vector does not construct any objects") {
    ctor_log.clear();

    {
        FixedVector<S, 10> v;
        CHECK(ctor_log.empty());
    }

    CHECK(ctor_log.empty());
}

TEST_CASE("Copy ctor/ass") {
    ctor_log.clear();
    FixedVector<S, 10> v1;
    FixedVector<S, 10> v2{S(), S(), S()};
    CHECK_LOG(
        CTOR CTOR CTOR
        COPY COPY COPY
        DTOR DTOR DTOR
    );

    FixedVector v3{v1};
    CHECK_LOG("");

    FixedVector v4{v2};
    CHECK_LOG(COPY COPY COPY);

    v2 = v2;
    CHECK_LOG("");

    v3 = v2;
    CHECK_LOG(COPY COPY COPY);

    v4 = v2;
    CHECK_LOG(COPY_ASS COPY_ASS COPY_ASS);
}

TEST_CASE("Move ctor/ass") {
    FixedVector<S, 10> v1{S(), S(), S()};
    ctor_log.clear();

    FixedVector v2{std::move(v1)};
    CHECK_LOG(
        MOVE MOVE MOVE
        DTOR DTOR DTOR
    );
    CHECK(v1.empty());

    v2 = std::move(v2);
    CHECK_LOG("");
    CHECK(v2.size() == 3);

    FixedVector<S, 10> v3;
    v3 = std::move(v2);
    CHECK_LOG(
        MOVE MOVE MOVE
        DTOR DTOR DTOR
    );
    CHECK(v2.empty());
    CHECK(v3.size() == 3);

    v2 = v3;
    CHECK_LOG(COPY COPY COPY);

    v2 = std::move(v3);
    CHECK_LOG(
        MOVE_ASS MOVE_ASS MOVE_ASS
        DTOR DTOR DTOR
    );
}

TEST_CASE("Copy/move ass: mismatched length") {
    FixedVector<S, 10> v1{1, 2, 3, 4};
    FixedVector<S, 10> v2{5, 6, 7, 8, 9, 10};

    SECTION("copy: target is shorter") {
        ctor_log.clear();
        v1 = v2;
        CHECK_LOG(
            COPY_ASS COPY_ASS COPY_ASS COPY_ASS
            COPY COPY
        );

        CHECK(v1 == FixedVector<S, 10>{5, 6, 7, 8, 9, 10});
        CHECK(v2 == FixedVector<S, 10>{5, 6, 7, 8, 9, 10});
    }

    SECTION("move: target is shorter") {
        ctor_log.clear();
        v1 = std::move(v2);
        CHECK_LOG(
            MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS
            MOVE MOVE
            DTOR DTOR DTOR DTOR DTOR DTOR
        );

        CHECK(v1 == FixedVector<S, 10>{5, 6, 7, 8, 9, 10});
        CHECK(v2.empty());
    }

    SECTION("copy: target is longer") {
        ctor_log.clear();
        v2 = v1;
        CHECK_LOG(
            COPY_ASS COPY_ASS COPY_ASS COPY_ASS
            DTOR DTOR
        );

        CHECK(v1 == FixedVector<S, 10>{1, 2, 3, 4});
        CHECK(v2 == FixedVector<S, 10>{1, 2, 3, 4});
    }

    SECTION("move: target is longer") {
        ctor_log.clear();
        v2 = std::move(v1);
        CHECK_LOG(
            MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS
            DTOR DTOR
            DTOR DTOR DTOR DTOR
        );

        CHECK(v1.empty());
        CHECK(v2 == FixedVector<S, 10>{1, 2, 3, 4});
    }
}

TEST_CASE("Construction from range") {
    ctor_log.clear();
    InputIterator it;

    // Construction from an input iterator.
    FixedVector<S, 10> v{it, std::default_sentinel};
    CHECK_LOG(CTOR CTOR CTOR CTOR CTOR);

    // Construction from a random-access iterator.
    FixedVector<S, 10> v2{v.begin(), v.end()};
    CHECK_LOG(COPY COPY COPY COPY COPY);
}

TEST_CASE("Destructor") {
    {
        FixedVector<S, 10> v{1, 2, 3, 4, 5};
        ctor_log.clear();
    }

    CHECK_LOG(DTOR DTOR DTOR DTOR DTOR);
}

TEST_CASE("clear() deletes elements") {
    FixedVector<S, 10> v{S(), S(), S()};
    ctor_log.clear();
    v.clear();
    CHECK_LOG(DTOR DTOR DTOR);
    CHECK(v.empty());

    v.clear();
    CHECK_LOG("");
}

TEST_CASE("insert() moves elements properly") {
    FixedVector<S, 10> v{1, 2, 3, 4, 5};
    ctor_log.clear();

    SECTION("copy") {
        S x{42};
        ctor_log.clear();

        v.insert(v.begin() + 2, x);
        CHECK_LOG(
            MOVE MOVE_ASS MOVE_ASS
            COPY_ASS
        );
        CHECK(v == FixedVector<S, 10>{1, 2, 42, 3, 4, 5});

        ctor_log.clear();
        x.i = 43;
        v.insert(v.end(), x);
        CHECK_LOG(COPY);
        CHECK(v == FixedVector<S, 10>{1, 2, 42, 3, 4, 5, 43});

        ctor_log.clear();
        x.i = 44;
        v.insert(v.begin(), x);
        CHECK_LOG(
            MOVE MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS
            COPY_ASS
        );
        CHECK(v == FixedVector<S, 10>{44, 1, 2, 42, 3, 4, 5, 43});
    }

    SECTION("move") {
        ctor_log.clear();
        v.insert(v.begin() + 2, S(42));
        CHECK_LOG(
            CTOR
            MOVE MOVE_ASS MOVE_ASS
            MOVE_ASS
            DTOR
        );
        CHECK(v == FixedVector<S, 10>{1, 2, 42, 3, 4, 5});

        ctor_log.clear();
        v.insert(v.end(), S(43));
        CHECK_LOG(CTOR MOVE DTOR);
        CHECK(v == FixedVector<S, 10>{1, 2, 42, 3, 4, 5, 43});

        ctor_log.clear();
        v.insert(v.begin(), S(44));
        CHECK_LOG(
            CTOR
            MOVE MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS
            MOVE_ASS
            DTOR
        );
        CHECK(v == FixedVector<S, 10>{44, 1, 2, 42, 3, 4, 5, 43});
    }

    SECTION("empty vector") {
        FixedVector<S, 10> v1{};
        S x;
        ctor_log.clear();

        v1.insert(v1.end(), S());
        CHECK_LOG(CTOR MOVE DTOR);

        v1.clear();
        CHECK_LOG(DTOR);

        v1.insert(v1.end(), x);
        CHECK_LOG(COPY);
    }
}

TEST_CASE("pop_back() destroys elements") {
    FixedVector<S, 10> v{S(), S(), S()};
    ctor_log.clear();

    v.pop_back();
    CHECK_LOG(DTOR);

    v.pop_back();
    CHECK_LOG(DTOR);

    v.pop_back();
    CHECK_LOG(DTOR);

    CHECK(v.empty());
}

TEST_CASE("Emplace and push back") {
    FixedVector<S, 10> v;
    S x;
    ctor_log.clear();

    v.emplace_back();
    CHECK_LOG(CTOR);

    v.emplace_back(x);
    CHECK_LOG(COPY);

    v.emplace_back(S());
    CHECK_LOG(CTOR MOVE DTOR);

    v.push_back(S());
    CHECK_LOG(CTOR MOVE DTOR);

    v.push_back(x);
    CHECK_LOG(COPY);
}

TEST_CASE("erase()") {
    FixedVector<S, 10> v{1, 2, 3, 4, 5};

    ctor_log.clear();
    v.erase(v.begin() + 2);
    CHECK_LOG(
        MOVE_ASS MOVE_ASS
        DTOR
    );
    CHECK(v == FixedVector<S, 10>{1, 2, 4, 5});

    ctor_log.clear();
    v.erase(v.begin() + 1, v.begin() + 3);
    CHECK_LOG(
        MOVE_ASS
        DTOR DTOR
    );
    CHECK(v == FixedVector<S, 10>{1, 5});

    v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ctor_log.clear();
    v.erase(v.begin() + 2, v.begin() + 6);
    CHECK_LOG(
        MOVE_ASS MOVE_ASS MOVE_ASS MOVE_ASS
        DTOR DTOR DTOR DTOR
    );
    CHECK(v == FixedVector<S, 10>{1, 2, 7, 8, 9, 10});
}

TEST_CASE("errors") {
    FixedVector<S, 5> v{1, 2, 3, 4, 5};
    CHECK_THROWS(v.emplace_back());
    CHECK_THROWS(v.emplace_back(3));
    CHECK_THROWS(v.emplace_back(v[1]));
    CHECK_THROWS(v.push_back(4));
    CHECK_THROWS(v == FixedVector<S, 5>{1, 2, 3, 4, 5, 6});
    CHECK_THROWS(v[-1]);
    CHECK_THROWS(v[5]);
    CHECK_THROWS(v[4 | (1 << 20)]); // Check we don’t truncate the index to the stored size type.
    CHECK_THROWS(v.insert(v.begin(), 3));

    v.clear();
    CHECK_THROWS(v.front());
    CHECK_THROWS(v.back());
    CHECK_THROWS(v[0]);
    CHECK_THROWS(v.pop_back());
}

TEST_CASE("FixedVector of non-copyable type") {
    struct NoCopy {
        std::string s;
        LIBBASE_MOVE_ONLY(NoCopy);
    public:
        NoCopy() {}
    };

    // Just check that these compile.
    FixedVector<NoCopy, 5> s;
    s.emplace_back();
    s.pop_back();
    s.emplace_back();
    s.push_back(NoCopy());
    s.clear();
    s.insert(s.begin(), NoCopy());
    s.erase(s.begin());
    s.emplace_back();

    FixedVector<NoCopy, 5> s2;
    s2 = std::move(s);
    FixedVector s3{std::move(s2)};
}

TEST_CASE("Fixed vector of immovable type") {
    struct NoMove {
        const std::string s;
        LIBBASE_IMMOVABLE(NoMove);
        NoMove() {}
        NoMove(std::string s) : s{std::move(s)} {}
    };

    // Just check that these compile.
    FixedVector<NoMove, 5> s;
    s.emplace_back();
    s.emplace_back("");
    s.clear();
}
