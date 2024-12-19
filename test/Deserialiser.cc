#include "TestSerialisation.hh"

TEST_CASE("Deserialisation: Zero integer") {
    CHECK(DeserialiseBE<u8>(0) == u8(0));
    CHECK(DeserialiseLE<u8>(0) == u8(0));
    CHECK(DeserialiseBE<u16>(0, 0) == u16(0));
    CHECK(DeserialiseLE<u16>(0, 0) == u16(0));
    CHECK(DeserialiseBE<u32>(0, 0, 0, 0) == u32(0));
    CHECK(DeserialiseLE<u32>(0, 0, 0, 0) == u32(0));
    CHECK(DeserialiseBE<u64>(0, 0, 0, 0, 0, 0, 0, 0) == u64(0));
    CHECK(DeserialiseLE<u64>(0, 0, 0, 0, 0, 0, 0, 0) == u64(0));

    CHECK_THROWS(DeserialiseBE<u8>());
    CHECK_THROWS(DeserialiseLE<u8>());
    CHECK_THROWS(DeserialiseBE<u16>());
    CHECK_THROWS(DeserialiseLE<u16>());
    CHECK_THROWS(DeserialiseBE<u32>());
    CHECK_THROWS(DeserialiseLE<u32>());
    CHECK_THROWS(DeserialiseBE<u64>());
    CHECK_THROWS(DeserialiseLE<u64>());

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<u8>());
        CHECK_THROWS(DeserialiseLE<u8>());
        CHECK_THROWS(DeserialiseBE<u16>(1));
        CHECK_THROWS(DeserialiseLE<u16>(1));
        CHECK_THROWS(DeserialiseBE<u32>(1, 2, 3));
        CHECK_THROWS(DeserialiseLE<u32>(1, 2, 3));
        CHECK_THROWS(DeserialiseBE<u64>(1, 2, 3, 4, 5, 6, 7));
        CHECK_THROWS(DeserialiseLE<u64>(1, 2, 3, 4, 5, 6, 7));
    }
}

TEST_CASE("Deserialisation: Integers") {
    CHECK(DeserialiseBE<u8>(0x12) == u8(0x12));
    CHECK(DeserialiseLE<u8>(0x12) == u8(0x12));
    CHECK(DeserialiseBE<u16>(0x12, 0x34) == u16(0x1234));
    CHECK(DeserialiseLE<u16>(0x34, 0x12) == u16(0x1234));
    CHECK(DeserialiseBE<u32>(0x12, 0x34, 0x56, 0x78) == u32(0x12345678));
    CHECK(DeserialiseLE<u32>(0x78, 0x56, 0x34, 0x12) == u32(0x12345678));
    CHECK(DeserialiseBE<u64>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0) == u64(0x123456789ABCDEF0));
    CHECK(DeserialiseLE<u64>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12) == u64(0x123456789ABCDEF0));

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

TEST_CASE("Deserialisation: Enums") {
    CHECK(DeserialiseBE<u8enum>(0x12) == u8enum(0x12));
    CHECK(DeserialiseLE<u8enum>(0x12) == u8enum(0x12));
    CHECK(DeserialiseBE<u16enum>(0x12, 0x34) == u16enum(0x1234));
    CHECK(DeserialiseLE<u16enum>(0x34, 0x12) == u16enum(0x1234));
    CHECK(DeserialiseBE<u32enum>(0x12, 0x34, 0x56, 0x78) == u32enum(0x12345678));
    CHECK(DeserialiseLE<u32enum>(0x78, 0x56, 0x34, 0x12) == u32enum(0x12345678));
    CHECK(DeserialiseBE<u64enum>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0) == u64enum(0x123456789ABCDEF0));
    CHECK(DeserialiseLE<u64enum>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12) == u64enum(0x123456789ABCDEF0));

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<u8enum>(0x12, 0xff) == u8enum(0x12));
        CHECK(DeserialiseLE<u8enum>(0x12, 0xff) == u8enum(0x12));
        CHECK(DeserialiseBE<u16enum>(0x12, 0x34, 0xff) == u16enum(0x1234));
        CHECK(DeserialiseLE<u16enum>(0x34, 0x12, 0xff) == u16enum(0x1234));
        CHECK(DeserialiseBE<u32enum>(0x12, 0x34, 0x56, 0x78, 0xff) == u32enum(0x12345678));
        CHECK(DeserialiseLE<u32enum>(0x78, 0x56, 0x34, 0x12, 0xff) == u32enum(0x12345678));
        CHECK(DeserialiseBE<u64enum>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0xff) == u64enum(0x123456789ABCDEF0));
        CHECK(DeserialiseLE<u64enum>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xff) == u64enum(0x123456789ABCDEF0));
    }

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<u8enum>());
        CHECK_THROWS(DeserialiseLE<u8enum>());
        CHECK_THROWS(DeserialiseBE<u16enum>(0x12));
        CHECK_THROWS(DeserialiseLE<u16enum>(0x34));
        CHECK_THROWS(DeserialiseBE<u32enum>(0x12, 0x34, 0x56));
        CHECK_THROWS(DeserialiseLE<u32enum>(0x78, 0x56, 0x34));
        CHECK_THROWS(DeserialiseBE<u64enum>(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE));
        CHECK_THROWS(DeserialiseLE<u64enum>(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34));
    }
}

