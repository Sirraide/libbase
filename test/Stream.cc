#include "TestCommon.hh"

using namespace base;

using Catch::Matchers::RangeEquals;

struct StringWrapper {
    std::string data;
    void operator +=(std::string_view s) { data += s; }
    bool operator==(std::string_view s) const { return data == s; }
};

template <>
struct Catch::StringMaker<stream> {
    static std::string convert(const stream& res) {
        std::string s;
        for (auto c : res.text()) {
            if (c < 32) {
                s += std::format("\\x{:02x}", c);
            } else {
                s += c;
            }
        }
        return s;
    }
};

using Views = std::vector<std::string_view>;

TEST_CASE("stream(string&)") {
    std::string s;
    CHECK(stream{s}.empty());
    CHECK(stream{s}.size() == 0);
}

TEST_CASE("stream(string_view)") {
    std::string_view s;
    CHECK(stream{s}.empty());
    CHECK(stream{s}.size() == 0);
}

TEST_CASE("stream::back()") {
    std::string s = "hello world";
    CHECK(stream{s}.back() == 'd');
    CHECK(stream{""sv}.back() == std::nullopt);
}

TEST_CASE("stream::char_ptr()") {
    std::string s = "hello world";
    std::u32string su32 = U"hello world";
    CHECK(stream{s}.char_ptr() == s.data());
    CHECK(u32stream{su32}.char_ptr() == reinterpret_cast<const char*>(su32.data()));
}

#ifdef __cpp_lib_ranges_chunk
TEST_CASE("stream::chunks()") {
    std::string s = "hello world";
    CHECK_THAT(stream{s}.chunks(3), RangeEquals(std::vector{"hel"sv, "lo "sv, "wor"sv, "ld"sv}));
    CHECK_THAT(stream{s}.chunks(5), RangeEquals(std::vector{"hello"sv, " worl"sv, "d"sv}));
    CHECK_THAT(stream{s}.chunks(6), RangeEquals(std::vector{"hello "sv, "world"sv}));
}
#endif

TEST_CASE("stream::contains()") {
    std::string s = "hello world";

    CHECK(stream{s}.contains('h'));
    CHECK(stream{s}.contains('w'));
    CHECK(stream{s}.contains(' '));
    CHECK_FALSE(stream{s}.contains('x'));
    CHECK_FALSE(stream{s}.contains('!'));

    CHECK(stream{s}.contains("hello"));
    CHECK(stream{s}.contains("world"));
    CHECK(stream{s}.contains(" "));
    CHECK_FALSE(stream{s}.contains("hellox"));
    CHECK_FALSE(stream{s}.contains("!world"));
}

TEST_CASE("stream::contains_any()") {
    std::string s = "hello world";

    CHECK(stream{s}.contains_any("hxyz"));
    CHECK(stream{s}.contains_any("wxyz"));
    CHECK(stream{s}.contains_any(" !"));
    CHECK_FALSE(stream{s}.contains_any("xyz"));
}

TEST_CASE("stream::consume(char)") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume('h'));
    CHECK(s.consume('e'));
    CHECK(s.consume('l'));
    CHECK_FALSE(s.consume('x'));
    CHECK_FALSE(s.consume('!'));
    CHECK(s.consume('l'));
    CHECK(s == "o world");
}

TEST_CASE("stream::consume(string_view)") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume("hello"));
    CHECK(s.consume(" "));
    CHECK_FALSE(s.consume("x"));
    CHECK_FALSE(s.consume("!"));
    CHECK(s.consume("wor"));
    CHECK(s.consume("ld"));
    CHECK(s.empty());
}

TEST_CASE("stream::consume_any()") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume_any("hxyz"));
    CHECK(s.consume_any("e"));
    CHECK(s.consume_any("l"));
    CHECK_FALSE(s.consume_any("xyz"));
    CHECK_FALSE(s.consume_any(" !"));
    CHECK(s.consume_any("worl"));
    CHECK(s.consume_any("lod"));
    CHECK(s == " world");
}

TEST_CASE("stream::consume_back()") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume_back('d'));
    CHECK(s.consume_back('l'));
    CHECK(s.consume_back('r'));
    CHECK_FALSE(s.consume_back('x'));
    CHECK_FALSE(s.consume_back('!'));
    CHECK(s.consume_back('o'));
    CHECK(s == "hello w");
}

