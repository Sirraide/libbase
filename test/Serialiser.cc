#include "TestSerialisation.hh"

TEST_CASE("Serialisation: Zero integer") {
    Test(u8(0), Bytes(0), Bytes(0));
    Test(u16(0), Bytes(0, 0), Bytes(0, 0));
    Test(u32(0), Bytes(0, 0, 0, 0), Bytes(0, 0, 0, 0));
    Test(u64(0), Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));
}

TEST_CASE("Serialisation: Integers") {
    // u8
    Test(char(47), Bytes(47), Bytes(47));
    Test(char8_t(47), Bytes(47), Bytes(47));
    Test(u8(47), Bytes(47), Bytes(47));
    Test(std::byte(47), Bytes(47), Bytes(47));

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
}


TEST_CASE("Serialisation: Enums") {
    Test(u8enum(0x12), Bytes(0x12), Bytes(0x12));
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
        Test(0.0f, Bytes(0, 0, 0, 0), Bytes(0, 0, 0, 0));
        Test(0.0, Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));

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
    Test(""s, Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));
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
}

TEST_CASE("Serialisation: std::u32string") {
    Test(U""s, Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));
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
}

TEST_CASE("Serialisation: std::array") {
    std::array<u8, 6> a{1, 2, 3, 4, 5, 6};
    std::array<u16, 6> b{1, 2, 3, 4, 5, 6};

    Test(std::array<u8, 6>{}, Bytes(0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0));
    Test(std::array<u16, 6>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    Test(a, Bytes(1, 2, 3, 4, 5, 6), Bytes(1, 2, 3, 4, 5, 6));
    Test(b, Bytes(0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6), Bytes(1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));
}

TEST_CASE("Serialisation: std::vector") {
    std::vector<u8> a{1, 2, 3, 4, 5, 6};
    std::vector<u16> b{1, 2, 3, 4, 5, 6};

    Test(std::vector<u8>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));
    Test(std::vector<u16>{}, Bytes(0, 0, 0, 0, 0, 0, 0, 0), Bytes(0, 0, 0, 0, 0, 0, 0, 0));

    Test(a, Bytes(0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6), Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6));
    Test(b, Bytes(0, 0, 0, 0, 0, 0, 0, 6, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6), Bytes(6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0));
}

TEST_CASE("Serialisation: std::optional<>") {
    Test(std::optional<int>{}, Bytes(0), Bytes(0));
    Test(std::optional<int>{42}, Bytes(1, 0, 0, 0, 42), Bytes(1, 42, 0, 0, 0));

    Test(std::optional<std::string>{}, Bytes(0), Bytes(0));
    Test(
        std::optional<std::string>{"foobar"},
        Bytes(1, 0, 0, 0, 0, 0, 0, 0, 6, 'f', 'o', 'o', 'b', 'a', 'r'),
        Bytes(1, 6, 0, 0, 0, 0, 0, 0, 0, 'f', 'o', 'o', 'b', 'a', 'r')
    );

    Test(std::optional<std::vector<u8>>{}, Bytes(0), Bytes(0));
    Test(
        std::optional<std::vector<u8>>{{1, 2, 3, 4, 5, 6}},
        Bytes(1, 0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6),
        Bytes(1, 6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6)
    );
}
