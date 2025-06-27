#include "TestCommon.hh"
#include <base/Ref.hh>

using namespace base;

struct X : RefBase {

};

struct Y : X {
    Y(int x) : x(x) {}
    int x;
};

TEST_CASE("Ref: basic constructors") {
    SECTION("default") {
        Ref<X> a;
        CHECK(a.ptr == nullptr);
    }

    SECTION("null") {
        Ref<X> a{nullptr};
        CHECK(a.ptr == nullptr);

        Ref b{static_cast<X*>(nullptr)};
        CHECK(b.ptr == nullptr);
    }

    SECTION("pointer / copy / move") {
        auto x1 = new X;
        Ref a{x1};
        CHECK(a.ptr == x1);
        CHECK(x1->ref_count == 1);

        {
            Ref b{a};
            CHECK(b.ptr == x1);
            CHECK(x1->ref_count == 2);
        }

        CHECK(x1->ref_count == 1);

        {
            Ref b{std::move(a)};
            CHECK(a.ptr == nullptr);
            CHECK(b.ptr == x1);
            CHECK(x1->ref_count == 1);
        }
    }
}

TEST_CASE("Ref: converting constructor") {
    auto y = new Y{3};

    Ref a{y};
    Ref b{a};

    CHECK(a.ptr == y);
    CHECK(b.ptr == y);
    CHECK(y->ref_count == 2);
}

TEST_CASE("Ref: Assignment") {
    auto x1 = new X;

    Ref a{x1};

    {
        Ref<X> b;
        b = a;
        CHECK(b.ptr == a.ptr);
        CHECK(x1->ref_count == 2);
    }

    CHECK(x1->ref_count == 1);
}

TEST_CASE("Ref: swap") {
    auto x1 = new X;
    auto x2 = new X;
    Ref a{x1};
    Ref b{x2};

    swap(a, b);

    CHECK(a.ptr == x2);
    CHECK(b.ptr == x1);
    CHECK(x1->ref_count == 1);
    CHECK(x2->ref_count == 1);
}

TEST_CASE("Ref::Create()") {
    Ref a = Ref<X>::Create();
    Ref b = Ref<Y>::Create(4);
    CHECK(b->x == 4);
}

TEST_CASE("Ref: equality") {
    auto x = new X;
    auto y = new Y{4};

    Ref<X> x1{x};
    Ref<X> x2{x};
    Ref<X> x3{y};
    Ref<Y> y1{y};

    CHECK(x1 == x1);
    CHECK(y1 == y1);
    CHECK(x1 == x2);
    CHECK(x2 == x1);
    CHECK(x3 == y1);

    CHECK(x1 != x3);
    CHECK(x3 != x1);
    CHECK(x1 != y1);

    CHECK(x1 == x);
    CHECK(x2 == x);
    CHECK(x3 == y);
    CHECK(y1 == y);

    CHECK(x1 != y);
    CHECK(x2 != y);
    CHECK(x3 != x);
    CHECK(y1 != x);

    CHECK(x1 != nullptr);
    CHECK(Ref<X>() == nullptr);
}
