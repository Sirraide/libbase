#include "TestCommon.hh"
#include <base/Trie.hh>

using namespace base;

trie t{{"foo", "bar"}};

TEST_CASE("Trie: Empty replacement.") {
    CHECK(t.replace("") == "");
}

TEST_CASE("Trie: No match") {
    CHECK(t.replace("does not match at all") == "does not match at all");
}

TEST_CASE("Trie: Simple match") {
    CHECK(t.replace("foo") == "bar");
}

TEST_CASE("Trie: Repeated match") {
    CHECK(t.replace("foofoofoo") == "barbarbar");
}

TEST_CASE("Trie: Interleaved") {
    CHECK(t.replace("fofoo") == "fobar");
    CHECK(t.replace("foofo") == "barfo");
    CHECK(t.replace("ffoo") == "fbar");
    CHECK(t.replace("ffooo") == "fbaro");
}

TEST_CASE("Trie: Recursion") {
    trie t{
        {"a", "aa"},
    };

    CHECK(t.replace("aa") == "aaaa");
}

TEST_CASE("Trie: Examples listed in comments") {
    trie a {
        {"fo", "X"},
        {"foo", "Y"},
    };

    CHECK(a.replace("foo") == "Y");
    CHECK(a.replace("fo") == "X");

    trie b {
        {"tree", "X"},
        {"reenact", "Y"},
    };

    CHECK(b.replace("treenact") == "Xnact");
}

TEST_CASE("Trie: HTML Escaping") {
    static constexpr std::string_view input = R"html(<table><tbody><tr><td>Birth</td><td>August 11, 1980</td></tr><tr><td>Death</td><td>November 22nd, 2018</td></tr><tr><td>Ability</td><td><a href="/w/index.php?title=Young_and_Menace&amp;action=edit&amp;redlink=1" class="new" title="Young and Menace (page does not exist)">Young and Menace</a></td></tr><tr><td>Parents</td><td>Valentin Pivovarov, Margarita Kepelkeker</td></tr><tr><td>Spouse</td><td>Aisha Mankita (2007–2018)</td></tr><tr><td>Children</td><td>Bean Pivovarov (b. 2019)</td></tr><tr><td>Career</td><td>Shift manager at Starbucks</td></tr><tr><td>Known for</td><td>Coffee</td></tr></tbody></table>)html";
    static constexpr std::string_view output = R"html(&lt;table&gt;&lt;tbody&gt;&lt;xxy&gt;&lt;xx&gt;Birth&lt;/xx&gt;&lt;xx&gt;August 11, 1980&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Death&lt;/xx&gt;&lt;xx&gt;November 22nd, 2018&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Ability&lt;/xx&gt;&lt;xx&gt;&lt;a href=&quot;/w/index.php?title=Young_and_Menace&amp;amp;action=edit&amp;amp;redlink=1&quot; class=&quot;new&quot; title=&quot;Young and Menace (page does not exist)&quot;&gt;Young and Menace&lt;/a&gt;&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Parents&lt;/xx&gt;&lt;xx&gt;Valentin Pivovarov, Margarita Kepelkeker&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Spouse&lt;/xx&gt;&lt;xx&gt;Aisha Mankita (2007&ndash;2018)&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Children&lt;/xx&gt;&lt;xx&gt;Bean Pivovarov (b. 2019)&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Career&lt;/xx&gt;&lt;xx&gt;Shift manager at Starbucks&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Known for&lt;/xx&gt;&lt;xx&gt;Coffee&lt;/xx&gt;&lt;/xxy&gt;&lt;/tbody&gt;&lt;/table&gt;)html";

    trie trie{
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"&", "&amp;"},
        {"tr", "xxy"},
        {"td", "xx"},
        {"–", "&ndash;"}
    };

    CHECK(trie.replace(input) == output);
}

TEST_CASE("Trie: HTML Escaping (UTF-32)") {
    static constexpr std::u32string_view input = UR"html(<table><tbody><tr><td>Birth</td><td>August 11, 1980</td></tr><tr><td>Death</td><td>November 22nd, 2018</td></tr><tr><td>Ability</td><td><a href="/w/index.php?title=Young_and_Menace&amp;action=edit&amp;redlink=1" class="new" title="Young and Menace (page does not exist)">Young and Menace</a></td></tr><tr><td>Parents</td><td>Valentin Pivovarov, Margarita Kepelkeker</td></tr><tr><td>Spouse</td><td>Aisha Mankita (2007–2018)</td></tr><tr><td>Children</td><td>Bean Pivovarov (b. 2019)</td></tr><tr><td>Career</td><td>Shift manager at Starbucks</td></tr><tr><td>Known for</td><td>Coffee</td></tr></tbody></table>)html";
    static constexpr std::u32string_view output = UR"html(&lt;table&gt;&lt;tbody&gt;&lt;xxy&gt;&lt;xx&gt;Birth&lt;/xx&gt;&lt;xx&gt;August 11, 1980&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Death&lt;/xx&gt;&lt;xx&gt;November 22nd, 2018&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Ability&lt;/xx&gt;&lt;xx&gt;&lt;a href=&quot;/w/index.php?title=Young_and_Menace&amp;amp;action=edit&amp;amp;redlink=1&quot; class=&quot;new&quot; title=&quot;Young and Menace (page does not exist)&quot;&gt;Young and Menace&lt;/a&gt;&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Parents&lt;/xx&gt;&lt;xx&gt;Valentin Pivovarov, Margarita Kepelkeker&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Spouse&lt;/xx&gt;&lt;xx&gt;Aisha Mankita (2007&ndash;2018)&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Children&lt;/xx&gt;&lt;xx&gt;Bean Pivovarov (b. 2019)&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Career&lt;/xx&gt;&lt;xx&gt;Shift manager at Starbucks&lt;/xx&gt;&lt;/xxy&gt;&lt;xxy&gt;&lt;xx&gt;Known for&lt;/xx&gt;&lt;xx&gt;Coffee&lt;/xx&gt;&lt;/xxy&gt;&lt;/tbody&gt;&lt;/table&gt;)html";

    u32trie trie{
        {U"<", U"&lt;"},
        {U">", U"&gt;"},
        {U"\"", U"&quot;"},
        {U"&", U"&amp;"},
        {U"tr", U"xxy"},
        {U"td", U"xx"},
        {U"–", U"&ndash;"}
    };

    CHECK(trie.replace(input) == output);
}

