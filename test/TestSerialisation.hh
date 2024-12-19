#ifndef SERIALISATION_HH
#define SERIALISATION_HH

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

#endif //SERIALISATION_HH
