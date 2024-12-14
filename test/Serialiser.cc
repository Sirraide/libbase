#include "TestCommon.hh"

#include <base/Serialisation.hh>

using namespace base;

enum class u16enum : u16 {};
enum class u32enum : u32 {};
enum class u64enum : u64 {};

template <typename ...T>
auto Bytes(T... vals) -> std::vector<std::byte> {
    return {std::byte(vals)...};
}

template <typename T>
auto SerialiseBE(const T& t) -> std::vector<std::byte> {
    return ser::Serialise<std::endian::big>(t);
}

template <typename T>
auto SerialiseLE(const T& t) -> std::vector<std::byte> {
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
    CHECK(SerialiseBE(u16enum(0x1234)) == Bytes(0x12, 0x34));
    CHECK(SerialiseLE(u16enum(0x1234)) == Bytes(0x34, 0x12));

    // u32
    CHECK(SerialiseBE(char32_t(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(char32_t(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(i32(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(i32(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u32(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(u32(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u32enum(0x12345678)) == Bytes(0x12, 0x34, 0x56, 0x78));
    CHECK(SerialiseLE(u32enum(0x12345678)) == Bytes(0x78, 0x56, 0x34, 0x12));

    // u64
    CHECK(SerialiseBE(i64(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(i64(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u64(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(u64(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
    CHECK(SerialiseBE(u64enum(0x123456789ABCDEF0)) == Bytes(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0));
    CHECK(SerialiseLE(u64enum(0x123456789ABCDEF0)) == Bytes(0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12));
}
