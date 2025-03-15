#include "TestCommon.hh"
#include <base/Size.hh>

using namespace base;

TEST_CASE("Formatter for std::vector") {
    std::vector vec {1, 2, 3, 4, 5};
    CHECK(std::format("{}", vec) == "[1, 2, 3, 4, 5]");
    CHECK(std::format("{}", std::vector<int>{}) == "[]");
}

TEST_CASE("Formatter for Size") {
    CHECK(std::format("{:b}", Size()) == "0");
    CHECK(std::format("{:B}", Size()) == "0");

    CHECK(std::format("{:b}", Size::Bits(6)) == "6");
    CHECK(std::format("{:B}", Size::Bits(6)) == "1");
    CHECK(std::format("{:b}", Size::Bits(127)) == "127");
    CHECK(std::format("{:B}", Size::Bits(127)) == "16");
    CHECK(std::format("{:b}", Size::Bits(128)) == "128");
    CHECK(std::format("{:B}", Size::Bits(128)) == "16");
    CHECK(std::format("{:b}", Size::Bits(129)) == "129");
    CHECK(std::format("{:B}", Size::Bits(129)) == "17");

    CHECK(std::format("{:bh}", Size()) == "0");
    CHECK(std::format("{:Bh}", Size()) == "0");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 10) - 1)) == "1023");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 10) - 1)) == "1023Bit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 10)) == "1K");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 10)) == "1KBit");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 20) - 1)) == "1023K");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 20) - 1)) == "1023KBit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 20)) == "1M");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 20)) == "1MBit");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 30) - 1)) == "1023M");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 30) - 1)) == "1023MBit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 30)) == "1G");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 30)) == "1GBit");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 40) - 1)) == "1023G");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 40) - 1)) == "1023GBit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 40)) == "1T");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 40)) == "1TBit");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 50) - 1)) == "1023T");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 50) - 1)) == "1023TBit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 50)) == "1P");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 50)) == "1PBit");
    CHECK(std::format("{:Bh}", Size::Bytes((u64(1) << 60) - 1)) == "1023P");
    CHECK(std::format("{:bh}", Size::Bits((u64(1) << 60) - 1)) == "1023PBit");
    CHECK(std::format("{:Bh}", Size::Bytes(u64(1) << 60)) == "1E");
    CHECK(std::format("{:bh}", Size::Bits(u64(1) << 60)) == "1EBit");

    Size s;
    CHECK_THROWS(std::vformat("{}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:h}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:hb}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:hB}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:bB}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:Bb}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:bb}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:BB}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:bhh}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:Bhh}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:BH}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:Bq}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:bq}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:qb}", std::make_format_args(s)));
    CHECK_THROWS(std::vformat("{:qB}", std::make_format_args(s)));
}
