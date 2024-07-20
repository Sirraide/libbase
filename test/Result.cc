#include "TestCommon.hh"
#include <base/Macros.hh>

import base;
using namespace base;

int x = 42;

auto get_int() -> Result<int&> {
    return x;
}

TEST_CASE("Result<T&> is unwrapped properly") {
    auto DoIt = [] -> Result<> {
        CHECK(Try(get_int()) == 42);
        Try(get_int()) = 43;
        CHECK(x == 43);
        return {};
    };

    DoIt().value();
}