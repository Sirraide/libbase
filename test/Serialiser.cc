#include "TestCommon.hh"

#include <base/Serialisation.hh>

using namespace base;
using Catch::Matchers::ContainsSubstring;

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

template <typename T>
auto DeserialiseBE(const ByteBuffer& b) -> T {
    return ser::Deserialise<T, std::endian::big>(b).value();
}

template <typename T, std::integral ...Vals>
auto DeserialiseBE(Vals ...v) -> T {
    return DeserialiseBE<T>(Bytes(v...));
}

template <typename T>
auto DeserialiseLE(const ByteBuffer& b) -> T {
    return ser::Deserialise<T, std::endian::little>(b).value();
}

template <typename T, std::integral ...Vals>
auto DeserialiseLE(Vals ...v) -> T {
    return DeserialiseLE<T>(Bytes(v...));
}

template <typename T>
auto Test(const T& t, const ByteBuffer& big, const ByteBuffer& little) {
    CHECK(SerialiseBE(t) == big);
    CHECK(SerialiseLE(t) == little);
    CHECK(DeserialiseBE<T>(big) == t);
    CHECK(DeserialiseLE<T>(little) == t);
    CHECK(DeserialiseBE<T>(SerialiseBE(t)) == t);
    CHECK(DeserialiseLE<T>(SerialiseLE(t)) == t);
}

template <typename T>
auto Test(const T& t, const ByteBuffer& both) {
    Test(t, both, both);
}

TEST_CASE("Serialisation: Zero integer") {
    Test(u8(0), Bytes(0));
    Test(u16(0), Bytes(0, 0));
    Test(u32(0), Bytes(0, 0, 0, 0));
    Test(u64(0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));
}

TEST_CASE("Serialisation: Integers") {
    // u8
    Test(char(47), Bytes(47));
    Test(char8_t(47), Bytes(47));
    Test(u8(47), Bytes(47));
    Test(std::byte(47), Bytes(47));

    // u16
    Test(char16_t(0x1234), Bytes(0x12, 0x34), Bytes(0x34, 0x12));
    Test(i16(0x1234), Bytes(0x12, 0x34), Bytes(0x34, 0x12));
    Test(u16(0x1234), Bytes(0x12, 0x34), Bytes(0x34, 0x12));

    // u32
    Test(char32_t(0x12345678), Bytes(0x12, 0x34, 0x56, 0x78), Bytes(0x78, 0x56, 0x34, 0x12));
    Test(i32(0x12345678), Bytes(0x12, 0x34, 0x56, 0x78), Bytes(0x78, 0x56, 0x34, 0x12));
    Test(u32(0x12345678), Bytes(0x12, 0x34, 0x56, 0x78), Bytes(0x78, 0x56, 0x34, 0x12));

    // u64
    Test(i64(0x123456789ABCDEF0), Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0), Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
    Test(u64(0x123456789ABCDEF0), Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0), Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<u8>(0x12, 0xff) == u8(0x12));
        CHECK(DeserialiseLE<u8>(0x12, 0xff) == u8(0x12));
        CHECK(DeserialiseBE<u16>(0x12, 0x34, 0xff) == u16(0x1234));
        CHECK(DeserialiseLE<u16>(0x34, 0x12, 0xff) == u16(0x1234));
        CHECK(DeserialiseBE<u32>(0x12, 0x34, 0x56, 0x78, 0xff) == u32(0x12345678));
        CHECK(DeserialiseLE<u32>(0x78, 0x56, 0x34, 0x12, 0xff) == u32(0x12345678));
        CHECK(DeserialiseBE<u64>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0xff) == u64(0x123456789ABCDEF0));
        CHECK(DeserialiseLE<u64>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xff) == u64(0x123456789ABCDEF0));
    }

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<u8>());
        CHECK_THROWS(DeserialiseLE<u8>());
        CHECK_THROWS(DeserialiseBE<u16>(0x12));
        CHECK_THROWS(DeserialiseLE<u16>(0x34));
        CHECK_THROWS(DeserialiseBE<u32>(0x12, 0x34, 0x56));
        CHECK_THROWS(DeserialiseLE<u32>(0x78, 0x56, 0x34));
        CHECK_THROWS(DeserialiseBE<u64>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE));
        CHECK_THROWS(DeserialiseLE<u64>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34));
    }
}