TEST_CASE("stream::consume_back(string_view)") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume_back("world"));
    CHECK(s.consume_back(" "));
    CHECK_FALSE(s.consume_back("x"));
    CHECK_FALSE(s.consume_back("!"));
    CHECK(s.consume_back("hello"));
    CHECK(s.empty());
}

TEST_CASE("stream::consume_back_any()") {
    std::string str = "hello world";
    stream s{str};

    CHECK(s.consume_back_any("dxyz"));
    CHECK(s.consume_back_any("l"));
    CHECK(s.consume_back_any("r"));
    CHECK_FALSE(s.consume_back_any("xyz"));
    CHECK_FALSE(s.consume_back_any(" !"));
    CHECK(s.consume_back_any("worl"));
    CHECK(s.consume_back_any("lodw"));
    CHECK(s == "hello ");
}

TEST_CASE("stream::count()") {
    std::string s = "aaabbbcccdddaacdcdcd";

    CHECK(stream{s}.count('a') == 5);
    CHECK(stream{s}.count('b') == 3);
    CHECK(stream{s}.count('c') == 6);
    CHECK(stream{s}.count('d') == 6);
    CHECK(stream{s}.count('x') == 0);

    CHECK(stream{s}.count("aa") == 2);
    CHECK(stream{s}.count("bb") == 1);
    CHECK(stream{s}.count("cc") == 1);
    CHECK(stream{s}.count("ccc") == 1);
    CHECK(stream{s}.count("cccc") == 0);
    CHECK(stream{s}.count("x") == 0);
}

TEST_CASE("stream::count_any") {
    std::string s = "aaabbbcccdddaacdcdcd";

    CHECK(stream{s}.count_any("abc") == 14);
    CHECK(stream{s}.count_any("xyz") == 0);
    CHECK(stream{s}.count_any("x") == 0);
    CHECK(stream{s}.count_any("xa") == 5);
    CHECK(stream{s}.count_any("cxab") == 14);
    CHECK(stream{s}.count_any("adcb") == 20);
}

TEST_CASE("stream::drop()") {
    std::string s = "hello world";

    CHECK(stream{s}.drop(0) == "hello world");
    CHECK(stream{s}.drop(1) == "ello world");
    CHECK(stream{s}.drop(2) == "llo world");
    CHECK(stream{s}.drop(200) == "");

    CHECK(stream{s}.drop(-0) == "hello world");
    CHECK(stream{s}.drop(-1) == "hello worl");
    CHECK(stream{s}.drop(-2) == "hello wor");
    CHECK(stream{s}.drop(-200) == "");
}

TEST_CASE("stream::drop_back()") {
    std::string s = "hello world";

    CHECK(stream{s}.drop_back(0) == "hello world");
    CHECK(stream{s}.drop_back(1) == "hello worl");
    CHECK(stream{s}.drop_back(2) == "hello wor");
    CHECK(stream{s}.drop_back(200) == "");

    CHECK(stream{s}.drop_back(-0) == "hello world");
    CHECK(stream{s}.drop_back(-1) == "ello world");
    CHECK(stream{s}.drop_back(-2) == "llo world");
    CHECK(stream{s}.drop_back(-200) == "");
}

TEST_CASE("stream::empty()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(not stream{s1}.empty());
    CHECK(stream{s2}.empty());
}

TEST_CASE("stream::ends_with()") {
    std::string s = "hello world";

    CHECK(stream{s}.ends_with('d'));
    CHECK(stream{s}.ends_with("d"));
    CHECK(stream{s}.ends_with("ld"));
    CHECK(stream{s}.ends_with("world"));
    CHECK_FALSE(stream{s}.ends_with("lx"));
    CHECK_FALSE(stream{s}.ends_with("worldx"));
}

TEST_CASE("stream::ends_with_any()") {
    std::string s = "hello world";

    CHECK(stream{s}.ends_with_any("d"));
    CHECK(stream{s}.ends_with_any("ld"));
    CHECK(stream{s}.ends_with_any("world"));
    CHECK(stream{s}.ends_with_any("dx"));
    CHECK(stream{s}.ends_with_any("ldx"));
    CHECK_FALSE(stream{s}.ends_with_any("lx"));
}

