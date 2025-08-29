#include "TestCommon.hh"

#include <base/Text.hh>

using namespace base;

using Catch::Matchers::RangeEquals;

struct StringWrapper {
    std::string data;
    void operator +=(std::string_view s) { data += s; }
    bool operator==(std::string_view s) const { return data == s; }
};

template <>
struct Catch::StringMaker<str> {
    static std::string convert(str res) {
        std::string s;
        for (auto c : res) {
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
    CHECK(str{s}.empty());
    CHECK(str{s}.size() == 0);
}

TEST_CASE("stream(string_view)") {
    std::string_view s;
    CHECK(str{s}.empty());
    CHECK(str{s}.size() == 0);
}

TEST_CASE("str::back()") {
    std::string s = "hello world";
    CHECK(str{s}.back() == 'd');
    CHECK(str{""sv}.back() == std::nullopt);
}

TEST_CASE("str iterators") {
    str s{"abcd"};
    CHECK(*s.begin() == 'a');
    CHECK(*s.cbegin() == 'a');
    CHECK(*s.rbegin() == 'd');
    CHECK(*std::prev(s.end()) == 'd');
    CHECK(*std::prev(s.cend()) == 'd');
    CHECK(*std::prev(s.rend()) == 'a');

    std::string s2;
    for (char c : s) s2 += c;
    CHECK(s == s2);
}

TEST_CASE("str::char_ptr()") {
    std::string s = "hello world";
    std::u32string su32 = U"hello world";
    CHECK(str{s}.char_ptr() == s.data());
    CHECK(str32{su32}.char_ptr() == reinterpret_cast<const char*>(su32.data()));
}

#ifdef __cpp_lib_ranges_chunk
TEST_CASE("str::chunks()") {
    std::string s = "hello world";
    CHECK_THAT(str{s}.chunks(3), RangeEquals(std::vector{"hel"sv, "lo "sv, "wor"sv, "ld"sv}));
    CHECK_THAT(str{s}.chunks(5), RangeEquals(std::vector{"hello"sv, " worl"sv, "d"sv}));
    CHECK_THAT(str{s}.chunks(6), RangeEquals(std::vector{"hello "sv, "world"sv}));
}
#endif

TEST_CASE("str::contains()") {
    std::string s = "hello world";

    CHECK(str{s}.contains('h'));
    CHECK(str{s}.contains('w'));
    CHECK(str{s}.contains(' '));
    CHECK_FALSE(str{s}.contains('x'));
    CHECK_FALSE(str{s}.contains('!'));

    CHECK(str{s}.contains("hello"));
    CHECK(str{s}.contains("world"));
    CHECK(str{s}.contains(" "));
    CHECK_FALSE(str{s}.contains("hellox"));
    CHECK_FALSE(str{s}.contains("!world"));
}

TEST_CASE("str::contains_any()") {
    std::string s = "hello world";

    CHECK(str{s}.contains_any("hxyz"));
    CHECK(str{s}.contains_any("wxyz"));
    CHECK(str{s}.contains_any(" !"));
    CHECK_FALSE(str{s}.contains_any("xyz"));
}

TEST_CASE("str::consume(char)") {
    str s{"hello world"};

    CHECK(s.consume('h'));
    CHECK(s.consume('e'));
    CHECK(s.consume('l'));
    CHECK_FALSE(s.consume('x'));
    CHECK_FALSE(s.consume('!'));
    CHECK(s.consume('l'));
    CHECK(s == "o world");
}

TEST_CASE("str::consume(string_view)") {
    str s{"hello world"};

    CHECK(s.consume("hello"));
    CHECK(s.consume(" "));
    CHECK_FALSE(s.consume("x"));
    CHECK_FALSE(s.consume("!"));
    CHECK(s.consume("wor"));
    CHECK(s.consume("ld"));
    CHECK(s.empty());
}

TEST_CASE("str::consume_any()") {
    str s{"hello world"};

    CHECK(s.consume_any("hxyz"));
    CHECK(s.consume_any("e"));
    CHECK(s.consume_any("l"));
    CHECK_FALSE(s.consume_any("xyz"));
    CHECK_FALSE(s.consume_any(" !"));
    CHECK(s.consume_any("worl"));
    CHECK(s.consume_any("lod"));
    CHECK(s == " world");
}

TEST_CASE("str::consume_back()") {
    str s{"hello world"};

    CHECK(s.consume_back('d'));
    CHECK(s.consume_back('l'));
    CHECK(s.consume_back('r'));
    CHECK_FALSE(s.consume_back('x'));
    CHECK_FALSE(s.consume_back('!'));
    CHECK(s.consume_back('o'));
    CHECK(s == "hello w");
}

TEST_CASE("str::consume_back(string_view)") {
    str s{"hello world"};

    CHECK(s.consume_back("world"));
    CHECK(s.consume_back(" "));
    CHECK_FALSE(s.consume_back("x"));
    CHECK_FALSE(s.consume_back("!"));
    CHECK(s.consume_back("hello"));
    CHECK(s.empty());
}

TEST_CASE("str::consume_back_any()") {
    str s{"hello world"};

    CHECK(s.consume_back_any("dxyz"));
    CHECK(s.consume_back_any("l"));
    CHECK(s.consume_back_any("r"));
    CHECK_FALSE(s.consume_back_any("xyz"));
    CHECK_FALSE(s.consume_back_any(" !"));
    CHECK(s.consume_back_any("worl"));
    CHECK(s.consume_back_any("lodw"));
    CHECK(s == "hello ");
}

TEST_CASE("str::count()") {
    std::string s = "aaabbbcccdddaacdcdcd";

    CHECK(str{s}.count('a') == 5);
    CHECK(str{s}.count('b') == 3);
    CHECK(str{s}.count('c') == 6);
    CHECK(str{s}.count('d') == 6);
    CHECK(str{s}.count('x') == 0);

    CHECK(str{s}.count("aa") == 2);
    CHECK(str{s}.count("bb") == 1);
    CHECK(str{s}.count("cc") == 1);
    CHECK(str{s}.count("ccc") == 1);
    CHECK(str{s}.count("cccc") == 0);
    CHECK(str{s}.count("x") == 0);
}

TEST_CASE("str::count_any") {
    std::string s = "aaabbbcccdddaacdcdcd";

    CHECK(str{s}.count_any("abc") == 14);
    CHECK(str{s}.count_any("xyz") == 0);
    CHECK(str{s}.count_any("x") == 0);
    CHECK(str{s}.count_any("xa") == 5);
    CHECK(str{s}.count_any("cxab") == 14);
    CHECK(str{s}.count_any("adcb") == 20);
}

TEST_CASE("str::data()") {
    std::string_view s1 = "abcd";
    str s{s1};
    CHECK(s.data() == s1.data());
    CHECK(s.char_ptr() == s1.data());
}

TEST_CASE("str::drop()") {
    std::string s = "hello world";

    CHECK(str{s}.drop(0) == "hello world");
    CHECK(str{s}.drop(1) == "ello world");
    CHECK(str{s}.drop(2) == "llo world");
    CHECK(str{s}.drop(200) == "");

    CHECK(str{s}.drop(-0) == "hello world");
    CHECK(str{s}.drop(-1) == "hello worl");
    CHECK(str{s}.drop(-2) == "hello wor");
    CHECK(str{s}.drop(-200) == "");
}

TEST_CASE("str::drop_back()") {
    std::string s = "hello world";

    CHECK(str{s}.drop_back(0) == "hello world");
    CHECK(str{s}.drop_back(1) == "hello worl");
    CHECK(str{s}.drop_back(2) == "hello wor");
    CHECK(str{s}.drop_back(200) == "");

    CHECK(str{s}.drop_back(-0) == "hello world");
    CHECK(str{s}.drop_back(-1) == "ello world");
    CHECK(str{s}.drop_back(-2) == "llo world");
    CHECK(str{s}.drop_back(-200) == "");
}

TEST_CASE("str::empty()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(not str{s1}.empty());
    CHECK(str{s2}.empty());
}

TEST_CASE("str::ends_with()") {
    std::string s = "hello world";

    CHECK(str{s}.ends_with('d'));
    CHECK(str{s}.ends_with("d"));
    CHECK(str{s}.ends_with("ld"));
    CHECK(str{s}.ends_with("world"));
    CHECK_FALSE(str{s}.ends_with("lx"));
    CHECK_FALSE(str{s}.ends_with("worldx"));
}

TEST_CASE("str::ends_with_any()") {
    std::string s = "hello world";

    CHECK(str{s}.ends_with_any("d"));
    CHECK(str{s}.ends_with_any("ld"));
    CHECK(str{s}.ends_with_any("world"));
    CHECK(str{s}.ends_with_any("dx"));
    CHECK(str{s}.ends_with_any("ldx"));
    CHECK_FALSE(str{s}.ends_with_any("lx"));
}

TEST_CASE("str::escape()") {
    CHECK(str("abcd").escape("b") == "a\\bcd");
    CHECK(str("a\\bcd").escape("b\\") == "a\\\\\\bcd");
    CHECK(str("abcd").escape("") == "abcd");
    CHECK(str("axbxcxd").escape("x") == "a\\xb\\xc\\xd");
    CHECK(str("").escape("x") == "");
    CHECK(str("").escape("xafeae3") == "");
    CHECK(str("a").escape("aaa") == "\\a");
    CHECK(str(" a ").escape("a") == " \\a ");
    CHECK(str("\\a").escape("a") == "\\\\a");

    CHECK(str("abcd").escape("a", "q") == "qabcd");
    CHECK(str("abcd").escape("abcd", "q") == "qaqbqcqd");
    CHECK(str("abcd").escape("abcd", "qx") == "qxaqxbqxcqxd");
    CHECK(str("abcd").escape("abcd", "") == "abcd"); // A bit dumb but valid.
    CHECK(str("").escape("", "") == "");

    CHECK(str32(U"abcd").escape(U"b") == U"a\\bcd");
    CHECK(str32(U"a\\bcd").escape(U"b\\") == U"a\\\\\\bcd");
    CHECK(str32(U"abcd").escape(U"") == U"abcd");
    CHECK(str32(U"axbxcxd").escape(U"x") == U"a\\xb\\xc\\xd");
    CHECK(str32(U"").escape(U"x") == U"");
    CHECK(str32(U"").escape(U"xafeae3") == U"");
    CHECK(str32(U"a").escape(U"aaa") == U"\\a");
    CHECK(str32(U" a ").escape(U"a") == U" \\a ");
    CHECK(str32(U"\\a").escape(U"a") == U"\\\\a");

    CHECK(str32(U"abcd").escape(U"a", U"q") == U"qabcd");
    CHECK(str32(U"abcd").escape(U"abcd", U"q") == U"qaqbqcqd");
    CHECK(str32(U"abcd").escape(U"abcd", U"qx") == U"qxaqxbqxcqxd");
    CHECK(str32(U"abcd").escape(U"abcd", U"") == U"abcd");
    CHECK(str32(U"").escape(U"", U"") == U"");

    CHECK(str32(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫ") == U"þƿɐɗɐɠ\\ɫʁɦɠ\\ɫʁɦ");
    CHECK(str32(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫɐ") == U"þƿ\\ɐɗ\\ɐɠ\\ɫʁɦɠ\\ɫʁɦ");
    CHECK(str32(U"þƿɐɗɐɠɫʁɦɠɫʁɦ").escape(U"ɫɐ", U"ʉ") == U"þƿʉɐɗʉɐɠʉɫʁɦɠʉɫʁɦ");
}

TEST_CASE("str::extract()") {
    char c1{}, c2{}, c3{}, c4{};
    std::string s1 = "";
    std::string s2 = "hel";
    std::string s3 = "lo w";

    CHECK(not str{s1}.extract(c1, c2, c3, c4));
    CHECK((c1 == 0 and c2 == 0 and c3 == 0 and c4 == 0));
    CHECK(not str{s2}.extract(c1, c2, c3, c4));
    CHECK((c1 == 0 and c2 == 0 and c3 == 0 and c4 == 0));
    CHECK(str{s3}.extract(c1, c2, c3, c4));
    CHECK((c1 == 'l' and c2 == 'o' and c3 == ' ' and c4 == 'w'));
}

TEST_CASE("str::first()") {
    str s = "abcd abcd abcd";

    CHECK(s.first('a') == 0);
    CHECK(s.first('b') == 1);
    CHECK(s.first('c') == 2);
    CHECK(s.first('d') == 3);
    CHECK(s.first('e') == std::nullopt);

    CHECK(s.first("a") == 0);
    CHECK(s.first("ab") == 0);
    CHECK(s.first("abc") == 0);
    CHECK(s.first("abcd abcd abcd") == 0);
    CHECK(s.first("abcd abcd abcdx") == std::nullopt);
    CHECK(s.first("") == 0);

    CHECK(s.first(text::IsAlnum) == 0);
    CHECK(s.first(text::IsSpace) == 4);
    CHECK(s.first([](char c) { return c == 'd'; }) == 3);

    CHECK(str().first('a') == std::nullopt);
    CHECK(str().first("a") == std::nullopt);
    CHECK(str().first(text::IsAlnum) == std::nullopt);
    CHECK(str().first("") == 0);
}

TEST_CASE("str::first_any()") {
    str s = "abcd abcd abcd";

    CHECK(s.first_any("abcd") == 0);
    CHECK(s.first_any("bc") == 1);
    CHECK(s.first_any("db") == 1);
    CHECK(s.first_any(" ") == 4);
    CHECK(s.first_any("") == std::nullopt);

    CHECK(str().first_any("abcd") == std::nullopt);
    CHECK(str().first_any("bc") == std::nullopt);
    CHECK(str().first_any("db") == std::nullopt);
    CHECK(str().first_any(" ") == std::nullopt);
    CHECK(str().first_any("") == std::nullopt);
}

TEST_CASE("str::fold_any()") {
    std::string s = "abcdaabbccddabcd";

    CHECK(str{s}.fold_any("", 'q') == s);
    CHECK(str{s}.fold_any("xyzw", 'q') == s);
    CHECK(str{s}.fold_any("a", 'a') == "abcdabbccddabcd");
    CHECK(str{s}.fold_any("a", 'q') == "qbcdqbbccddqbcd");
    CHECK(str{s}.fold_any("a", "q") == "qbcdqbbccddqbcd");
    CHECK(str{s}.fold_any("abcd", 'q') == "q");
    CHECK(str{s}.fold_any("dbca", 'q') == "q");
    CHECK(str{s}.fold_any("ab", "q") == "qcdqccddqcd");
    CHECK(str{s}.fold_any("ad", "q") == "qbcqbbccqbcq");
}

TEST_CASE("str::fold_ws()") {
    auto s = "   a  b\t\t c\td\r\v\f\ne \r \v \n f  "s;
    CHECK(str{s}.fold_ws() == " a b c d e f ");
    CHECK(str{s}.fold_ws("q") == "qaqbqcqdqeqfq");
}

TEST_CASE("str::front()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(str{s1}.front() == 'h');
    CHECK(str{s2}.front() == std::nullopt);
}

TEST_CASE("str::has()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(str{s1}.has(0));
    CHECK(str{s1}.has(1));
    CHECK(str{s1}.has(2));
    CHECK(str{s1}.has(6));
    CHECK(str{s1}.has(11));
    CHECK(not str{s1}.has(12));

    CHECK(str{s2}.has(0));
    CHECK(not str{s2}.has(1));
}

TEST_CASE("str::last()") {
    str s = "abcd abcd abcd";

    CHECK(s.last('a') == 10);
    CHECK(s.last('b') == 11);
    CHECK(s.last('c') == 12);
    CHECK(s.last('d') == 13);
    CHECK(s.last('e') == std::nullopt);

    CHECK(s.last("a") == 10);
    CHECK(s.last("ab") == 10);
    CHECK(s.last("abc") == 10);
    CHECK(s.last("abcd abcd abcd") == 0);
    CHECK(s.last("abcd abcd abcdx") == std::nullopt);
    CHECK(s.last("") == 14);

    CHECK(s.last(text::IsAlnum) == 13);
    CHECK(s.last(text::IsSpace) == 9);
    CHECK(s.last([](char c) { return c == 'd'; }) == 13);

    CHECK(str("x").last('x') == 0);
    CHECK(str("x").last("x") == 0);
    CHECK(str("x").last([](char c) { return c == 'x'; }) == 0);

    CHECK(str().last('a') == std::nullopt);
    CHECK(str().last("a") == std::nullopt);
    CHECK(str().last(text::IsAlnum) == std::nullopt);
    CHECK(str().last("") == 0);
}

TEST_CASE("str::last_any()") {
    str s = "abcd abcd abcd";

    CHECK(s.last_any("abcd") == 13);
    CHECK(s.last_any("bc") == 12);
    CHECK(s.last_any("db") == 13);
    CHECK(s.last_any("ab") == 11);
    CHECK(s.last_any(" ") == 9);
    CHECK(s.last_any("") == std::nullopt);

    CHECK(str().last_any("abcd") == std::nullopt);
    CHECK(str().last_any("bc") == std::nullopt);
    CHECK(str().last_any("db") == std::nullopt);
    CHECK(str().last_any(" ") == std::nullopt);
    CHECK(str().last_any("") == std::nullopt);
}

TEST_CASE("str::max_size()") {
    CHECK(str().max_size() == std::string_view().max_size());
}

TEST_CASE("str::narrow()") {
    str s = "hello world";

    s.narrow(1, 1000);
    CHECK(s == "ello world");

    s.narrow(0, 1000);
    CHECK(s == "ello world");

    s.narrow(2, 6);
    CHECK(s == "lo wor");

    s.narrow(1, 1);
    CHECK(s == "o");

    s.narrow(10, 0);
    CHECK(s == "");

    s.narrow(10, 0);
    CHECK(s == "");
}

TEST_CASE("str::remove()") {
    std::string s = "hello world";

    CHECK(str{s}.remove('l') == "heo word");
    CHECK(str{s}.remove_all("l") == "heo word");
    CHECK(str{s}.remove_all("lo") == "he wrd");
    CHECK(str{s}.remove_all("ol") == "he wrd");
    CHECK(str{s}.remove('x') == "hello world");
    CHECK(str{s}.remove_all("xyz") == "hello world");

    CHECK(str{s}.remove<StringWrapper>('l') == "heo word");
    CHECK(str{s}.remove_all<StringWrapper>("l") == "heo word");
    CHECK(str{s}.remove_all<StringWrapper>("lo") == "he wrd");
    CHECK(str{s}.remove_all<StringWrapper>("ol") == "he wrd");
    CHECK(str{s}.remove<StringWrapper>('x') == "hello world");
    CHECK(str{s}.remove_all<StringWrapper>("xyz") == "hello world");
}

TEST_CASE("str::replace()") {
    str s1 = "hello world";
    str s2 = "aaa foo bbb aaa";

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

TEST_CASE("str::replace_many()") {
    str s1 = "hello world";
    str s2 = "aaa foo bbb aaa";

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

TEST_CASE("str::reverse()") {
    CHECK(str("abcd").reverse() == "dcba");
    CHECK(str("a").reverse() == "a");
    CHECK(str("").reverse() == "");
    CHECK(str("11223344").reverse() == "44332211");
    CHECK(str("av").reverse() == "va");

    CHECK(str32(U"abcd").reverse() == U"dcba");
    CHECK(str32(U"a").reverse() == U"a");
    CHECK(str32(U"").reverse() == U"");
    CHECK(str32(U"11223344").reverse() == U"44332211");
    CHECK(str32(U"av").reverse() == U"va");
}

TEST_CASE("str::size()") {
    std::string s1 = "hello world";
    std::string s2;

    CHECK(str{s1}.size() == 11);
    CHECK(str{s2}.size() == 0);
}

TEST_CASE("str::size_bytes()") {
    std::u32string s1 = U"hello world";
    std::u32string s2;

    CHECK(str32{s1}.size_bytes() == s1.size() * sizeof(char32_t));
    CHECK(str32{s2}.size_bytes() == 0);
}

TEST_CASE("str::slice()") {
    str s = "hello world";

    CHECK(s.slice(0, 2) == "he");
    CHECK(s.slice(3, 0) == "");
    CHECK(s.slice(4, 1) == "o");
    CHECK(s.slice(2, 2) == "ll");
    CHECK(s.slice(2, 1000) == "llo world");
    CHECK(s.slice(1000, 2) == "");
    CHECK(s.slice(1000, 1000) == "");

    CHECK(str().slice(1, 1) == "");

    CHECK(s.substr(0, 2) == "he");
    CHECK(s.substr(3, 0) == "");
    CHECK(s.substr(4, 1) == "o");
    CHECK(s.substr(2, 2) == "ll");
    CHECK(s.substr(2, 1000) == "llo world");
    CHECK(s.substr(1000, 2) == "");
    CHECK(s.substr(1000, 1000) == "");

    CHECK(str().substr(1, 1) == "");
}

TEST_CASE("str::split()") {
    std::string s = "aa\nbb\ncc\nbb\ncc";

    SECTION ("1") {
        auto lines = str{s}.lines();
        REQUIRE(rgs::distance(lines) == 5);
        CHECK_THAT(lines, RangeEquals(std::vector{"aa"sv, "bb"sv, "cc"sv, "bb"sv, "cc"sv}));
    }

    SECTION ("2") {
        auto bb = str{s}.split("bb");
        REQUIRE(rgs::distance(bb) == 3);
        CHECK_THAT(bb, RangeEquals(std::vector{"aa\n"sv, "\ncc\n"sv, "\ncc"sv}));
    }

    SECTION ("3") {
        auto cc = str{s}.split("cc");
        REQUIRE(rgs::distance(cc) == 3);
        CHECK_THAT(cc, RangeEquals(std::vector{"aa\nbb\n"sv, "\nbb\n"sv, ""sv}));
    }
}

TEST_CASE("str::starts_with") {
    std::string s = "hello world";

    CHECK(str{s}.starts_with('h'));
    CHECK(str{s}.starts_with("h"));
    CHECK(str{s}.starts_with("he"));
    CHECK(str{s}.starts_with("hello"));
    CHECK_FALSE(str{s}.starts_with("hx"));
    CHECK_FALSE(str{s}.starts_with("xhello"));
}

TEST_CASE("str::starts_with_any") {
    std::string s = "hello world";

    CHECK(str{s}.starts_with_any("h"));
    CHECK(str{s}.starts_with_any("eh"));
    CHECK(str{s}.starts_with_any("hello"));
    CHECK(str{s}.starts_with_any("hx"));
    CHECK(str{s}.starts_with_any("xhello"));
    CHECK_FALSE(str{s}.starts_with_any("x"));
}

TEST_CASE("str::swap()") {
    str s1 = "abcd";
    str s2 = "defg";

    s1.swap(s2);
    CHECK(s1 == "defg");
    CHECK(s2 == "abcd");

    s1.swap(s2);
    CHECK(s1 == "abcd");
    CHECK(s2 == "defg");
}

TEST_CASE("str::take_delimited()") {
    str s;
    str out;

    out = {};
    s = "1foo1abc";
    CHECK(s.take_delimited(out, "1"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "123foo123abc";
    CHECK(s.take_delimited(out, "123"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "123foo123abc";
    CHECK(not s.take_delimited(out, "1234"));
    CHECK(s == "123foo123abc");
    CHECK(out == "");

    out = {};
    s = "1foo";
    CHECK(not s.take_delimited(out, "1"));
    CHECK(s == "1foo");
    CHECK(out == "");

    out = {};
    s = "123foo";
    CHECK(not s.take_delimited(out, "123"));
    CHECK(s == "123foo");
    CHECK(out == "");

    out = {};
    s = "1";
    CHECK(not s.take_delimited(out, "1"));
    CHECK(s == "1");
    CHECK(out == "");

    out = {};
    s = "123";
    CHECK(not s.take_delimited(out, "123"));
    CHECK(s == "123");
    CHECK(out == "");

    out = {};
    s = "";
    CHECK(not s.take_delimited(out, "123"));
    CHECK(s == "");
    CHECK(out == "");

    CHECK_THROWS_WITH(str("abc").take_delimited(out, ""), ContainsSubstring("Delimiter must not be empty"));
    CHECK_THROWS_WITH(str("").take_delimited(out, ""), ContainsSubstring("Delimiter must not be empty"));
}

TEST_CASE("str::take_delimited_any()") {
    str s;
    str out;

    out = {};
    s = "1foo1abc";
    CHECK(s.take_delimited_any(out, "1"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "1foo1abc";
    CHECK(s.take_delimited_any(out, "123"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "2foo2abc";
    CHECK(s.take_delimited_any(out, "123"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "3foo3abc";
    CHECK(s.take_delimited_any(out, "123"));
    CHECK(s == "abc");
    CHECK(out == "foo");

    out = {};
    s = "3foo1abc";
    CHECK(not s.take_delimited_any(out, "123"));
    CHECK(s == "3foo1abc");
    CHECK(out == "");

    out = {};
    s = "1foo2abc";
    CHECK(not s.take_delimited_any(out, "123"));
    CHECK(s == "1foo2abc");
    CHECK(out == "");

    out = {};
    s = "3fooabc";
    CHECK(not s.take_delimited_any(out, "123"));
    CHECK(s == "3fooabc");
    CHECK(out == "");

    out = {};
    s = "3";
    CHECK(not s.take_delimited_any(out, "123"));
    CHECK(s == "3");
    CHECK(out == "");

    out = {};
    s = "";
    CHECK(not s.take_delimited_any(out, "123"));
    CHECK(s == "");
    CHECK(out == "");

    CHECK_THROWS_WITH(str("abc").take_delimited_any(out, ""), ContainsSubstring("At least one delimiter is required"));
    CHECK_THROWS_WITH(str("").take_delimited_any(out, ""), ContainsSubstring("At least one delimiter is required"));
}

TEST_CASE("str::take_until(_any)(_and_drop)") {
    std::string s = "hello world";
    const Views v{"lx", "llox"};

    SECTION("take_until") {
        CHECK(str{s}.take_until(' ') == "hello"sv);
        CHECK(str{s}.take_until('o') == "hell"sv);
        CHECK(str{s}.take_until('x') == "hello world"sv);
        CHECK(str{s}.take_until('h') == ""sv);

        CHECK(str{s}.take_until("") == ""sv);
        CHECK(str{s}.take_until(" ") == "hello"sv);
        CHECK(str{s}.take_until("lo") == "hel"sv);
        CHECK(str{s}.take_until("ld") == "hello wor"sv);
        CHECK(str{s}.take_until("lx") == "hello world"sv);

        CHECK(str{s}.take_until([](char c) { return c == ' '; }) == "hello"sv);
        CHECK(str{s}.take_until([](char c) { return c == 'o'; }) == "hell"sv);
        CHECK(str{s}.take_until([](char c) { return c == 'x'; }) == "hello world"sv);
        CHECK(str{s}.take_until([](char c) { return c == 'h'; }) == ""sv);
    }

    SECTION("any") {
        CHECK(str{s}.take_until_any("") == "hello world"sv);
        CHECK(str{s}.take_until_any(" ") == "hello"sv);
        CHECK(str{s}.take_until_any("eo") == "h"sv);
        CHECK(str{s}.take_until_any("x") == "hello world"sv);
        CHECK(str{s}.take_until_any("rw") == "hello "sv);

        CHECK(str{s}.take_until_any(Views{}) == "hello world"sv);
        CHECK(str{s}.take_until_any(Views{"e", "o"}) == "h"sv);
        CHECK(str{s}.take_until_any(Views{"lo", "el"}) == "h"sv);
        CHECK(str{s}.take_until_any(Views{"lo", "llo"}) == "he"sv);
        CHECK(str{s}.take_until_any(v) == "hello world"sv);
        CHECK(str{s}.take_until_any(v | vws::transform([](auto x) { return x; })) == "hello world"sv);
        CHECK(str{s}.take_until_any(v | vws::filter([](auto x) { return true; })) == "hello world"sv);
    }

    SECTION("and_drop") {
        str s1{s};
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
        str s1{s};
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

TEST_CASE("str::take_until_or_empty") {
    std::string s = "hello world";
    const Views v{"lx", "llox"};

    CHECK(str{s}.take_until_or_empty(' ') == "hello"sv);
    CHECK(str{s}.take_until_or_empty('o') == "hell"sv);
    CHECK(str{s}.take_until_or_empty('x') == ""sv);
    CHECK(str{s}.take_until_or_empty('h') == ""sv);

    CHECK(str{s}.take_until_any_or_empty(" ") == "hello"sv);
    CHECK(str{s}.take_until_any_or_empty("eo") == "h"sv);
    CHECK(str{s}.take_until_any_or_empty("x") == ""sv);
    CHECK(str{s}.take_until_any_or_empty("rw") == "hello "sv);

    CHECK(str{s}.take_until_or_empty(" ") == "hello"sv);
    CHECK(str{s}.take_until_or_empty("lo") == "hel"sv);
    CHECK(str{s}.take_until_or_empty("ld") == "hello wor"sv);
    CHECK(str{s}.take_until_or_empty("lx") == ""sv);

    CHECK(str{s}.take_until_any_or_empty(Views{}) == ""sv);
    CHECK(str{s}.take_until_any_or_empty(Views{"e", "o"}) == "h"sv);
    CHECK(str{s}.take_until_any_or_empty(Views{"lo", "el"}) == "h"sv);
    CHECK(str{s}.take_until_any_or_empty(Views{"lo", "llo"}) == "he"sv);
    CHECK(str{s}.take_until_any_or_empty(v) == ""sv);
    CHECK(str{s}.take_until_any_or_empty(v | vws::transform([](auto x) { return x; })) == ""sv);
    CHECK(str{s}.take_until_any_or_empty(v | vws::filter([](auto x) { return true; })) == ""sv);

    CHECK(str{s}.take_until_or_empty([](char c) { return c == ' '; }) == "hello"sv);
    CHECK(str{s}.take_until_or_empty([](char c) { return c == 'o'; }) == "hell"sv);
    CHECK(str{s}.take_until_or_empty([](char c) { return c == 'x'; }) == ""sv);
    CHECK(str{s}.take_until_or_empty([](char c) { return c == 'h'; }) == ""sv);
}

TEST_CASE("str::take_until_ws") {
    str s = "a  b\t\t c\td\r\v\f\ne \r \v \n f"sv;
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

TEST_CASE("str::take_while") {
    std::string s = "hello world";

    CHECK(str{s}.take_while(' ') == ""sv);
    CHECK(str{s}.take_while('h') == "h"sv);
    CHECK(str{s}.take_while('o') == ""sv);
    CHECK(str{s}.take_while('x') == ""sv);

    CHECK(str{s}.take_while_any(" ") == ""sv);
    CHECK(str{s}.take_while_any("eho") == "he"sv);
    CHECK(str{s}.take_while_any("x") == ""sv);
    CHECK(str{s}.take_while_any("w loeh") == "hello wo"sv);

    CHECK(str{s}.take_while([](char c) { return c == ' '; }) == ""sv);
    CHECK(str{s}.take_while([](char c) { return "eho"sv.contains(c); }) == "he"sv);
    CHECK(str{s}.take_while([](char c) { return c == 'x'; }) == ""sv);
    CHECK(str{s}.take_while([](char c) { return "w loeh"sv.contains(c); }) == "hello wo"sv);
}

TEST_CASE("str::take_back_until") {
    std::string s = "hello world";

    CHECK(str{s}.take_back_until(' ') == "world"sv);
    CHECK(str{s}.drop_back_until(' ') == "hello "sv);

    CHECK(str{s}.take_back_until('o') == "rld"sv);
    CHECK(str{s}.drop_back_until('o') == "hello wo"sv);

    CHECK(str{s}.take_back_until('x') == "hello world"sv);
    CHECK(str{s}.drop_back_until('x') == ""sv);

    CHECK(str{s}.take_back_until('h') == "ello world"sv);
    CHECK(str{s}.drop_back_until('h') == "h"sv);

    CHECK(str{s}.take_back_until('d') == ""sv);
    CHECK(str{s}.drop_back_until('d') == "hello world"sv);

    CHECK(str{s}.take_back_until_any(" ") == "world"sv);
    CHECK(str{s}.drop_back_until_any(" ") == "hello "sv);

    CHECK(str{s}.take_back_until_any("eo") == "rld"sv);
    CHECK(str{s}.drop_back_until_any("eo") == "hello wo"sv);

    CHECK(str{s}.take_back_until_any("x") == "hello world"sv);
    CHECK(str{s}.drop_back_until_any("x") == ""sv);

    CHECK(str{s}.take_back_until_any("rw") == "ld"sv);
    CHECK(str{s}.drop_back_until_any("rw") == "hello wor"sv);

    CHECK(str{s}.take_back_until_any("ld") == ""sv);
    CHECK(str{s}.drop_back_until_any("ld") == "hello world"sv);

    CHECK(str{s}.take_back_until(" ") == "world"sv);
    CHECK(str{s}.drop_back_until(" ") == "hello "sv);

    CHECK(str{s}.take_back_until("lo") == " world"sv);
    CHECK(str{s}.drop_back_until("lo") == "hello"sv);

    CHECK(str{s}.take_back_until("ld") == ""sv);
    CHECK(str{s}.drop_back_until("ld") == "hello world"sv);

    CHECK(str{s}.take_back_until("lx") == "hello world"sv);
    CHECK(str{s}.drop_back_until("lx") == ""sv);
}

TEST_CASE("str::take_back_until_or_empty") {
    std::string s = "hello world";

    CHECK(str{s}.take_back_until_or_empty(' ') == "world"sv);
    CHECK(str{s}.drop_back_until_or_empty(' ') == "hello "sv);

    CHECK(str{s}.take_back_until_or_empty('o') == "rld"sv);
    CHECK(str{s}.drop_back_until_or_empty('o') == "hello wo"sv);

    CHECK(str{s}.take_back_until_or_empty('x') == ""sv);
    CHECK(str{s}.drop_back_until_or_empty('x') == "hello world"sv);

    CHECK(str{s}.take_back_until_or_empty('h') == "ello world"sv);
    CHECK(str{s}.drop_back_until_or_empty('h') == "h"sv);

    CHECK(str{s}.take_back_until_or_empty('d') == ""sv);
    CHECK(str{s}.drop_back_until_or_empty('d') == "hello world"sv);

    CHECK(str{s}.take_back_until_any_or_empty(" ") == "world"sv);
    CHECK(str{s}.drop_back_until_any_or_empty(" ") == "hello "sv);

    CHECK(str{s}.take_back_until_any_or_empty("eo") == "rld"sv);
    CHECK(str{s}.drop_back_until_any_or_empty("eo") == "hello wo"sv);

    CHECK(str{s}.take_back_until_any_or_empty("x") == ""sv);
    CHECK(str{s}.drop_back_until_any_or_empty("x") == "hello world"sv);

    CHECK(str{s}.take_back_until_any_or_empty("rw") == "ld"sv);
    CHECK(str{s}.drop_back_until_any_or_empty("rw") == "hello wor"sv);

    CHECK(str{s}.take_back_until_any_or_empty("ld") == ""sv);
    CHECK(str{s}.drop_back_until_any_or_empty("ld") == "hello world"sv);

    CHECK(str{s}.take_back_until_or_empty(" ") == "world"sv);
    CHECK(str{s}.drop_back_until_or_empty(" ") == "hello "sv);

    CHECK(str{s}.take_back_until_or_empty("lo") == " world"sv);
    CHECK(str{s}.drop_back_until_or_empty("lo") == "hello"sv);

    CHECK(str{s}.take_back_until_or_empty("ld") == ""sv);
    CHECK(str{s}.drop_back_until_or_empty("ld") == "hello world"sv);

    CHECK(str{s}.take_back_until_or_empty("lx") == ""sv);
    CHECK(str{s}.drop_back_until_or_empty("lx") == "hello world"sv);
}

TEST_CASE("str::take_back_until: whitespace") {
    constexpr std::string_view ws = " \t\r\n\f\v";
    std::string s1{"foo  bar\tbaz\r\nquux\fbar"};
    str s{s1};

    CHECK(s.take_back_until_any(ws) == "bar"sv);
    CHECK(RawString{std::string{s}} == "foo  bar\tbaz\r\nquux\f"_raw);
    CHECK(s.consume_back('\f'));
    CHECK(s.take_back_until_any(ws) == "quux"sv);
    CHECK(s.consume_back('\n'));
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(s.consume_back('\r'));
    CHECK(s.take_back_until_any(ws) == "baz"sv);
    CHECK(s.consume_back('\t'));
    CHECK(s.take_back_until_any(ws) == "bar"sv);
    CHECK(RawString{std::string{s}} == "foo  "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s}} == "foo "_raw);
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(RawString{std::string{s}} == "foo "_raw);
    CHECK(s.consume_back(' '));
    CHECK(s.take_back_until_any(ws) == "foo"sv);
    CHECK(s.take_back_until_any(ws) == ""sv);
    CHECK(s.empty());

    s = s1;
    CHECK(s.take_back_until_any_or_empty(ws) == "bar"sv);
    CHECK(RawString{std::string{s}} == "foo  bar\tbaz\r\nquux\f"_raw);
    CHECK(s.consume_back('\f'));
    CHECK(s.take_back_until_any_or_empty(ws) == "quux"sv);
    CHECK(s.consume_back('\n'));
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s.consume_back('\r'));
    CHECK(s.take_back_until_any_or_empty(ws) == "baz"sv);
    CHECK(s.consume_back('\t'));
    CHECK(s.take_back_until_any_or_empty(ws) == "bar"sv);
    CHECK(RawString{std::string{s}} == "foo  "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s}} == "foo "_raw);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(RawString{std::string{s}} == "foo "_raw);
    CHECK(s.consume_back(' '));
    CHECK(RawString{std::string{s}} == "foo"_raw);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s.take_back_until_any_or_empty(ws) == ""sv);
    CHECK(s == "foo");
}

TEST_CASE("str::string()") {
    CHECK(str("1234").string() == "1234");
    CHECK(
        str32(U"12341234123412341234123412341234123412341234").string()
        ==
        U"12341234123412341234123412341234123412341234"
    );
}

TEST_CASE("str::operator[]") {
    str s = "hello world";
    CHECK(s[0] == 'h');
    CHECK(s[3] == 'l');

    CHECK_THROWS(s[-1]);
    CHECK_THROWS(s[100]);
    CHECK_THROWS(s[s.size()]);

    CHECK(str("x")[0] == 'x');
    CHECK_THROWS(str("x")[1]);
    CHECK_THROWS(str("x")[2]);

    CHECK_THROWS(str()[2]);
    CHECK_THROWS(str()[0]);
    CHECK_THROWS(str()[1]);
    CHECK_THROWS(str()[2]);
}

TEST_CASE("str::operator<=>") {
    str s1{"hello world"};
    str s2{"hello world"};
    str s3{"hello"};

    CHECK(s1 == s2);
    CHECK(s1 != s3);
    CHECK(s1 == s2);
    CHECK(s1 != s3);
    CHECK(s1 == s2);
    CHECK(s1 != s3);

    CHECK(s1 == "hello world");
    CHECK(s1 != "hello");
    CHECK(s1 == "hello world"sv);
    CHECK(s1 != "hello"sv);
    CHECK(s1 == "hello world"s);
    CHECK(s1 != "hello"s);
}

TEST_CASE("str: implicit conversion to string view") {
    [[maybe_unused]] std::string_view sv;
    sv = str();
}

TEST_CASE("str: formatter") {
    CHECK(std::format("123 {}", str("abc")) == "123 abc");
    CHECK(std::format("123 {}", str("abc").take(2)) == "123 ab");
}

TEST_CASE("str: hashable") {
    std::unordered_map<str, int> m;
    m[str("123")] = 123;
    m[str("456")] = 456;
    CHECK(m.at(str("123")) == 123);
    CHECK(m.at(str("456")) == 456);
}