TEST_CASE("Serialisation: Enums") {
    Test(u8enum(0x12), Bytes(0x12));
    Test(u16enum(0x1234), Bytes(0x12, 0x34), Bytes(0x34, 0x12));
    Test(u32enum(0x12345678), Bytes(0x12, 0x34, 0x56, 0x78), Bytes(0x78, 0x56, 0x34, 0x12));
    Test(u64enum(0x123456789ABCDEF0), Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0), Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
}

TEST_CASE("Serialisation: Floats") {
    SECTION("Simple values") {
        Test(3.14f, Bytes(0x40, 0x48, 0xF5, 0xC3), Bytes(0xC3, 0xF5, 0x48, 0x40));
        Test(3.14, Bytes(0x40, 0x09, 0x1e, 0xb8, 0x51, 0xeb, 0x85, 0x1f), Bytes(0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x09, 0x40));
    }

    SECTION("Zero") {
        Test(0.0f, Bytes(0, 0, 0, 0));
        Test(0.0, Bytes(0, 0, 0, 0, 0, 0, 0, 0));

        // If signed zeroes are enabled, also check for those.
        if (std::bit_cast<u32>(0.f) != std::bit_cast<u32>(-0.f)) {
            Test(-0.0f, Bytes(0x80, 0, 0, 0), Bytes(0, 0, 0, 0x80));
            Test(-0.0, Bytes(0x80, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0x80));
        }
    }

    SECTION("+Inf") {
        Test(std::numeric_limits<float>::infinity(), Bytes(0x7F, 0x80, 0, 0), Bytes(0, 0, 0x80, 0x7F));
        Test(std::numeric_limits<double>::infinity(), Bytes(0x7F, 0xF0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0xF0, 0x7F));
    }

    SECTION("-Inf") {
        Test(-std::numeric_limits<float>::infinity(), Bytes(0xFF, 0x80, 0, 0), Bytes(0, 0, 0x80, 0xFF));
        Test(-std::numeric_limits<double>::infinity(), Bytes(0xFF, 0xF0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0xF0, 0xFF));
    }

    // Comparing nans with == doesn’t work, so do these manually.
    SECTION("NaN") {
        CHECK(SerialiseBE(std::numeric_limits<float>::quiet_NaN()) == Bytes(0x7F, 0xC0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<float>::quiet_NaN()) == Bytes(0, 0, 0xC0, 0x7F));
        CHECK(SerialiseBE(std::numeric_limits<double>::quiet_NaN()) == Bytes(0x7F, 0xF8, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<double>::quiet_NaN()) == Bytes(0, 0, 0, 0, 0, 0, 0xF8, 0x7F));
        CHECK(std::isnan(DeserialiseBE<f32>(0x7F, 0xC0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f32>(0, 0, 0xC0, 0x7F)));
        CHECK(std::isnan(DeserialiseBE<f64>(0x7F, 0xF8, 0, 0, 0, 0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF8, 0x7F)));
    }

    SECTION("SNaN") {
        CHECK(SerialiseBE(std::numeric_limits<float>::signaling_NaN()) == Bytes(0x7F, 0xA0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<float>::signaling_NaN()) == Bytes(0, 0, 0xA0, 0x7F));
        CHECK(SerialiseBE(std::numeric_limits<double>::signaling_NaN()) == Bytes(0x7F, 0xF4, 0, 0, 0, 0, 0, 0));
        CHECK(SerialiseLE(std::numeric_limits<double>::signaling_NaN()) == Bytes(0, 0, 0, 0, 0, 0, 0xF4, 0x7F));
        CHECK(std::isnan(DeserialiseBE<f32>(0x7F, 0xA0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f32>(0, 0, 0xA0, 0x7F)));
        CHECK(std::isnan(DeserialiseBE<f64>(0x7F, 0xF4, 0, 0, 0, 0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF4, 0x7F)));
    }
}


TEST_CASE("Serialisation: std::string") {
    Test(""s, Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    Test("x"s, Bytes(0, 0, 0, 0, 0, 0, 0, 1, 'x'), Bytes(1, 0, 0, 0, 0, 0, 0, 0, 'x'));

    Test("Hello, world"s, Bytes(
        0, 0, 0, 0, 0, 0, 0, 12,
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd'
    ), Bytes(
        12, 0, 0, 0, 0, 0, 0, 0,
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd'
    ));

    auto big = std::string(1'000, 'x');
    auto xs = ByteBuffer(1'000, std::byte('x'));
    auto big_be = Bytes(0, 0, 0, 0, 0, 0, 3, 0xE8);
    auto big_le = Bytes(0xE8, 0x03, 0, 0, 0, 0, 0, 0);
    big_be.insert(big_be.end(), xs.begin(), xs.end());
    big_le.insert(big_le.end(), xs.begin(), xs.end());
    Test(big, big_be, big_le);

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 1, 'x', 'y') == "x");
        CHECK(DeserialiseLE<std::string>(1, 0, 0, 0, 0, 0, 0, 0, 'x', 'y') == "x");
        CHECK(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 3, 'a', 'b', 'c', 'y') == "abc");
        CHECK(DeserialiseLE<std::string>(3, 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c', 'y') == "abc");
    }

    SECTION("Size incomplete") {
        CHECK_THROWS(DeserialiseBE<std::string>());
        CHECK_THROWS(DeserialiseLE<std::string>());
        CHECK_THROWS(DeserialiseBE<std::string>(1));
        CHECK_THROWS(DeserialiseLE<std::string>(1));
        CHECK_THROWS(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0));
        CHECK_THROWS(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0));
    }

    SECTION("Data incomplete") {
        CHECK_THROWS(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 2, 'x'));
        CHECK_THROWS(DeserialiseLE<std::string>(2, 0, 0, 0, 0, 0, 0, 0, 'x'));
        CHECK_THROWS(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 5, 'a', 'b', 'c'));
        CHECK_THROWS(DeserialiseLE<std::string>(5, 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c'));

        CHECK_NOTHROW(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 2, 'x', 'y'));
        CHECK_NOTHROW(DeserialiseLE<std::string>(2, 0, 0, 0, 0, 0, 0, 0, 'x', 'y'));
        CHECK_NOTHROW(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 4, 'a', 'b', 'c', 'y', 'y'));
        CHECK_NOTHROW(DeserialiseLE<std::string>(4, 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c', 'y', 'y'));
    }

    SECTION("Too big") {
        CHECK_THROWS_WITH(
            DeserialiseBE<std::string>(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff),
            ContainsSubstring(std::format(
                "Input size {} exceeds maximum size {} of std::basic_string<>",
                std::numeric_limits<u64>::max(),
                std::string{}.max_size()
            ))
        );
    }
}

TEST_CASE("Serialisation: std::u32string") {
    Test(U""s, Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    Test(U"x"s, Bytes(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'x'), Bytes(1, 0, 0, 0, 0, 0, 0, 0, 'x', 0, 0, 0));

    Test(U"Hello, world"s, Bytes(
        0, 0, 0, 0, 0, 0, 0, 12,
        0, 0, 0, 'H', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l',
        0, 0, 0, 'o', 0, 0, 0, ',', 0, 0, 0, ' ', 0, 0, 0, 'w',
        0, 0, 0, 'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd'
    ), Bytes(
        12, 0, 0, 0, 0, 0, 0, 0,
        'H', 0, 0, 0, 'e', 0, 0, 0, 'l', 0, 0, 0, 'l', 0, 0, 0,
        'o', 0, 0, 0, ',', 0, 0, 0, ' ', 0, 0, 0, 'w', 0, 0, 0,
        'o', 0, 0, 0, 'r', 0, 0, 0, 'l', 0, 0, 0, 'd', 0, 0, 0
    ));

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'x', 0xff) == U"x");
        CHECK(DeserialiseLE<std::u32string>(1, 0, 0, 0, 0, 0, 0, 0, 'x', 0, 0, 0, 0xff) == U"x");
        CHECK(
            DeserialiseBE<std::u32string>(
                0, 0, 0, 0, 0, 0, 0, 3,
                0, 0, 0, 'a',
                0, 0, 0, 'b',
                0, 0, 0, 'c',
                0xff
            )
            ==
            U"abc"
        );

        CHECK(
            DeserialiseLE<std::u32string>(
                3, 0, 0, 0, 0, 0, 0, 0,
                'a', 0, 0, 0,
                'b', 0, 0, 0,
                'c', 0, 0, 0,
                0xff
            )
            ==
            U"abc"
        );
    }

    SECTION("Size incomplete") {
        CHECK_THROWS(DeserialiseBE<std::u32string>());
        CHECK_THROWS(DeserialiseLE<std::u32string>());
        CHECK_THROWS(DeserialiseBE<std::u32string>(1));
        CHECK_THROWS(DeserialiseLE<std::u32string>(1));
        CHECK_THROWS(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0));
        CHECK_THROWS(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0));
    }

    SECTION("Data incomplete") {
        CHECK_THROWS(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'x'));
        CHECK_THROWS(DeserialiseLE<std::u32string>(1, 0, 0, 0, 0, 0, 0, 0, 'x', 0, 0));
        CHECK_THROWS(
            DeserialiseBE<std::u32string>(
                0, 0, 0, 0, 0, 0, 0, 3,
                0, 0, 0, 'a',
                0, 0, 0, 'b',
                0
            )
        );

        CHECK_THROWS(
            DeserialiseLE<std::u32string>(
                3, 0, 0, 0, 0, 0, 0, 0,
                'a', 0, 0, 0,
                'b', 0, 0, 0
            )
        );
    }

    SECTION("Too big") {
        CHECK_THROWS_WITH(
            DeserialiseBE<std::u32string>(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff),
            ContainsSubstring(std::format(
                "Input size {} exceeds maximum size {} of range",
                std::numeric_limits<u64>::max(),
                std::u32string{}.max_size()
            ))
        );
    }
}