TEST_CASE("Trie: Adding a pattern eventually recomputes failure links") {
    trie trie{{"foot", "hand"}};
    CHECK(trie.replace("foof foot foot foo") == "foof hand hand foo");

    trie.add("oof", "bar");
    CHECK(trie.replace("foof foot foot foo") == "fbar hand hand foo");
}

TEST_CASE("Trie: Backtracking after a match") {
    trie trie{
        {"foo", "bar"},
        {"football", "baz"},
        {"tba", "quux"},
    };

    CHECK(trie.replace("foox foox") == "barx barx");
    CHECK(trie.replace("foofoo") == "barbar");
    CHECK(trie.replace("footb") == "bartb");
    CHECK(trie.replace("footba") == "barquux");
    CHECK(trie.replace("footbal") == "barquuxl");
    CHECK(trie.replace("football") == "baz");
    CHECK(trie.replace("footbaq") == "barquuxq");
    CHECK(trie.replace("footbafoo") == "barquuxbar");
    CHECK(trie.replace("footballfoo") == "bazbar");
}

TEST_CASE("Trie: failure cases") {
    trie trie{
        {"football", "baz"},
        {"otbas", "bar"},
        {"tea", "quux"},
    };

    CHECK(trie.replace("footbas") == "fobar");
    CHECK(trie.replace("footqtea") == "footqquux");
    CHECK(trie.replace("footea") == "fooquux");
    CHECK(trie.replace("xxxyyy") == "xxxyyy");
}

TEST_CASE("Trie: multiple failures in a row") {
    trie trie {
        {"abcdef", "1"},
        {"bcdef", "3"},
        {"cdef", "4"},
        {"def", "5"},
        {"ef", "6"},
        {"g", "7"},
    };

    CHECK(trie.replace("abcdeg") == "abcde7");
}

TEST_CASE("Trie::get()") {
    trie trie {
        {"abcdef", "1"},
        {"bcdef", "3"},
        {"cdef", "4"},
        {"def", "5"},
        {"ef", "6"},
        {"g", "7"},
    };

    CHECK(trie.get("abcdef") == "1");
    CHECK(trie.get("bcdef") == "3");
    CHECK(trie.get("cdef") == "4");
    CHECK(trie.get("def") == "5");
    CHECK(trie.get("ef") == "6");
    CHECK(trie.get("g") == "7");

    CHECK(trie.get("abcdefg") == std::nullopt);
    CHECK(trie.get("abcde") == std::nullopt);
    CHECK(trie.get("abc") == std::nullopt);
    CHECK(trie.get("a") == std::nullopt);
    CHECK(trie.get("defg") == std::nullopt);
    CHECK(trie.get("x") == std::nullopt);
    CHECK(trie.get("qqqq") == std::nullopt);
    CHECK(trie.get("ef ") == std::nullopt);
    CHECK(trie.get("") == std::nullopt);
}

TEST_CASE("Trie::is_prefix_of()") {
    trie trie {
        {"abcdef", "1"},
        {"bcdef", "3"},
        {"cdef", "4"},
        {"def", "5"},
        {"ef", "6"},
        {"g", "7"},
    };

    CHECK(trie.is_prefix_of("abcdef"));
    CHECK(trie.is_prefix_of("abcdefqq"));
    CHECK(trie.is_prefix_of("bcdef"));
    CHECK(trie.is_prefix_of("bcdefqq"));
    CHECK(trie.is_prefix_of("cdef"));
    CHECK(trie.is_prefix_of("cdefq"));
    CHECK(trie.is_prefix_of("def"));
    CHECK(trie.is_prefix_of("defafaefaef"));
    CHECK(trie.is_prefix_of("ef"));
    CHECK(trie.is_prefix_of("efafaef"));
    CHECK(trie.is_prefix_of("g"));
    CHECK(trie.is_prefix_of("ga"));

    CHECK(not trie.is_prefix_of("abcde"));
    CHECK(not trie.is_prefix_of("abcd"));
    CHECK(not trie.is_prefix_of("cde"));
    CHECK(not trie.is_prefix_of("cdeg"));
    CHECK(not trie.is_prefix_of("cd"));
    CHECK(not trie.is_prefix_of("cdq"));
    CHECK(not trie.is_prefix_of("qq"));
    CHECK(not trie.is_prefix_of("xafcaefgagef"));
}
