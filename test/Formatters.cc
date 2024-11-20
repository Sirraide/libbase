#include "TestCommon.hh"

TEST_CASE("std::vector") {
    std::vector vec {1, 2, 3, 4, 5};
    CHECK(std::format("{}", vec) == "[1, 2, 3, 4, 5]");
    CHECK(std::format("{}", std::vector<int>{}) == "[]");
}