TEST_CASE("Serialisation: std::array") {
    std::array<u8, 6> a{1, 2, 3, 4, 5, 6};
    std::array<u16, 6> b{1, 2, 3, 4, 5, 6};

    Test(std::array<u8, 6>{}, Bytes(0, 0, 0, 0, 0, 0));
    Test(std::array<u16, 6>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    Test(a, Bytes(1, 2, 3, 4, 5, 6), Bytes(1, 2, 3, 4, 5, 6));
    Test(b, Bytes(0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6), Bytes(1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<std::array<u8, 6>>(1, 2, 3, 4, 5, 6, 0xff) == a);
        CHECK(DeserialiseLE<std::array<u8, 6>>(1, 2, 3, 4, 5, 6, 0xff) == a);
        CHECK(DeserialiseBE<std::array<u16, 6>>(0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 0xff) == b);
        CHECK(DeserialiseLE<std::array<u16, 6>>(1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 0xff) == b);
    }

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<std::array<u8, 6>>());
        CHECK_THROWS(DeserialiseLE<std::array<u8, 6>>());
        CHECK_THROWS(DeserialiseBE<std::array<u8, 6>>(1, 2, 3, 4, 5));
        CHECK_THROWS(DeserialiseLE<std::array<u8, 6>>(1, 2, 3, 4, 5));
        CHECK_THROWS(DeserialiseBE<std::array<u16, 6>>(0, 1, 0, 2, 0, 3, 0, 4, 0, 5));
        CHECK_THROWS(DeserialiseLE<std::array<u16, 6>>(1, 0, 2, 0, 3, 0));
    }
}

