#include "TestCommon.hh"

#include <base/Serialisation.hh>

using namespace base;

enum class u8enum : u8 {};
enum class u16enum : u16 {};
enum class u32enum : u32 {};
enum class u64enum : u64 {};

using ByteBuffer = std::vector<std::byte>;

template <std::integral ...T>
auto Bytes(T... vals) -> ByteBuffer {
    return {std::byte(vals)...};
}

template <typename T>
auto SerialiseBE(const T& t) -> ByteBuffer {
    return ser::Serialise<std::endian::big>(t);
}

template <typename T>
auto SerialiseLE(const T& t) -> ByteBuffer {
    return ser::Serialise<std::endian::little>(t);
}

TEST_CASE("Serialisation: Zero integer") {
    CHECK(SerialiseBE(u8(0)) == Bytes(0));
    CHECK(SerialiseLE(u8(0)) == Bytes(0));
    CHECK(SerialiseBE(u16(0)) == Bytes(0, 0));
    CHECK(SerialiseLE(u16(0)) == Bytes(0, 0));
    CHECK(SerialiseBE(u32(0)) == Bytes(0, 0, 0, 0));
    CHECK(SerialiseLE(u32(0)) == Bytes(0, 0, 0, 0));
    CHECK(SerialiseBE(u64(0)) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(u64(0)) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
}

TEST_CASE("Serialisation: Integers") {
    // u8
    CHECK(SerialiseBE(char(47)) == Bytes(47));
    CHECK(SerialiseLE(char(47)) == Bytes(47));
    CHECK(SerialiseBE(char8_t(47)) == Bytes(47));
    CHECK(SerialiseLE(char8_t(47)) == Bytes(47));
    CHECK(SerialiseBE(u8(47)) == Bytes(47));
    CHECK(SerialiseLE(u8(47)) == Bytes(47));
    CHECK(SerialiseBE(std::byte(47)) == Bytes(47));
    CHECK(SerialiseLE(std::byte(47)) == Bytes(47));

    // u16
    CHECK(SerialiseBE(char16_t(0x1234)) == Bytes(0x12, 0x34));
    CHECK(SerialiseLE(char16_t(0x1234)) == Bytes(0x34, 0x12));
    CHECK(SerialiseBE(i16(0x1234)) == Bytes(0x12, 0x34));
    CHECK(SerialiseLE(i16(0x1234)) == Bytes(0x34, 0x12));
    CHECK(SerialiseBE(u16(0x1234)) == Bytes(0x12, 0x34));
    CHECK(SerialiseLE(u16(0x1234)) == Bytes(0x34, 0x12));

    // u32
    CHECK(SerialiseBE(char32_t(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(char32_t(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(i32(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(i32(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u32(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(u32(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));

    // u64
    CHECK(SerialiseBE(i64(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(i64(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u64(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(u64(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
}

TEST_CASE("Serialisation: Enums") {
    CHECK(SerialiseBE(u8enum(0x12)) == Bytes(0x12));
    CHECK(SerialiseLE(u8enum(0x12)) == Bytes(0x12));
    CHECK(SerialiseBE(u16enum(0x1234)) == Bytes(0x12, 0x34));
    CHECK(SerialiseLE(u16enum(0x1234)) == Bytes(0x34, 0x12));
    CHECK(SerialiseBE(u32enum(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(u32enum(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u64enum(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(u64enum(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
}

TEST_CASE("Serialisation: Floats") {
    SECTION("Simple values") {
        CHECK(SerialiseBE(3.14f) == Bytes(0x40, 0x48, 0xF5, 0xC3));
        CHECK(SerialiseLE(3.14f) == Bytes(0xC3, 0xF5, 0x48, 0x40));
        CHECK(SerialiseBE(3.14) == Bytes(0x40, 0x09, 0x1e, 0xb8, 0x51, 0xeb, 0x85, 0x1f));
        CHECK(SerialiseLE(3.14) == Bytes(0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x09, 0x40));
    }

    SECTION("Zero") {
        CHECK(SerialiseBE(0.0f) == Bytes(0, 0, 0, 0));
        CHECK(SerialiseLE(0.0f) == Bytes(0, 0, 0, 0));
        CHECK(SerialiseBE(0.0) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(0.0) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));

        // If signed zeroes are enabled, also check for those.
        if (std::bit_cast<u32>(0.f) != std::bit_cast<u32>(-0.f)) {
            CHECK(SerialiseBE(-0.0f) == Bytes(0x80, 0, 0, 0));
            CHECK(SerialiseLE(-0.0f) == Bytes(0, 0, 0, 0x80));
            CHECK(SerialiseBE(-0.0) == Bytes(0x80, 0, 0, 0, 0, 0, 0, 0));
            CHECK(SerialiseLE(-0.0) == Bytes(0, 0, 0, 0, 0, 0, 0, 0x80));
        }
    }

    SECTION("+Inf") {
        CHECK(SerialiseBE(std::numeric_limits<float>::infinity()) == Bytes(0x7F, 0x80, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<float>::infinity()) == Bytes(0, 0, 0x80, 0x7F));
        CHECK(SerialiseBE(std::numeric_limits<double>::infinity()) == Bytes(0x7F, 0xF0, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<double>::infinity()) == Bytes(0, 0, 0, 0, 0, 0, 0xF0, 0x7F));
    }

    SECTION("-Inf") {
        CHECK(SerialiseBE(-std::numeric_limits<float>::infinity()) == Bytes(0xFF, 0x80, 0, 0));
        CHECK(SerialiseLE(-std::numeric_limits<float>::infinity()) == Bytes(0, 0, 0x80, 0xFF));
        CHECK(SerialiseBE(-std::numeric_limits<double>::infinity()) == Bytes(0xFF, 0xF0, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(-std::numeric_limits<double>::infinity()) == Bytes(0, 0, 0, 0, 0, 0, 0xF0, 0xFF));
    }

    SECTION("NaN") {
        CHECK(SerialiseBE(std::numeric_limits<float>::quiet_NaN()) == Bytes(0x7F, 0xC0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<float>::quiet_NaN()) == Bytes(0, 0, 0xC0, 0x7F));
        CHECK(SerialiseBE(std::numeric_limits<double>::quiet_NaN()) == Bytes(0x7F, 0xF8, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<double>::quiet_NaN()) == Bytes(0, 0, 0, 0, 0, 0, 0xF8, 0x7F));
    }

    SECTION("SNaN") {
        CHECK(SerialiseBE(std::numeric_limits<float>::signaling_NaN()) == Bytes(0x7F, 0xA0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<float>::signaling_NaN()) == Bytes(0, 0, 0xA0, 0x7F));
        CHECK(SerialiseBE(std::numeric_limits<double>::signaling_NaN()) == Bytes(0x7F, 0xF4, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<double>::signaling_NaN()) == Bytes(0, 0, 0, 0, 0, 0, 0xF4, 0x7F));
    }
}

TEST_CASE("Serialisation: std::string") {
    CHECK(SerialiseBE(""s) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(""s) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));

    CHECK(SerialiseBE("x"s) == Bytes(0, 0, 0, 0, 0, 0, 0, 1, 'x'));
    CHECK(SerialiseLE("x"s) == Bytes(1, 0, 0, 0, 0, 0, 0, 0, 'x'));

    CHECK(SerialiseBE("Hello, world"s) == Bytes(
        0, 0, 0, 0, 0, 0, 0, 12,
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd'
    ));

    CHECK(SerialiseLE("Hello, world"s) == Bytes(
        12, 0, 0, 0, 0, 0, 0, 0,
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd'
    ));

    auto big = std::string(1'000, 'x');
    auto xs = ByteBuffer(1'000, std::byte('x'));
    auto big_be = Bytes(0, 0, 0, 0, 0, 0, 3, 0xE8);
    auto big_le = Bytes(0xE8, 0x03, 0, 0, 0, 0, 0, 0);
    big_be.insert(big_be.end(), xs.begin(), xs.end());
    big_le.insert(big_le.end(), xs.begin(), xs.end());
    CHECK(SerialiseBE(big) == big_be);
    CHECK(SerialiseLE(big) == big_le);
}

TEST_CASE("Serialisation: std::u32string") {
    CHECK(SerialiseBE(U""s) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(U""s) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));

    CHECK(SerialiseBE(U"x"s) == Bytes(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'x'));
    CHECK(SerialiseLE(U"x"s) == Bytes(1, 0, 0, 0, 0, 0, 0, 0, 'x', 0, 0, 0));

    CHECK(SerialiseBE(U"Hello, world"s) == Bytes(
        0, 0, 0, 0, 0, 0, 0, 12,
        0, 0, 0, 'H', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l',
        0, 0, 0, 'o', 0, 0, 0, ',', 0, 0, 0, ' ', 0, 0, 0, 'w',
        0, 0, 0, 'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd'
    ));

    CHECK(SerialiseLE(U"Hello, world"s) == Bytes(
        12, 0, 0, 0, 0, 0, 0, 0,
        'H', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l', 0, 0, 0,
        'o', 0, 0, 0, ',', 0, 0, 0, ' ', 0, 0, 0, 'w', 0, 0, 0,
        'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd', 0, 0, 0
    ));
}

TEST_CASE("Serialisation: std::array") {
    std::array<u8, 6> a{1, 2, 3, 4, 5, 6};
    std::array<u16, 6> b{1, 2, 3, 4, 5, 6};

    CHECK(SerialiseBE(std::array<u8, 6>{}) == Bytes(0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(std::array<u8, 6>{}) == Bytes(0, 0, 0, 0, 0, 0));
    CHECK(SerialiseBE(std::array<u16, 6>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(std::array<u16, 6>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    CHECK(SerialiseBE(a) == Bytes(1, 2, 3, 4, 5, 6));
    CHECK(SerialiseLE(a) == Bytes(1, 2, 3, 4, 5, 6));

    CHECK(SerialiseBE(b) == Bytes(0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6));
    CHECK(SerialiseLE(b) == Bytes(1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));
}

TEST_CASE("Serialisation: std::vector") {
    std::vector<u8> a{1, 2, 3, 4, 5, 6};
    std::vector<u16> b{1, 2, 3, 4, 5, 6};

    CHECK(SerialiseBE(std::vector<u8>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(std::vector<u8>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseBE(std::vector<u16>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    CHECK(SerialiseLE(std::vector<u16>{}) == Bytes(0, 0, 0, 0, 0, 0, 0, 0));

    CHECK(SerialiseBE(a) == Bytes(0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6));
    CHECK(SerialiseLE(a) == Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6));
    CHECK(SerialiseBE(b) == Bytes(0, 0, 0, 0, 0, 0, 0, 6, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6));
    CHECK(SerialiseLE(b) == Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));
}
