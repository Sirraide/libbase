#include "TestCommon.hh"
#include <base/Size.hh>

using namespace base;

// We *definitely* need more tests if this ever changes.
static_assert(Size::BitsPerByte == CHAR_BIT);

TEST_CASE("Align::Align") {
    CHECK(Align() == Align(1));
    CHECK(Align().log_repr() == 0);
    CHECK(Align(Size::Bits(8)) == Align(1));
    CHECK(Align(Size::Bits(64)) == Align(8));

    // Valid values.
    for (u64 i = 0; i < 64; ++i) {
        Align a{1ull << i};
        CHECK(a.log_repr() == u8(i));
    }

    // Invalid values.
    CHECK_THROWS(Align{0});
    CHECK_THROWS(Align{3});
    CHECK_THROWS(Align{5});
    CHECK_THROWS(Align{7});
    CHECK_THROWS(Align{10});
    CHECK_THROWS(Align{12});
    CHECK_THROWS(Align{15});
    CHECK_THROWS(Align{1000000000});
}

TEST_CASE("Align::Of") {
    CHECK(Align::Of<Align>().value().bytes() == alignof(Align));
    CHECK(Align::Of<u8>().value().bytes() == alignof(u8));
    CHECK(Align::Of<u16>().value().bytes() == alignof(u16));
    CHECK(Align::Of<u32>().value().bytes() == alignof(u32));
    CHECK(Align::Of<u64>().value().bytes() == alignof(u64));
    CHECK(Align::Of<std::string>().value().bytes() == alignof(std::string));
}

TEST_CASE("Align::To") {
    CHECK(Align::To(0, 1) == 0);
    CHECK(Align::To(1, 1) == 1);
    CHECK(Align::To(2, 1) == 2);
    CHECK(Align::To(3, 1) == 3);

    CHECK(Align::To(0, 4) == 0);
    CHECK(Align::To(1, 4) == 4);
    CHECK(Align::To(2, 4) == 4);
    CHECK(Align::To(3, 4) == 4);
    CHECK(Align::To(4, 4) == 4);
    CHECK(Align::To(5, 4) == 8);
    CHECK(Align::To(6, 4) == 8);
    CHECK(Align::To(7, 4) == 8);
    CHECK(Align::To(8, 4) == 8);

    CHECK(Align::To(0, 512) == 0);
    CHECK(Align::To(1, 512) == 512);
    CHECK(Align::To(2, 512) == 512);
    CHECK(Align::To(511, 512) == 512);
    CHECK(Align::To(512, 512) == 512);
    CHECK(Align::To(513, 512) == 1024);
}

TEST_CASE("Align::align") {
    CHECK(Align{1}.align(reinterpret_cast<void*>(41)) == reinterpret_cast<void*>(41));
    CHECK(Align{2}.align(reinterpret_cast<void*>(41)) == reinterpret_cast<void*>(42));
    CHECK(Align{4}.align(reinterpret_cast<void*>(41)) == reinterpret_cast<void*>(44));
    CHECK(Align{8}.align(reinterpret_cast<void*>(41)) == reinterpret_cast<void*>(48));
    CHECK(Align{256}.align(reinterpret_cast<void*>(41)) == reinterpret_cast<void*>(256));
    CHECK(Align{256}.align(reinterpret_cast<void*>(257)) == reinterpret_cast<void*>(512));
}

TEST_CASE("Align::value") {
    CHECK(Align{1}.value().bytes() == 1);
    CHECK(Align{2}.value().bytes() == 2);
    CHECK(Align{4}.value().bytes() == 4);
    CHECK(Align{8}.value().bytes() == 8);
    CHECK(Align{64}.value().bytes() == 64);
}