TEST_CASE("Serialisation: std::vector") {
    std::vector<u8> a{1, 2, 3, 4, 5, 6};
    std::vector<u16> b{1, 2, 3, 4, 5, 6};

    Test(std::vector<u8>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    Test(std::vector<u16>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0));

    Test(a, Bytes(0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6), Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6));
    Test(b, Bytes(0, 0, 0, 0, 0, 0, 0, 6, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6), Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6, 0xff) == a);
        CHECK(DeserialiseLE<std::vector<u8>>(6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 0xff) == a);
        CHECK(DeserialiseBE<std::vector<u16>>(0, 0, 0, 0, 0, 0, 0, 6, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 0xff) == b);
        CHECK(DeserialiseLE<std::vector<u16>>(6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 0, 0xff) == b);
    }

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0));
        CHECK_THROWS(DeserialiseLE<std::vector<u8>>(0, 0, 0, 0, 0, 0));
        CHECK_THROWS(DeserialiseBE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0, 7, 1, 2, 3, 4, 5));
        CHECK_THROWS(DeserialiseLE<std::vector<u8>>(7, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5));
        CHECK_THROWS(DeserialiseBE<std::vector<u16>>(0, 0, 0, 0, 0, 0, 0, 6, 1, 0, 2, 0, 3, 0, 4, 0, 5));
        CHECK_THROWS(DeserialiseLE<std::vector<u16>>(6, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0));
    }
}

