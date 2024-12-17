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

}
