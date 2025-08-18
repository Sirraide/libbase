#include "TestCommon.hh"

using namespace base;

int x = 42;

auto get_int() -> Result<int&> {
    return x;
}

struct MoveOnlyType {
    LIBBASE_MOVE_ONLY(MoveOnlyType);

public:
    int x;
    MoveOnlyType(int x) : x(x) {}
};

TEST_CASE("Result<T&> is unwrapped properly") {
    auto DoIt = [] -> Result<> {
        int& y = Try(get_int());
        CHECK(y == 42);
        CHECK(Try(get_int()) == 42);
        Try(get_int()) = 43;
        CHECK(x == 43);
        return {};
    };

    DoIt().value();
}

TEST_CASE("Result<MoveOnlyType>") {
    auto CopyInit = [] -> Result<MoveOnlyType> {
        return MoveOnlyType{42};
    };

    auto NRVO = [] -> Result<MoveOnlyType> {
        MoveOnlyType x{42};
        return x;
    };

    CHECK(CopyInit().value().x == 42);
    CHECK(NRVO().value().x == 42);
}

TEST_CASE("Try() also works with std::expected") {
    using Ex = std::expected<std::string, int>;
    auto Get = [](bool ok) -> Ex {
        if (ok) return "foobar";
        return std::unexpected(42);
    };

    auto DoIt = [&](bool ok) -> Ex {
        auto val = Try(Get(ok));
        val += "baz";
        return val;
    };

    CHECK(DoIt(true).value() == "foobarbaz");
    CHECK(DoIt(false).error() == 42);
}