TEST_CASE("Serialisation: std::optional<>") {
    Test(std::optional<int>{}, Bytes(0));
    Test(std::optional<std::string>{}, Bytes(0));
    Test(std::optional<std::vector<u8>>{}, Bytes(0));

    Test(std::optional<int>{42}, Bytes(1, 0, 0, 0, 42), Bytes(1, 42, 0, 0, 0));

    Test(
        std::optional<std::string>{"foobar"},
        Bytes(1, 0, 0, 0, 0, 0, 0, 0, 6, 'f', 'o', 'o', 'b', 'a', 'r'),
        Bytes(1, 6, 0, 0, 0, 0, 0, 0, 0, 'f', 'o', 'o', 'b', 'a', 'r')
    );

    Test(
        std::optional<std::vector<u8>>{{1, 2, 3, 4, 5, 6}},
        Bytes(1, 0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6),
        Bytes(1, 6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6)
    );

    SECTION("Nested optionals") {
        Test(std::optional<std::optional<int>>{}, Bytes(0));
        Test(std::optional<std::optional<int>>{std::optional<int>{std::nullopt}}, Bytes(1, 0));
        Test(std::optional<std::optional<int>>{{4}}, Bytes(1, 1, 0, 0, 0, 4), Bytes(1, 1, 4, 0, 0, 0));
    }
}

TEST_CASE("Serialisation: std::variant") {
    using V1 = std::variant<u8, int, std::monostate, std::string>;

    Test(V1(u8(42)), Bytes(0, 0, 0, 0, 0, 0, 0, 0, 42));
    Test(V1(int(42)), Bytes(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 42), Bytes(1, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0));
    Test(V1(std::monostate{}), Bytes(0, 0, 0, 0, 0, 0, 0, 2), Bytes(2, 0, 0, 0, 0, 0, 0, 0));
    Test(
        V1("foobar"s),
        Bytes(0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 6, 'f', 'o', 'o', 'b', 'a', 'r'),
        Bytes(3, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 'f', 'o', 'o', 'b', 'a', 'r')
    );
}
