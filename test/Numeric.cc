#include "TestCommon.hh"

using namespace base;
using namespace Catch::literals;

template <typename Ty>
using L = std::numeric_limits<Ty>;

#define STR_(X) #X
#define STR(X) STR_(X)

#define OK(F, S) CHECK(F(STR(S)).value() == S)
#define ERR(F, S, E) CHECK(F(S).error() == "Failed to parse number from string '" S "': " E)

TEST_CASE("Log2Floor") {
    utils::list<i8, u8, i16, u16, i32, u32, i64, u64>::each([]<typename T>{
        CHECK(Log2Floor(T(0)) == -1);
        for (T i = 1; i < 100; ++i) CHECK(Log2Floor(i) == std::floor(std::log2(i)));

        // Powers of 2.
        T t = 1;
        for (u32 i = 0; i < L<T>::digits; ++i) {
            CHECK(Log2Floor(t) == i);
            t <<= 1;
        }
    });
}

TEST_CASE("Parse<bool>") {
    CHECK(Parse<bool>("true").value() == true);
    CHECK(Parse<bool>("false").value() == false);
    CHECK(Parse<bool>("foo").error() == "Expected 'true' or 'false', was 'foo'");
    CHECK(Parse<bool>("trues").error() == "Expected 'true' or 'false', was 'trues'");
    CHECK(Parse<bool>("true  ").error() == "Expected 'true' or 'false', was 'true  '");
    CHECK(Parse<bool>(" true").error() == "Expected 'true' or 'false', was ' true'");
}

TEST_CASE("Parse<i8>") {
    OK(Parse<i8>, 0);
    OK(Parse<i8>, 1);
    OK(Parse<i8>, -1);
    OK(Parse<i8>, 127);
    OK(Parse<i8>, -128);
    ERR(Parse<i8>, "128", "Numerical result out of range");
    ERR(Parse<i8>, "-129", "Numerical result out of range");
    ERR(Parse<i8>, "foo", "Invalid argument");
}

TEST_CASE("Parse<i16>") {
    OK(Parse<i16>, 0);
    OK(Parse<i16>, 1);
    OK(Parse<i16>, -1);
    OK(Parse<i16>, 32767);
    OK(Parse<i16>, -32768);
    ERR(Parse<i16>, "32768", "Numerical result out of range");
    ERR(Parse<i16>, "-32769", "Numerical result out of range");
    ERR(Parse<i16>, "foo", "Invalid argument");
}

TEST_CASE("Parse<i32>") {
    OK(Parse<i32>, 0);
    OK(Parse<i32>, 1);
    OK(Parse<i32>, -1);
    OK(Parse<i32>, 2147483647);
    OK(Parse<i32>, -2147483648);
    ERR(Parse<i32>, "2147483648", "Numerical result out of range");
    ERR(Parse<i32>, "-2147483649", "Numerical result out of range");
    ERR(Parse<i32>, "foo", "Invalid argument");
}

TEST_CASE("Parse<i64>") {
    OK(Parse<i64>, 0);
    OK(Parse<i64>, 1);
    OK(Parse<i64>, -1);
    CHECK(Parse<i64>("9223372036854775807").value() == L<i64>::max());
    CHECK(Parse<i64>("-9223372036854775808").value() == L<i64>::min());
    ERR(Parse<i64>, "9223372036854775808", "Numerical result out of range");
    ERR(Parse<i64>, "-9223372036854775809", "Numerical result out of range");
    ERR(Parse<i64>, "foo", "Invalid argument");
}

TEST_CASE("Parse<u8>") {
    OK(Parse<u8>, 0);
    OK(Parse<u8>, 1);
    OK(Parse<u8>, 255);
    ERR(Parse<u8>, "256", "Numerical result out of range");
    ERR(Parse<u8>, "-1", "Invalid argument");
    ERR(Parse<u8>, "foo", "Invalid argument");
}

TEST_CASE("Parse<u16>") {
    OK(Parse<u16>, 0);
    OK(Parse<u16>, 1);
    OK(Parse<u16>, 65535);
    ERR(Parse<u16>, "65536", "Numerical result out of range");
    ERR(Parse<u16>, "-1", "Invalid argument");
    ERR(Parse<u16>, "foo", "Invalid argument");
}

TEST_CASE("Parse<u32>") {
    OK(Parse<u32>, 0);
    OK(Parse<u32>, 1);
    OK(Parse<u32>, 4294967295);
    ERR(Parse<u32>, "4294967296", "Numerical result out of range");
    ERR(Parse<u32>, "-1", "Invalid argument");
    ERR(Parse<u32>, "foo", "Invalid argument");
}

TEST_CASE("Parse<u64>") {
    OK(Parse<u64>, 0);
    OK(Parse<u64>, 1);
    CHECK(Parse<u64>("18446744073709551615").value() == L<u64>::max());
    ERR(Parse<u64>, "18446744073709551616", "Numerical result out of range");
    ERR(Parse<u64>, "-1", "Invalid argument");
    ERR(Parse<u64>, "foo", "Invalid argument");
}

TEST_CASE("Parse<f32>") {
    CHECK(Parse<f32>("0").value() == 0.f);
    CHECK(Parse<f32>("-0").value() == -0.f);
    CHECK(Parse<f32>("1").value() == 1.f);
    CHECK(Parse<f32>("-1").value() == -1.f);
    CHECK(Parse<f32>("3.14159").value() == 3.14159f);
    CHECK(Parse<f32>("-3.14159").value() == -3.14159f);
    CHECK(Parse<f32>(std::format("{}", L<f32>::min())).value() == L<f32>::min());
    CHECK(Parse<f32>(std::format("{}", L<f32>::max())).value() == L<f32>::max());
    CHECK(Parse<f32>("inf").value() == L<f32>::infinity());
    CHECK(Parse<f32>("-inf").value() == -L<f32>::infinity());
    CHECK(std::isnan(Parse<f32>("nan").value()));
}

TEST_CASE("Parse<f64>") {
    CHECK(Parse<f64>("0").value() == 0.);
    CHECK(Parse<f64>("-0").value() == -0.);
    CHECK(Parse<f64>("1").value() == 1.);
    CHECK(Parse<f64>("-1").value() == -1.);
    CHECK(Parse<f64>("3.14159").value() == 3.14159);
    CHECK(Parse<f64>("-3.14159").value() == -3.14159);
    CHECK(Parse<f64>(std::format("{}", L<f64>::min())).value() == L<f64>::min());
    CHECK(Parse<f64>(std::format("{}", L<f64>::max())).value() == L<f64>::max());
    CHECK(Parse<f64>("inf").value() == L<f64>::infinity());
    CHECK(Parse<f64>("-inf").value() == -L<f64>::infinity());
    CHECK(std::isnan(Parse<f64>("nan").value()));
}