TEST_CASE("Size: construction") {
    CHECK(Size().bits() == 0);
    CHECK(Size().bytes() == 0);

    CHECK(Size::Bytes(1).bits() == 8);
    CHECK(Size::Bytes(1).bytes() == 1);
    CHECK(Size::Bytes(2).bits() == 16);
    CHECK(Size::Bytes(2).bytes() == 2);
    CHECK(Size::Bytes(3).bits() == 24);
    CHECK(Size::Bytes(3).bytes() == 3);
    CHECK(Size::Bytes(4).bits() == 32);
    CHECK(Size::Bytes(4).bytes() == 4);

    CHECK(Size::Bits(1).bits() == 1);
    CHECK(Size::Bits(1).bytes() == 1);
    CHECK(Size::Bits(2).bits() == 2);
    CHECK(Size::Bits(2).bytes() == 1);
    CHECK(Size::Bits(3).bits() == 3);
    CHECK(Size::Bits(3).bytes() == 1);
    CHECK(Size::Bits(8).bits() == 8);
    CHECK(Size::Bits(8).bytes() == 1);
    CHECK(Size::Bits(63).bits() == 63);
    CHECK(Size::Bits(63).bytes() == 8);
    CHECK(Size::Bits(64).bits() == 64);
    CHECK(Size::Bits(64).bytes() == 8);
    CHECK(Size::Bits(65).bits() == 65);
    CHECK(Size::Bits(65).bytes() == 9);
}

TEST_CASE("Size::Of") {
    CHECK(Size::Of<Size>().bytes() == sizeof(Size));
    CHECK(Size::Of<u8>().bytes() == sizeof(u8));
    CHECK(Size::Of<u16>().bytes() == sizeof(u16));
    CHECK(Size::Of<u32>().bytes() == sizeof(u32));
    CHECK(Size::Of<u64>().bytes() == sizeof(u64));
    CHECK(Size::Of<std::string>().bytes() == sizeof(std::string));
}

TEST_CASE("Aligning sizes") {
    CHECK(Size().align(Align(1)).bits() == 0);
    CHECK(Size().align(Align(1)).bytes() == 0);
    CHECK(Size().align(Align(2)).bits() == 0);
    CHECK(Size().align(Align(2)).bytes() == 0);
    CHECK(Size().align(Align(128)).bits() == 0);
    CHECK(Size().align(Align(128)).bytes() == 0);

    CHECK(Size::Bytes(1).align(Align(1)).bytes() == 1);
    CHECK(Size::Bytes(1).align(Align(2)).bytes() == 2);
    CHECK(Size::Bytes(1).align(Align(4)).bytes() == 4);
    CHECK(Size::Bytes(1).align(Align(8)).bytes() == 8);
    CHECK(Size::Bits(63).align(Align(16)).bits() == 128);
    CHECK(Size::Bits(63).align(Align(16)).bytes() == 16);
    CHECK(Size::Bits(127).align(Align(16)).bits() == 128);
    CHECK(Size::Bits(127).align(Align(16)).bytes() == 16);
    CHECK(Size::Bits(128).align(Align(16)).bits() == 128);
    CHECK(Size::Bits(128).align(Align(16)).bytes() == 16);
    CHECK(Size::Bits(129).align(Align(16)).bits() == 256);
    CHECK(Size::Bits(129).align(Align(16)).bytes() == 32);
}

TEST_CASE("Align::as_bytes") {
    CHECK(Size().as_bytes().bits() == 0);
    CHECK(Size::Bits(1).as_bytes().bits() == 8);
    CHECK(Size::Bits(1).as_bytes().bytes() == 1);
    CHECK(Size::Bits(7).as_bytes().bits() == 8);
    CHECK(Size::Bits(7).as_bytes().bytes() == 1);
    CHECK(Size::Bits(8).as_bytes().bits() == 8);
    CHECK(Size::Bits(8).as_bytes().bytes() == 1);
    CHECK(Size::Bits(9).as_bytes().bits() == 16);
    CHECK(Size::Bits(9).as_bytes().bytes() == 2);
}

TEST_CASE("Size::is_power_of_2()") {
    CHECK(not Size::Bits(0).is_power_of_2());
    CHECK(Size::Bits(1).is_power_of_2());
    CHECK(Size::Bits(2).is_power_of_2());
    CHECK(not Size::Bits(3).is_power_of_2());
    CHECK(not Size::Bits(7).is_power_of_2());
    CHECK(Size::Bits(8).is_power_of_2());
    CHECK(not Size::Bits(63).is_power_of_2());
    CHECK(Size::Bits(64).is_power_of_2());
    CHECK(not Size::Bits(65).is_power_of_2());
}

