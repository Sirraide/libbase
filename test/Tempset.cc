#include "TestCommon.hh"

using namespace base;

TEST_CASE("tempset compound assignment ops") {
    int x = 3;

    SECTION("|=") {
        tempset x |= 4;
        CHECK(x == 7);
    }

    SECTION("&=") {
        {
            tempset x &= 4;
            CHECK(x == 0);
        }

        CHECK(x == 3);

        {
            tempset x &= 3;
            CHECK(x == 3);
        }
    }

    SECTION("^=") {
        {
            tempset x ^= 4;
            CHECK(x == 7);
        }

        CHECK(x == 3);

        {
            tempset x ^= 3;
            CHECK(x == 0);
        }
    }

    SECTION("<<=") {
        tempset x <<= 2;
        CHECK(x == 12);
    }

    SECTION(">>=") {
        tempset x >>= 1;
        CHECK(x == 1);
    }

    SECTION("+=") {
        tempset x += 4;
        CHECK(x == 7);
    }

    SECTION("-=") {
        tempset x -= 2;
        CHECK(x == 1);
    }

    SECTION("*=") {
        tempset x *= 2;
        CHECK(x == 6);
    }

    SECTION("/=") {
        tempset x /= 2;
        CHECK(x == 1);
    }

    SECTION("%=") {
        tempset x %= 2;
        CHECK(x == 1);
    }

    CHECK(x == 3);
}