TEST_CASE("stream::escape()") {
    CHECK(stream("abcd").escape("b") == "a\\bcd");
    CHECK(stream("a\\bcd").escape("b\\") == "a\\\\\\bcd");
    CHECK(stream("abcd").escape("") == "abcd");
    CHECK(stream("axbxcxd").escape("x") == "a\\xb\\xc\\xd");
    CHECK(stream("").escape("x") == "");
    CHECK(stream("").escape("xafeae3") == "");
    CHECK(stream("a").escape("aaa") == "\\a");
    CHECK(stream(" a ").escape("a") == " \\a ");
    CHECK(stream("\\a").escape("a") == "\\\\a");

    CHECK(stream("abcd").escape("a", "q") == "qabcd");
    CHECK(stream("abcd").escape("abcd", "q") == "qaqbqcqd");
    CHECK(stream("abcd").escape("abcd", "qx") == "qxaqxbqxcqxd");
    CHECK(stream("abcd").escape("abcd", "") == "abcd"); // A bit dumb but valid.
    CHECK(stream("").escape("", "") == "");

    CHECK(u32stream(U"abcd").escape(U"b") == U"a\\bcd");
    CHECK(u32stream(U"a\\bcd").escape(U"b\\") == U"a\\\\\\bcd");
    CHECK(u32stream(U"abcd").escape(U"") == U"abcd");
    CHECK(u32stream(U"axbxcxd").escape(U"x") == U"a\\xb\\xc\\xd");
    CHECK(u32stream(U"").escape(U"x") == U"");
    CHECK(u32stream(U"").escape(U"xafeae3") == U"");
    CHECK(u32stream(U"a").escape(U"aaa") == U"\\a");
    CHECK(u32stream(U" a ").escape(U"a") == U" \\a ");
    CHECK(u32stream(U"\\a").escape(U"a") == U"\\\\a");

    CHECK(u32stream(U"abcd").escape(U"a", U"q") == U"qabcd");
    CHECK(u32stream(U"abcd").escape(U"abcd", U"q") == U"qaqbqcqd");
    CHECK(u32stream(U"abcd").escape(U"abcd", U"qx") == U"qxaqxbqxcqxd");
    CHECK(u32stream(U"abcd").escape(U"abcd", U"") == U"abcd");
    CHECK(u32stream(U"").escape(U"", U"") == U"");

    CHECK(u32stream(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫ") == U"þƿɐɗɐɠ\\ɫʁɦɠ\\ɫʁɦ");
    CHECK(u32stream(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫɐ") == U"þƿ\\ɐɗ\\ɐɠ\\ɫʁɦɠ\\ɫʁɦ");
    CHECK(u32stream(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫɐ", U"ʉ") == U"þƿʉɐɗʉɐɠʉɫʁɦɠʉɫʁɦ");
}

TEST_CASE("stream::extract()") {
    char c1{}, c2{}, c3{}, c4{};
    std::string s1 = "";
    std::string s2 = "hel";
    std::string s3 = "lo w";

    CHECK(not stream{s1}.extract(c1, c2, c3, c4));
    CHECK((c1 == 0 and c2 == 0 and c3 == 0 and c4 == 0));
    CHECK(not stream{s2}.extract(c1, c2, c3, c4));
    CHECK((c1 == 0 and c2 == 0 and c3 == 0 and c4 == 0));
    CHECK(stream{s3}.extract(c1, c2, c3, c4));
    CHECK((c1 == 'l' and c2 == 'o' and c3 == ' ' and c4 == 'w'));
}

TEST_CASE("stream::fold_any()") {
    std::string s = "abcdaabbccddabcd";

    CHECK(stream{s}.fold_any("", 'q') == s);
    CHECK(stream{s}.fold_any("xyzw", 'q') == s);
    CHECK(stream{s}.fold_any("a", 'a') == "abcdabbccddabcd");
    CHECK(stream{s}.fold_any("a", 'q') == "qbcdqbbccddqbcd");
    CHECK(stream{s}.fold_any("a", "q") == "qbcdqbbccddqbcd");
    CHECK(stream{s}.fold_any("abcd", 'q') == "q");
    CHECK(stream{s}.fold_any("dbca", 'q') == "q");
    CHECK(stream{s}.fold_any("ab", "q") == "qcdqccddqcd");
    CHECK(stream{s}.fold_any("ad", "q") == "qbcqbbccqbcq");
}

TEST_CASE("stream::fold_ws()") {
    auto s = "   a  b\t\t c\td\r\v\f\ne \r \v \n f  "s;
    CHECK(stream{s}.fold_ws() == " a b c d e f ");
    CHECK(stream{s}.fold_ws("q") == "qaqbqcqdqeqfq");
}

TEST_CASE("stream::front()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(stream{s1}.front() == 'h');
    CHECK(stream{s2}.front() == std::nullopt);
}

TEST_CASE("stream::has()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(stream{s1}.has(0));
    CHECK(stream{s1}.has(1));
    CHECK(stream{s1}.has(2));
    CHECK(stream{s1}.has(6));
    CHECK(stream{s1}.has(11));
    CHECK(not stream{s1}.has(12));

    CHECK(stream{s2}.has(0));
    CHECK(not stream{s2}.has(1));
}

TEST_CASE("stream::remove()") {
    std::string s = "hello world";

    CHECK(stream{s}.remove('l') == "heo word");
    CHECK(stream{s}.remove_all("l") == "heo word");
    CHECK(stream{s}.remove_all("lo") == "he wrd");
    CHECK(stream{s}.remove_all("ol") == "he wrd");
    CHECK(stream{s}.remove('x') == "hello world");
    CHECK(stream{s}.remove_all("xyz") == "hello world");

    CHECK(stream{s}.remove<StringWrapper>('l') == "heo word");
    CHECK(stream{s}.remove_all<StringWrapper>("l") == "heo word");
    CHECK(stream{s}.remove_all<StringWrapper>("lo") == "he wrd");
    CHECK(stream{s}.remove_all<StringWrapper>("ol") == "he wrd");
    CHECK(stream{s}.remove<StringWrapper>('x') == "hello world");
    CHECK(stream{s}.remove_all<StringWrapper>("xyz") == "hello world");
}

TEST_CASE("stream::replace()") {
    stream s1 = "hello world";
    stream s2 = "aaa foo bbb aaa";

    CHECK(s1.replace('l', 'X') == "heXXo worXd");
    CHECK(s1.replace("l", "X") == "heXXo worXd");
    CHECK(s1.replace("lo", "X") == "helX world");
    CHECK(s1.replace("ol", 'X') == "hello world");
    CHECK(s1.replace('o', 'o') == "hello world");
    CHECK(s1.replace('q', "he") == "hello world");

    CHECK(s2.replace('a', 'X') == "XXX foo bbb XXX");
    CHECK(s2.replace("aaa", "X") == "X foo bbb X");
    CHECK(s2.replace("aaa", 'X') == "X foo bbb X");
    CHECK(s2.replace("aaa", "aaaaaa") == "aaaaaa foo bbb aaaaaa");
    CHECK(s2.replace("aaa", "ccc") == "ccc foo bbb ccc");
}

TEST_CASE("stream::replace_many()") {
    stream s1 = "hello world";
    stream s2 = "aaa foo bbb aaa";

    CHECK(s1.replace_many("h", "X") == "Xello world");
    CHECK(s1.replace_many("hl", "XX") == "XeXXo worXd");
    CHECK(s1.replace_many("hl", "XY") == "XeYYo worYd");
    CHECK(s1.replace_many("hl", "X") == "Xeo word");
    CHECK(s1.replace_many("hl", "") == "eo word");
    CHECK(s1.replace_many("hh", "XY") == "Xello world");

    CHECK(s2.replace_many("afob", "") == "   ");
    CHECK(s2.replace_many("afob ", "12345") == "111523354445111");
    CHECK(s1.replace_many("h", "123") == "1ello world");
    CHECK(s1.replace_many("", "123") == "hello world");
    CHECK(s1.replace_many("", "") == "hello world");
}

TEST_CASE("stream::size()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(stream{s1}.size() == 11);
    CHECK(stream{s2}.size() == 0);
}

TEST_CASE("stream::size_bytes()") {
    std::u32string s1 = U"hello world";
    std::u32string s2;

    CHECK(u32stream{s1}.size_bytes() == s1.size() * sizeof(char32_t));
    CHECK(u32stream{s2}.size_bytes() == 0);
}

TEST_CASE("stream::split()") {
    std::string s = "aa\nbb\ncc\nbb\ncc";

    SECTION ("1") {
        auto lines = stream{s}.lines();
        REQUIRE(rgs::distance(lines) == 5);
        CHECK_THAT(lines, RangeEquals(std::vector{"aa"sv, "bb"sv, "cc"sv, "bb"sv, "cc"sv}));
    }

    SECTION ("2") {
        auto bb = stream{s}.split("bb");
        REQUIRE(rgs::distance(bb) == 3);
        CHECK_THAT(bb, RangeEquals(std::vector{"aa\n"sv, "\ncc\n"sv, "\ncc"sv}));
    }

    SECTION ("3") {
        auto cc = stream{s}.split("cc");
        REQUIRE(rgs::distance(cc) == 3);
        CHECK_THAT(cc, RangeEquals(std::vector{"aa\nbb\n"sv, "\nbb\n"sv, ""sv}));
    }
}

TEST_CASE("stream::starts_with") {
    std::string s = "hello world";

    CHECK(stream{s}.starts_with('h'));
    CHECK(stream{s}.starts_with("h"));
    CHECK(stream{s}.starts_with("he"));
    CHECK(stream{s}.starts_with("hello"));
    CHECK_FALSE(stream{s}.starts_with("hx"));
    CHECK_FALSE(stream{s}.starts_with("xhello"));
}

TEST_CASE("stream::starts_with_any") {
    std::string s = "hello world";

    CHECK(stream{s}.starts_with_any("h"));
    CHECK(stream{s}.starts_with_any("eh"));
    CHECK(stream{s}.starts_with_any("hello"));
    CHECK(stream{s}.starts_with_any("hx"));
    CHECK(stream{s}.starts_with_any("xhello"));
    CHECK_FALSE(stream{s}.starts_with_any("x"));
}

TEST_CASE("stream::take_until(_any)(_and_drop)") {
    std::string s = "hello world";
    const Views v{"lx", "llox"};

    SECTION("take_until") {
        CHECK(stream{s}.take_until(' ') == "hello"sv);
        CHECK(stream{s}.take_until('o') == "hell"sv);
        CHECK(stream{s}.take_until('x') == "hello world"sv);
        CHECK(stream{s}.take_until('h') == ""sv);

        CHECK(stream{s}.take_until("") == ""sv);
        CHECK(stream{s}.take_until(" ") == "hello"sv);
        CHECK(stream{s}.take_until("lo") == "hel"sv);
        CHECK(stream{s}.take_until("ld") == "hello wor"sv);
        CHECK(stream{s}.take_until("lx") == "hello world"sv);

        CHECK(stream{s}.take_until([](char c) { return c == ' '; }) == "hello"sv);
        CHECK(stream{s}.take_until([](char c) { return c == 'o'; }) == "hell"sv);
        CHECK(stream{s}.take_until([](char c) { return c == 'x'; }) == "hello world"sv);
        CHECK(stream{s}.take_until([](char c) { return c == 'h'; }) == ""sv);
    }

    SECTION("any") {
        CHECK(stream{s}.take_until_any("") == "hello world"sv);
        CHECK(stream{s}.take_until_any(" ") == "hello"sv);
        CHECK(stream{s}.take_until_any("eo") == "h"sv);
        CHECK(stream{s}.take_until_any("x") == "hello world"sv);
        CHECK(stream{s}.take_until_any("rw") == "hello "sv);

        CHECK(stream{s}.take_until_any(Views{}) == "hello world"sv);
        CHECK(stream{s}.take_until_any(Views{"e", "o"}) == "h"sv);
        CHECK(stream{s}.take_until_any(Views{"lo", "el"}) == "h"sv);
        CHECK(stream{s}.take_until_any(Views{"lo", "llo"}) == "he"sv);
        CHECK(stream{s}.take_until_any(v) == "hello world"sv);
        CHECK(stream{s}.take_until_any(v | vws::transform([](auto x) { return x; })) == "hello world"sv);
        CHECK(stream{s}.take_until_any(v | vws::filter([](auto x) { return true; })) == "hello world"sv);
    }

    SECTION("and_drop") {
        stream s1{s};
        CHECK(s1.take_until_and_drop("wo") == "hello "sv);
        CHECK(s1 == "rld");

        s1 = s;
        CHECK(s1.take_until_and_drop("") == ""sv);
        CHECK(s1 == "hello world");

        s1 = s;
        CHECK(s1.take_until_and_drop('o') == "hell"sv);
        CHECK(s1 == " world");

        s1 = s;
        CHECK(s1.take_until_and_drop('x') == "hello world"sv);
        CHECK(s1 == "");

        s1 = s;
        CHECK(s1.take_until_and_drop('h') == ""sv);
        CHECK(s1 == "ello world");

        s1 = s;
        CHECK(s1.take_until_and_drop(std::string{"l"}) == "he"sv);
        CHECK(s1 == "lo world");
    }

    SECTION("and_take") {
        stream s1{s};
        CHECK(s1.take_until_and_take("wo") == "hello wo"sv);
        CHECK(s1 == "rld");

        s1 = s;
        CHECK(s1.take_until_and_take("") == ""sv);
        CHECK(s1 == "hello world");

        s1 = s;
        CHECK(s1.take_until_and_take('o') == "hello"sv);
        CHECK(s1 == " world");

        s1 = s;
        CHECK(s1.take_until_and_take('x') == "hello world"sv);
        CHECK(s1 == "");

        s1 = s;
        CHECK(s1.take_until_and_take('h') == "h"sv);
        CHECK(s1 == "ello world");

        s1 = s;
        CHECK(s1.take_until_and_take(std::string{"l"}) == "hel"sv);
        CHECK(s1 == "lo world");
    }
}

TEST_CASE("stream::take_until_or_empty") {
    std::string s = "hello world";
    const Views v{"lx", "llox"};

    CHECK(stream{s}.take_until_or_empty(' ') == "hello"sv);
    CHECK(stream{s}.take_until_or_empty('o') == "hell"sv);
    CHECK(stream{s}.take_until_or_empty('x') == ""sv);
    CHECK(stream{s}.take_until_or_empty('h') == ""sv);

    CHECK(stream{s}.take_until_any_or_empty(" ") == "hello"sv);
    CHECK(stream{s}.take_until_any_or_empty("eo") == "h"sv);
    CHECK(stream{s}.take_until_any_or_empty("x") == ""sv);
    CHECK(stream{s}.take_until_any_or_empty("rw") == "hello "sv);

    CHECK(stream{s}.take_until_or_empty(" ") == "hello"sv);
    CHECK(stream{s}.take_until_or_empty("lo") == "hel"sv);
    CHECK(stream{s}.take_until_or_empty("ld") == "hello wor"sv);
    CHECK(stream{s}.take_until_or_empty("lx") == ""sv);

    CHECK(stream{s}.take_until_any_or_empty(Views{}) == ""sv);
    CHECK(stream{s}.take_until_any_or_empty(Views{"e", "o"}) == "h"sv);
    CHECK(stream{s}.take_until_any_or_empty(Views{"lo", "el"}) == "h"sv);
    CHECK(stream{s}.take_until_any_or_empty(Views{"lo", "llo"}) == "he"sv);
    CHECK(stream{s}.take_until_any_or_empty(v) == ""sv);
    CHECK(stream{s}.take_until_any_or_empty(v | vws::transform([](auto x) { return x; })) == ""sv);
    CHECK(stream{s}.take_until_any_or_empty(v | vws::filter([](auto x) { return true; })) == ""sv);

    CHECK(stream{s}.take_until_or_empty([](char c) { return c == ' '; }) == "hello"sv);
    CHECK(stream{s}.take_until_or_empty([](char c) { return c == 'o'; }) == "hell"sv);
    CHECK(stream{s}.take_until_or_empty([](char c) { return c == 'x'; }) == ""sv);
    CHECK(stream{s}.take_until_or_empty([](char c) { return c == 'h'; }) == ""sv);
}

TEST_CASE("stream::take_until_ws") {
    stream s = "a  b\t\t c\td\r\v\f\ne \r \v \n f"sv;
    CHECK(s.take_until_ws() == "a"sv);
    s.trim_front();
    CHECK(s.take_until_ws() == "b"sv);
    s.trim_front();
    CHECK(s.take_until_ws() == "c"sv);
    s.trim_front();
    CHECK(s.take_until_ws() == "d"sv);
    s.trim_front();
    CHECK(s.take_until_ws() == "e"sv);
    s.trim_front();
    CHECK(s.take_until_ws() == "f"sv);
    CHECK(s.empty());
}

TEST_CASE("stream::take_while") {
    std::string s = "hello world";

    CHECK(stream{s}.take_while(' ') == ""sv);
    CHECK(stream{s}.take_while('h') == "h"sv);
    CHECK(stream{s}.take_while('o') == ""sv);
    CHECK(stream{s}.take_while('x') == ""sv);

    CHECK(stream{s}.take_while_any(" ") == ""sv);
    CHECK(stream{s}.take_while_any("eho") == "he"sv);
    CHECK(stream{s}.take_while_any("x") == ""sv);
    CHECK(stream{s}.take_while_any("w loeh") == "hello wo"sv);

    CHECK(stream{s}.take_while([](char c) { return c == ' '; }) == ""sv);
    CHECK(stream{s}.take_while([](char c) { return "eho"sv.contains(c); }) == "he"sv);
    CHECK(stream{s}.take_while([](char c) { return c == 'x'; }) == ""sv);
    CHECK(stream{s}.take_while([](char c) { return "w loeh"sv.contains(c); }) == "hello wo"sv);
}

TEST_CASE("stream::take_back_until") {
    std::string s = "hello world";

    CHECK(stream{s}.take_back_until(' ') == "world"sv);
    CHECK(stream{s}.drop_back_until(' ') == "hello "sv);

    CHECK(stream{s}.take_back_until('o') == "rld"sv);
    CHECK(stream{s}.drop_back_until('o') == "hello wo"sv);

    CHECK(stream{s}.take_back_until('x') == "hello world"sv);
    CHECK(stream{s}.drop_back_until('x') == ""sv);

    CHECK(stream{s}.take_back_until('h') == "ello world"sv);
    CHECK(stream{s}.drop_back_until('h') == "h"sv);

    CHECK(stream{s}.take_back_until('d') == ""sv);
    CHECK(stream{s}.drop_back_until('d') == "hello world"sv);

    CHECK(stream{s}.take_back_until_any(" ") == "world"sv);
    CHECK(stream{s}.drop_back_until_any(" ") == "hello "sv);

    CHECK(stream{s}.take_back_until_any("eo") == "rld"sv);
    CHECK(stream{s}.drop_back_until_any("eo") == "hello wo"sv);

    CHECK(stream{s}.take_back_until_any("x") == "hello world"sv);
    CHECK(stream{s}.drop_back_until_any("x") == ""sv);

    CHECK(stream{s}.take_back_until_any("rw") == "ld"sv);
    CHECK(stream{s}.drop_back_until_any("rw") == "hello wor"sv);

    CHECK(stream{s}.take_back_until_any("ld") == ""sv);
    CHECK(stream{s}.drop_back_until_any("ld") == "hello world"sv);

    CHECK(stream{s}.take_back_until(" ") == "world"sv);
    CHECK(stream{s}.drop_back_until(" ") == "hello "sv);

    CHECK(stream{s}.take_back_until("lo") == " world"sv);
    CHECK(stream{s}.drop_back_until("lo") == "hello"sv);

    CHECK(stream{s}.take_back_until("ld") == ""sv);
    CHECK(stream{s}.drop_back_until("ld") == "hello world"sv);

    CHECK(stream{s}.take_back_until("lx") == "hello world"sv);
    CHECK(stream{s}.drop_back_until("lx") == ""sv);
}

TEST_CASE("stream::take_back_until_or_empty") {
    std::string s = "hello world";

    CHECK(stream{s}.take_back_until_or_empty(' ') == "world"sv);
    CHECK(stream{s}.drop_back_until_or_empty(' ') == "hello "sv);

    CHECK(stream{s}.take_back_until_or_empty('o') == "rld"sv);
    CHECK(stream{s}.drop_back_until_or_empty('o') == "hello wo"sv);

    CHECK(stream{s}.take_back_until_or_empty('x') == ""sv);
    CHECK(stream{s}.drop_back_until_or_empty('x') == "hello world"sv);

    CHECK(stream{s}.take_back_until_or_empty('h') == "ello world"sv);
    CHECK(stream{s}.drop_back_until_or_empty('h') == "h"sv);

    CHECK(stream{s}.take_back_until_or_empty('d') == ""sv);
    CHECK(stream{s}.drop_back_until_or_empty('d') == "hello world"sv);

    CHECK(stream{s}.take_back_until_any_or_empty(" ") == "world"sv);
    CHECK(stream{s}.drop_back_until_any_or_empty(" ") == "hello "sv);

    CHECK(stream{s}.take_back_until_any_or_empty("eo") == "rld"sv);
    CHECK(stream{s}.drop_back_until_any_or_empty("eo") == "hello wo"sv);

    CHECK(stream{s}.take_back_until_any_or_empty("x") == ""sv);
    CHECK(stream{s}.drop_back_until_any_or_empty("x") == "hello world"sv);

    CHECK(stream{s}.take_back_until_any_or_empty("rw") == "ld"sv);
    CHECK(stream{s}.drop_back_until_any_or_empty("rw") == "hello wor"sv);

    CHECK(stream{s}.take_back_until_any_or_empty("ld") == ""sv);
    CHECK(stream{s}.drop_back_until_any_or_empty("ld") == "hello world"sv);

    CHECK(stream{s}.take_back_until_or_empty(" ") == "world"sv);
    CHECK(stream{s}.drop_back_until_or_empty(" ") == "hello "sv);

    CHECK(stream{s}.take_back_until_or_empty("lo") == " world"sv);
    CHECK(stream{s}.drop_back_until_or_empty("lo") == "hello"sv);

    CHECK(stream{s}.take_back_until_or_empty("ld") == ""sv);
    CHECK(stream{s}.drop_back_until_or_empty("ld") == "hello world"sv);

    CHECK(stream{s}.take_back_until_or_empty("lx") == ""sv);
    CHECK(stream{s}.drop_back_until_or_empty("lx") == "hello world"sv);
}

TEST_CASE("stream::take_back_until: whitespace") {
    constexpr std::string_view ws = " \t\r\n\f\v";
    std::string str{"foo  bar\tbaz\r\nquux\fbar"};
    stream s{str};

    CHECK(s.take_back_until_any(ws) == "bar"sv);
    CHECK(RawString{std::string{s.text()}} == "foo  bar\tbaz\r\nquux\f"_raw);
    CHECK(s.consume_back('\f'));
    CHECK(s.take_back_until_any(ws) == "quux"sv);
    CHECK(s.consume_back('\n'));
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(s.consume_back('\r'));
    CHECK(s.take_back_until_any(ws) == "baz"sv);
    CHECK(s.consume_back('\t'));
    CHECK(s.take_back_until_any(ws) == "bar"sv);
    CHECK(RawString{std::string{s.text()}} == "foo  "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s.text()}} == "foo "_raw);
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(RawString{std::string{s.text()}} == "foo "_raw);
    CHECK(s.consume_back(' '));
    CHECK(s.take_back_until_any(ws) == "foo"sv);
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(s.empty());

    s = str;
    CHECK(s.take_back_until_any_or_empty(ws) == "bar"sv);
    CHECK(RawString{std::string{s.text()}} == "foo  bar\tbaz\r\nquux\f"_raw);
    CHECK(s.consume_back('\f'));
    CHECK(s.take_back_until_any_or_empty(ws) == "quux"sv);
    CHECK(s.consume_back('\n'));
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s.consume_back('\r'));
    CHECK(s.take_back_until_any_or_empty(ws) == "baz"sv);
    CHECK(s.consume_back('\t'));
    CHECK(s.take_back_until_any_or_empty(ws) == "bar"sv);
    CHECK(RawString{std::string{s.text()}} == "foo  "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s.text()}} == "foo "_raw);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(RawString{std::string{s.text()}} == "foo "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s.text()}} == "foo"_raw);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s == "foo");
}

TEST_CASE("stream::operator<=>") {
    stream s1{"hello world"};
    stream s2{"hello world"};
    stream s3{"hello"};

    CHECK(s1 == s2);
    CHECK(s1 != s3);
    CHECK(s1 == s2.text());
    CHECK(s1 != s3.text());
    CHECK(s1.text() == s2);
    CHECK(s1.text() != s3);
}