TEST_CASE("Size x Size") {
    CHECK(Size::Bits(3) + Size::Bits(3) == Size::Bits(6));
    CHECK(Size::Bits(4) + Size::Bits(4) == Size::Bits(8));
    CHECK(Size::Bytes(3) + Size::Bytes(3) == Size::Bytes(6));
    CHECK(Size::Bytes(4) + Size::Bytes(4) == Size::Bytes(8));

    CHECK(Size::Bits(3) - Size::Bits(3) == Size::Bits(0));
    CHECK(Size::Bits(4) - Size::Bits(1) == Size::Bits(3));
    CHECK(Size::Bytes(3) - Size::Bytes(3) == Size::Bytes(0));
    CHECK(Size::Bytes(4) - Size::Bytes(1) == Size::Bytes(3));

    CHECK(Size::Bytes(2) + Size::Bits(3) == Size::Bits(19));
    CHECK(Size::Bytes(2) - Size::Bits(3) == Size::Bits(13));

    CHECK(Size::Bytes(16) / Size::Bytes(4) == 4);
    CHECK(Size::Of<i64[4]>() / Size::Of<i64>() == 4);

    Size s = Size::Bytes(3);
    s += Size::Bytes(3);
    CHECK(s == Size::Bytes(6));
    s -= Size::Bytes(2);
    CHECK(s == Size::Bytes(4));
    s -= Size::Bits(3);
    CHECK(s == Size::Bits(29));

    CHECK_THROWS(Size::Bytes(2) - Size::Bytes(3));
    CHECK_THROWS(Size() - Size::Bits(1));
}

TEST_CASE("Size x scalar") {
    CHECK(Size::Bits(3) * 2 == Size::Bits(6));
    CHECK(Size::Bits(4) * 2 == Size::Bits(8));
    CHECK(Size::Bytes(3) * 2 == Size::Bytes(6));
    CHECK(Size::Bytes(4) * 2 == Size::Bytes(8));
    CHECK(2 * Size::Bits(3) == Size::Bits(6));
    CHECK(2 * Size::Bits(4) == Size::Bits(8));
    CHECK(2 * Size::Bytes(3) == Size::Bytes(6));
    CHECK(2 * Size::Bytes(4) == Size::Bytes(8));
}

TEST_CASE("Pointer x Size") {
    auto p1 = reinterpret_cast<const std::byte*>(41);
    auto p2 = reinterpret_cast<void*>(47);
    auto p3 = reinterpret_cast<const volatile unsigned char*>(102);

    CHECK(p1 + Size::Bytes(0) == p1);
    CHECK(p2 + Size::Bytes(0) == p2);
    CHECK(p3 + Size::Bytes(0) == p3);

    CHECK(p1 - Size::Bytes(0) == p1);
    CHECK(p2 - Size::Bytes(0) == p2);
    CHECK(p3 - Size::Bytes(0) == p3);

    CHECK(p1 + Size::Bytes(1) == p1 + 1);
    CHECK(p2 + Size::Bytes(1) == static_cast<char*>(p2) + 1);
    CHECK(p3 + Size::Bytes(1) == p3 + 1);

    CHECK(p1 - Size::Bytes(1) == p1 - 1);
    CHECK(p2 - Size::Bytes(1) == static_cast<char*>(p2) - 1);
    CHECK(p3 - Size::Bytes(1) == p3 - 1);

    CHECK(p1 + Size::Bytes(37) == p1 + 37);
    CHECK(p2 + Size::Bytes(37) == static_cast<char*>(p2) + 37);
    CHECK(p3 + Size::Bytes(37) == p3 + 37);

    CHECK(p1 - Size::Bytes(37) == p1 - 37);
    CHECK(p2 - Size::Bytes(37) == static_cast<char*>(p2) - 37);
    CHECK(p3 - Size::Bytes(37) == p3 - 37);

    CHECK(p1 + Size::Bits(5) == p1 + 1);
    CHECK(p2 + Size::Bits(5) == static_cast<char*>(p2) + 1);
    CHECK(p3 + Size::Bits(5) == p3 + 1);

    CHECK(p1 - Size::Bits(5) == p1 - 1);
    CHECK(p2 - Size::Bits(5) == static_cast<char*>(p2) - 1);
    CHECK(p3 - Size::Bits(5) == p3 - 1);

    auto p1_copy = p1;
    auto p2_copy = p2;
    auto p3_copy = p3;
    p1 += Size::Bytes(37);
    p2 += Size::Bits(11);
    p3 -= Size::Bytes(65);
    CHECK(p1 == p1_copy + 37);
    CHECK(p2 == static_cast<char*>(p2_copy) + 2);
    CHECK(p3 == p3_copy - 65);
}