TEST_CASE("Deserialisation: Floats") {
    SECTION("Simple values") {
        CHECK(DeserialiseBE<f32>(0x40, 0x48, 0xF5, 0xC3) == 3.14f);
        CHECK(DeserialiseLE<f32>(0xC3, 0xF5, 0x48, 0x40) == 3.14f);
        CHECK(DeserialiseBE<f64>(0x40, 0x09, 0x1e, 0xb8, 0x51, 0xeb, 0x85, 0x1f) == 3.14);
        CHECK(DeserialiseLE<f64>(0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x09, 0x40) == 3.14);
    }

    SECTION("Zero") {
        CHECK(DeserialiseBE<f32>(0, 0, 0, 0) == 0.0f);
        CHECK(DeserialiseLE<f32>(0, 0, 0, 0) == 0.0f);
        CHECK(DeserialiseBE<f64>(0, 0, 0, 0, 0, 0, 0, 0) == 0.0);
        CHECK(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0, 0) == 0.0);

        if (std::bit_cast<u32>(0.f) != std::bit_cast<u32>(-0.f)) {
            CHECK(DeserialiseBE<f32>(0x80, 0, 0, 0) == -0.0f);
            CHECK(DeserialiseLE<f32>(0, 0, 0, 0x80) == -0.0f);
            CHECK(DeserialiseBE<f64>(0x80, 0, 0, 0, 0, 0, 0, 0) == -0.0);
            CHECK(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0, 0x80) == -0.0);
        }
    }

    SECTION("+Inf") {
        CHECK(DeserialiseBE<f32>(0x7F, 0x80, 0, 0) == std::numeric_limits<f32>::infinity());
        CHECK(DeserialiseLE<f32>(0, 0, 0x80, 0x7F) == std::numeric_limits<f32>::infinity());
        CHECK(DeserialiseBE<f64>(0x7F, 0xF0, 0, 0, 0, 0, 0, 0) == std::numeric_limits<f64>::infinity());
        CHECK(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF0, 0x7F) == std::numeric_limits<f64>::infinity());
    }

    SECTION("-Inf") {
        CHECK(DeserialiseBE<f32>(0xFF, 0x80, 0, 0) == -std::numeric_limits<f32>::infinity());
        CHECK(DeserialiseLE<f32>(0, 0, 0x80, 0xFF) == -std::numeric_limits<f32>::infinity());
        CHECK(DeserialiseBE<f64>(0xFF, 0xF0, 0, 0, 0, 0, 0, 0) == -std::numeric_limits<f64>::infinity());
        CHECK(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF0, 0xFF) == -std::numeric_limits<f64>::infinity());
    }

    SECTION("NaN") {
        CHECK(std::isnan(DeserialiseBE<f32>(0x7F, 0xC0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f32>(0, 0, 0xC0, 0x7F)));
        CHECK(std::isnan(DeserialiseBE<f64>(0x7F, 0xF8, 0, 0, 0, 0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF8, 0x7F)));
    }

    SECTION("SNaN") {
        CHECK(std::isnan(DeserialiseBE<f32>(0x7F, 0xA0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f32>(0, 0, 0xA0, 0x7F)));
        CHECK(std::isnan(DeserialiseBE<f64>(0x7F, 0xF4, 0, 0, 0, 0, 0, 0)));
        CHECK(std::isnan(DeserialiseLE<f64>(0, 0, 0, 0, 0, 0, 0xF4, 0x7F)));
    }

    SECTION("Extra trailing data") {
        CHECK(DeserialiseBE<f32>(0x40, 0x48, 0xF5, 0xC3, 0xff) == 3.14f);
        CHECK(DeserialiseLE<f32>(0xC3, 0xF5, 0x48, 0x40, 0xff) == 3.14f);
        CHECK(DeserialiseBE<f64>(0x40, 0x09, 0x1e, 0xb8, 0x51, 0xeb, 0x85, 0x1f, 0xff) == 3.14);
        CHECK(DeserialiseLE<f64>(0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x09, 0x40, 0xff) == 3.14);
    }

    SECTION("Not enough data") {
        CHECK_THROWS(DeserialiseBE<f32>(0x40, 0x48, 0xF5) == 3.14f);
        CHECK_THROWS(DeserialiseLE<f32>(0xC3, 0xF5, 0x48) == 3.14f);
        CHECK_THROWS(DeserialiseBE<f64>(0x40, 0x09, 0x1e, 0xb8, 0x51, 0xeb, 0x85) == 3.14);
        CHECK_THROWS(DeserialiseLE<f64>(0x1f, 0x85, 0xeb, 0x51, 0xb8, 0x1e, 0x09) == 3.14);
    }
}

TEST_CASE("Deserialisation: std::string") {
    SECTION("Empty") {
        CHECK(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 0) == "");
        CHECK(DeserialiseLE<std::string>(0, 0, 0, 0, 0, 0, 0, 0) == "");
    }

    SECTION("Simple values") {
        CHECK(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 1, 'x') == "x");
        CHECK(DeserialiseLE<std::string>(1, 0, 0, 0, 0, 0, 0, 0, 'x') == "x");
        CHECK(DeserialiseBE<std::string>(0, 0, 0, 0, 0, 0, 0, 3, 'a', 'b', 'c') == "abc");
        CHECK(DeserialiseLE<std::string>(3, 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c') == "abc");

        CHECK(
            DeserialiseBE<std::string>(
                0, 0, 0, 0, 0, 0, 0, 11,
                'H', 'e', 'l', 'l', 'o', ' ',
                'w', 'o', 'r', 'l', 'd'
            ) ==
            "Hello world"
        );

        CHECK(
            DeserialiseLE<std::string>(
                11, 0, 0, 0, 0, 0, 0, 0,
                'H', 'e', 'l', 'l', 'o', ' ',
                'w', 'o', 'r', 'l', 'd'
            ) ==
            "Hello world"
        );
    }

    SECTION("Big string") {
        auto big = std::string(1'000, 'x');
        auto xs = ByteBuffer(1'000, std::byte('x'));
        auto big_be = Bytes(0, 0, 0, 0, 0, 0, 3, 0xE8);
        auto big_le = Bytes(0xE8, 0x03, 0, 0, 0, 0, 0, 0);
        big_be.insert(big_be.end(), xs.begin(), xs.end());
        big_le.insert(big_le.end(), xs.begin(), xs.end());
        CHECK(DeserialiseBE<std::string>(big_be) == big);
        CHECK(DeserialiseLE<std::string>(big_le) == big);
    }

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

TEST_CASE("Deserialisation: std::u32string") {
    SECTION("Empty") {
        CHECK(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0, 0) == U"");
        CHECK(DeserialiseLE<std::u32string>(0, 0, 0, 0, 0, 0, 0, 0) == U"");
    }

    SECTION("Simple values") {
        CHECK(DeserialiseBE<std::u32string>(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'x') == U"x");
        CHECK(DeserialiseLE<std::u32string>(1, 0, 0, 0, 0, 0, 0, 0, 'x', 0, 0, 0) == U"x");
        CHECK(
            DeserialiseBE<std::u32string>(
                0, 0, 0, 0, 0, 0, 0, 3,
                0, 0, 0, 'a',
                0, 0, 0, 'b',
                0, 0, 0, 'c'
            )
            ==
            U"abc"
        );

        CHECK(
            DeserialiseLE<std::u32string>(
                3, 0, 0, 0, 0, 0, 0, 0,
                'a', 0, 0, 0,
                'b', 0, 0, 0,
                'c', 0, 0, 0
            )
            ==
            U"abc"
        );

        CHECK(
            DeserialiseBE<std::u32string>(
                0, 0, 0, 0, 0, 0, 0, 11,
                0, 0, 0, 'H',
                0, 0, 0, 'e',
                0, 0, 0, 'l',
                0, 0, 0, 'l',
                0, 0, 0, 'o',
                0, 0, 0, ' ',
                0, 0, 0, 'w',
                0, 0, 0, 'o',
                0, 0, 0, 'r',
                0, 0, 0, 'l',
                0, 0, 0, 'd'
            ) ==
            U"Hello world"
        );

        CHECK(
            DeserialiseLE<std::u32string>(
                11, 0, 0, 0, 0, 0, 0, 0,
                'H', 0, 0, 0,
                'e', 0, 0, 0,
                'l', 0, 0, 0,
                'l', 0, 0, 0,
                'o', 0, 0, 0,
                ' ', 0, 0, 0,
                'w', 0, 0, 0,
                'o', 0, 0, 0,
                'r', 0, 0, 0,
                'l', 0, 0, 0,
                'd', 0, 0, 0
            ) ==
            U"Hello world"
        );
    }

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

TEST_CASE("Deserialisation: std::array") {
    std::array<u8, 6> a{1, 2, 3, 4, 5, 6};
    std::array<u16, 6> b{1, 2, 3, 4, 5, 6};

    CHECK(DeserialiseBE<std::array<u8, 6>>(0, 0, 0, 0, 0, 0) == std::array<u8, 6>{});
    CHECK(DeserialiseLE<std::array<u8, 6>>(0, 0, 0, 0, 0, 0) == std::array<u8, 6>{});
    CHECK(DeserialiseBE<std::array<u16, 6>>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == std::array<u16, 6>{});
    CHECK(DeserialiseLE<std::array<u16, 6>>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == std::array<u16, 6>{});

    CHECK(DeserialiseBE<std::array<u8, 6>>(1, 2, 3, 4, 5, 6) == a);
    CHECK(DeserialiseLE<std::array<u8, 6>>(1, 2, 3, 4, 5, 6) == a);

    CHECK(DeserialiseBE<std::array<u16, 6>>(0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6) == b);
    CHECK(DeserialiseLE<std::array<u16, 6>>(1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0) == b);

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

TEST_CASE("Deserialisation: std::vector") {
    std::vector<u8> a{1, 2, 3, 4, 5, 6};
    std::vector<u16> b{1, 2, 3, 4, 5, 6};

    CHECK(DeserialiseBE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0, 0) == std::vector<u8>{});
    CHECK(DeserialiseLE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0, 0) == std::vector<u8>{});
    CHECK(DeserialiseBE<std::vector<u16>>(0, 0, 0, 0, 0, 0, 0, 0) == std::vector<u16>{});
    CHECK(DeserialiseLE<std::vector<u16>>(0, 0, 0, 0, 0, 0, 0, 0) == std::vector<u16>{});

    CHECK(DeserialiseBE<std::vector<u8>>(0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6) == a);
    CHECK(DeserialiseLE<std::vector<u8>>(6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6) == a);

    CHECK(DeserialiseBE<std::vector<u16>>(0, 0, 0, 0, 0, 0, 0, 6, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6) == b);
    CHECK(DeserialiseLE<std::vector<u16>>(6, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0) == b);

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

TEST_CASE("Deserialization: std::optional<>") {
    CHECK(DeserialiseBE<std::optional<int>>(0) == std::nullopt);
    CHECK(DeserialiseLE<std::optional<int>>(0) == std::nullopt);
    CHECK(DeserialiseBE<std::optional<int>>(1, 0, 0, 0, 42) == std::optional<int>{42});
    CHECK(DeserialiseLE<std::optional<int>>(1, 42, 0, 0, 0) == std::optional<int>{42});

    CHECK(DeserialiseBE<std::optional<std::string>>(0) == std::nullopt);
    CHECK(DeserialiseLE<std::optional<std::string>>(0) == std::nullopt);

    CHECK(DeserialiseBE<std::optional<std::string>>
        (1, 0, 0, 0, 0, 0, 0, 0, 6, 'f', 'o', 'o', 'b', 'a', 'r')
        ==
        std::optional<std::string>{"foobar"}
    );

    CHECK(DeserialiseLE<std::optional<std::string>>
        (1, 6, 0, 0, 0, 0, 0, 0, 0, 'f', 'o', 'o', 'b', 'a', 'r')
        ==
        std::optional<std::string>{"foobar"}
    );

    CHECK(DeserialiseBE<std::optional<std::vector<uint8_t>>>(0) == std::nullopt);
    CHECK(DeserialiseLE<std::optional<std::vector<uint8_t>>>(0) == std::nullopt);
    CHECK(DeserialiseBE<std::optional<std::vector<uint8_t>>>
        (1, 0, 0, 0, 0, 0, 0, 0, 6, 1, 2, 3, 4, 5, 6)
        ==
        std::optional<std::vector<uint8_t>>{{1, 2, 3, 4, 5, 6}}
    );

    CHECK(DeserialiseLE<std::optional<std::vector<uint8_t>>>
        (1, 6, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6)
        ==
        std::optional<std::vector<uint8_t>>{{1, 2, 3, 4, 5, 6}}
    );
}
