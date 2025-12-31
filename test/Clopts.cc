#include <base/Clopts.hh>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <fstream>
#include <tuple>

#include "TestCommon.hh"

using namespace base;
using namespace base::cmd;
using namespace Catch::literals;
using namespace std::literals;

static bool error_handler(std::string&& s) {
    throw std::runtime_error(s);
}

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc23-extensions"
static constexpr char this_file_data[]{
#embed __FILE__
};
#pragma GCC diagnostic pop
#endif // __clang__

template <typename path_type = std::filesystem::path>
static auto this_file() -> std::pair<path_type, std::string> {
    std::string_view _file_ = __FILE__;

#ifdef __clang__
    std::string contents{this_file_data, sizeof this_file_data};
#else
    std::ifstream f{__FILE__};
    std::string contents{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
#endif // __clang__

    path_type path{_file_.begin(), _file_.end()};
    return std::pair{path, contents};
}

static void print_number_and_exit(void* arg, std::string_view) {
    int* i = static_cast<int*>(arg);
    std::println("{}", *i);
    std::exit(0);
}

static_assert(detail::is_positional_v<positional<"foo", "bar">>);
static_assert(detail::is_positional_v<multiple<positional<"foo", "bar">>>);

using basic_options = clopts<
    option<"--string", "A string", std::string>,
    option<"--number", "A number", int64_t>,
    option<"--float", "A float", double>>;

TEST_CASE("Options are nullptr by default") {
    auto opts = basic_options::parse(0, nullptr, error_handler);
    CHECK(not opts.get<"--string">());
    CHECK(not opts.get<"--number">());
    CHECK(not opts.get<"--float">());
}

TEST_CASE("Options can be parsed") {
    std::array args = {
        "test",
        "--string",
        "Hello, world!",
        "--number",
        "42",
        "--float",
        "3.141592653589",
    };

    auto opts1 = basic_options::parse(args.size(), args.data(), error_handler);
    REQUIRE(opts1.get<"--string">());
    REQUIRE(opts1.get<"--number">());
    REQUIRE(opts1.get<"--float">());
    CHECK(*opts1.get<"--string">() == "Hello, world!");
    CHECK(*opts1.get<"--number">() == 42);
    CHECK(*opts1.get<"--float">() == 3.141592653589_a);

    SECTION("multiple times") {
        auto opts2 = basic_options::parse(args.size(), args.data(), error_handler);

        REQUIRE(opts2.get<"--string">());
        REQUIRE(opts2.get<"--number">());
        REQUIRE(opts2.get<"--float">());

        CHECK(*opts2.get<"--string">() == "Hello, world!");
        CHECK(*opts2.get<"--number">() == 42);
        CHECK(*opts2.get<"--float">() == 3.141592653589_a);
    }
}

TEST_CASE("Options can be parsed out of order") {
    std::array args = {
        "test",
        "--float",
        "3.141592653589",
        "--number",
        "42",
        "--string",
        "Hello, world!",
    };

    auto opts = basic_options::parse(args.size(), args.data(), error_handler);
    REQUIRE(opts.get<"--string">());
    REQUIRE(opts.get<"--number">());
    REQUIRE(opts.get<"--float">());
    CHECK(*opts.get<"--string">() == "Hello, world!");
    CHECK(*opts.get<"--number">() == 42);
    CHECK(*opts.get<"--float">() == 3.141592653589_a);
}

TEST_CASE("Required options must be present") {
    using options = clopts<option<"--required", "A required option", std::string, true>>;
    CHECK_THROWS(options::parse(0, nullptr, error_handler));
}

TEST_CASE("Flags are never required") {
    using options = clopts<flag<"--flag", "A flag">>;
    CHECK_NOTHROW(options::parse(0, nullptr, error_handler));
}

TEST_CASE("Setting a custom error handler works") {
    using options = clopts<option<"--required", "A required option", std::string, true>>;
    bool called = false;
    options::parse(0, nullptr, [&](std::string&&) { return called = true; });
    CHECK(called);
}

TEST_CASE("values<> option type is handled properly") {
    using int_options = clopts<option<"--values", "A values option", values<0, 1, 2, 3>>>;
    using string_options = clopts<option<"--values", "A values option", values<"foo", "bar", "baz">>>;

    SECTION("Correct values are accepted") {
        std::array int_args = {
            "test",
            "--values",
            "1",
        };
        std::array string_args = {
            "test",
            "--values",
            "foo",
        };

        auto int_opts = int_options::parse(int_args.size(), int_args.data(), error_handler);
        auto string_opts = string_options::parse(string_args.size(), string_args.data(), error_handler);

        REQUIRE(int_opts.get<"--values">());
        REQUIRE(string_opts.get<"--values">());
        CHECK(*int_opts.get<"--values">() == 1);
        CHECK(*string_opts.get<"--values">() == "foo");
    }

    SECTION("Invalid values are rejected") {
        std::array int_args = {
            "test",
            "--values",
            "4",
        };
        std::array string_args = {
            "test",
            "--values",
            "qux",
        };

        CHECK_THROWS(int_options::parse(int_args.size(), int_args.data(), error_handler));
        CHECK_THROWS(string_options::parse(string_args.size(), string_args.data(), error_handler));
    }
}

TEST_CASE("Positional options are handled correctly") {
    using options = clopts<
        positional<"first", "The first positional argument", std::string, false>,
        positional<"second", "The second positional argument", int64_t, false>,
        positional<"third", "The third positional argument", double, false>>;

    std::array args = {
        "test",
        "Hello, world!",
        "42",
        "3.141592653589",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    REQUIRE(opts.get<"first">());
    REQUIRE(opts.get<"second">());
    REQUIRE(opts.get<"third">());

    CHECK(*opts.get<"first">() == "Hello, world!");
    CHECK(*opts.get<"second">() == 42);
    CHECK(*opts.get<"third">() == 3.141592653589_a);
}

TEST_CASE("Positional and non-positional options mix properly") {
    using options = clopts<
        positional<"first", "The first positional argument", std::string, false>,
        positional<"second", "The second positional argument", int64_t, false>,
        positional<"third", "The third positional argument", double, false>,
        option<"--string", "A string", std::string>,
        option<"--number", "A number", int64_t>,
        option<"--float", "A float", double>>;

    std::array args = {
        "test",
        "--string",
        "Hello, world!",
        "foobarbaz",
        "24",
        "--number",
        "42",
        "6.283185307179",
        "--float",
        "3.141592653589",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    REQUIRE(opts.get<"first">());
    REQUIRE(opts.get<"second">());
    REQUIRE(opts.get<"third">());
    REQUIRE(opts.get<"--string">());
    REQUIRE(opts.get<"--number">());
    REQUIRE(opts.get<"--float">());

    CHECK(*opts.get<"first">() == "foobarbaz");
    CHECK(*opts.get<"second">() == 24);
    CHECK(*opts.get<"third">() == 6.283185307179_a);
    CHECK(*opts.get<"--string">() == "Hello, world!");
    CHECK(*opts.get<"--number">() == 42);
    CHECK(*opts.get<"--float">() == 3.141592653589_a);
}

TEST_CASE("Positional options are required by default") {
    using options = clopts<positional<"first", "The first positional argument">>;
    CHECK_THROWS(options::parse(0, nullptr, error_handler));
}

TEST_CASE("Positional values<> work") {
    using string_options = clopts<positional<"format", "Output format", values<"foo", "bar">>>;
    using int_options = clopts<positional<"format", "Output format", values<0, 1>>>;

    SECTION("Correct values are accepted") {
        std::array args1 = {"test", "foo"};
        std::array args2 = {"test", "bar"};
        std::array args3 = {"test", "0"};
        std::array args4 = {"test", "1"};

        auto opts1 = string_options::parse(args1.size(), args1.data(), error_handler);
        auto opts2 = string_options::parse(args2.size(), args2.data(), error_handler);
        auto opts3 = int_options::parse(args3.size(), args3.data(), error_handler);
        auto opts4 = int_options::parse(args4.size(), args4.data(), error_handler);

        REQUIRE(opts1.get<"format">());
        REQUIRE(opts2.get<"format">());
        REQUIRE(opts3.get<"format">());
        REQUIRE(opts4.get<"format">());

        CHECK(*opts1.get<"format">() == "foo");
        CHECK(*opts2.get<"format">() == "bar");
        CHECK(*opts3.get<"format">() == 0);
        CHECK(*opts4.get<"format">() == 1);
    }

    SECTION("Invalid values raise an error") {
        std::array args1 = {"test", "baz"};
        std::array args2 = {"test", "2"};

        CHECK_THROWS(string_options::parse(args1.size(), args1.data(), error_handler));
        CHECK_THROWS(int_options::parse(args2.size(), args2.data(), error_handler));
    }
}

TEST_CASE("Multiple positional values<> work") {
    using string_options = clopts<multiple<positional<"format", "Output format", values<"foo", "bar">>>>;
    using int_options = clopts<multiple<positional<"format", "Output format", values<0, 1>>>>;

    SECTION("Correct values are accepted") {
        std::array args1 = {"test", "foo", "bar", "foo"};
        std::array args2 = {"test", "0", "1", "1"};

        auto opts1 = string_options::parse(args1.size(), args1.data(), error_handler);
        auto opts2 = int_options::parse(args2.size(), args2.data(), error_handler);

        REQUIRE(opts1.get<"format">().size() == 3);
        REQUIRE(opts2.get<"format">().size() == 3);

        CHECK(opts1.get<"format">()[0] == "foo");
        CHECK(opts1.get<"format">()[1] == "bar");
        CHECK(opts1.get<"format">()[2] == "foo");
        CHECK(opts2.get<"format">()[0] == 0);
        CHECK(opts2.get<"format">()[1] == 1);
        CHECK(opts2.get<"format">()[2] == 1);
    }

    SECTION("Invalid values raise an error") {
        std::array args1 = {"test", "foo", "baz", "foo"};
        std::array args2 = {"test", "0", "2", "1"};

        CHECK_THROWS(string_options::parse(args1.size(), args1.data(), error_handler));
        CHECK_THROWS(int_options::parse(args2.size(), args2.data(), error_handler));
    }
}

TEST_CASE("Short option options are parsed properly") {
    using options = clopts<
        short_option<"s", "A string", std::string>,
        short_option<"n", "A number", int64_t>,
        short_option<"-f", "A float", double>>;

    std::array args = {
        "test",
        "sHello, world!",
        "n=42",
        "-f3.141592653589",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    REQUIRE(opts.get<"s">());
    REQUIRE(opts.get<"n">());
    REQUIRE(opts.get<"-f">());
    CHECK(*opts.get<"s">() == "Hello, world!");
    CHECK(*opts.get<"n">() == 42);
    CHECK(*opts.get<"-f">() == 3.141592653589_a);
}

TEST_CASE("Empty option value is handled correctly") {
    std::array args = {"test", "--empty="};

    SECTION("for strings") {
        using options = clopts<option<"--empty", "An empty string", std::string>>;
        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.get<"--empty">());
        CHECK(opts.get<"--empty">()->empty());
    }

    SECTION("for integers") {
        using options = clopts<option<"--empty", "An empty integer", int64_t>>;
        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("for floats") {
        using options = clopts<option<"--empty", "An empty float", double>>;
        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("for values<> that contains the empty string") {
        using options = clopts<option<"--empty", "An empty value", values<"">>>;
        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.get<"--empty">());
        CHECK(opts.get<"--empty">()->empty());
    }
}

TEST_CASE("Integer overflow is an error") {
    using options = clopts<option<"--overflow", "A number", int64_t>>;

    std::array args = {
        "test",
        "--overflow",
        "100000000000000000000000000000000000000000000000",
    };

    CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
}

TEST_CASE("Multiple meta-option") {
    using options = clopts<
        multiple<option<"--int", "Integers", int64_t, true>>,
        multiple<option<"--string", "Strings", std::string, true>>>;

    std::array args = {
        "test",
        "--int",
        "1",
        "--string",
        "foo",
        "--int",
        "2",
        "--string",
        "bar",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    CHECK(opts.get<"--int">().size() == 2);
    CHECK(opts.get<"--string">().size() == 2);

    CHECK(opts.get<"--int">()[0] == 1);
    CHECK(opts.get<"--int">()[1] == 2);
    CHECK(opts.get<"--string">()[0] == "foo");
    CHECK(opts.get<"--string">()[1] == "bar");
}

TEST_CASE("Multiple + Positional works") {
    using options = clopts<
        multiple<option<"--int", "Integers", int64_t, true>>,
        multiple<option<"--string", "Strings", std::string, true>>,
        multiple<positional<"rest", "The remaining arguments", std::string, false>>>;

    std::array args = {
        "test",
        "--int",
        "1",
        "baz",
        "--string",
        "foo",
        "--int",
        "2",
        "--string",
        "bar",
        "qux",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    CHECK(opts.get<"--int">().size() == 2);
    CHECK(opts.get<"--string">().size() == 2);
    CHECK(opts.get<"rest">().size() == 2);

    CHECK(opts.get<"--int">()[0] == 1);
    CHECK(opts.get<"--int">()[1] == 2);
    CHECK(opts.get<"--string">()[0] == "foo");
    CHECK(opts.get<"--string">()[1] == "bar");
    CHECK(opts.get<"rest">()[0] == "baz");
    CHECK(opts.get<"rest">()[1] == "qux");
}

TEST_CASE("Calling from main() works as expected") {
    using options = clopts<option<"--number", "A number", int64_t>>;

    std::array backing_args = {
        "test"s,
        "--number"s,
        "42"s,
    };

    std::array args = {
        backing_args[0].data(),
        backing_args[1].data(),
        backing_args[2].data(),
    };

    int argc = int(args.size());
    char** argv = args.data();
    auto opts = options::parse(argc, argv, error_handler);

    REQUIRE(opts.get<"--number">());
    CHECK(*opts.get<"--number">() == 42);
}

TEST_CASE("File option can map a file properly") {
    auto run = []<typename file> {
        using options = clopts<option<"file", "A file", file>>;

        std::array args = {
            "test",
            "file",
            __FILE__,
        };

        auto [path, contents] = this_file<typename file::path_type>();
        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.template get<"file">());
        CHECK(opts.template get<"file">()->path == path);
        CHECK(ByteSpan(opts.template get<"file">()->contents) == ByteSpan(contents));
    };

    run.template operator()<file<>>();
    run.template operator()<file<std::vector<char>>>();
    run.template operator()<file<std::string, std::string>>();
    run.template operator()<file<std::string, std::vector<char>>>();
}

TEST_CASE("stop_parsing<> option") {
    using options = clopts<
        multiple<option<"--foo", "Foo option", std::string, true>>,
        flag<"--bar", "Bar option">,
        stop_parsing<"stop">>;

    SECTION("stops parsing") {
        std::array args = {
            "test",
            "--foo",
            "arg",
            "--foo",
            "stop", /// Argument of '--foo'
            "stop", /// Stop parsing
            "--bar",
            "--foo", /// Missing argument, but ignored because it’s after 'stop'.
        };

        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.get<"--foo">().size() == 2);
        CHECK(opts.get<"--foo">()[0] == "arg");
        CHECK(opts.get<"--foo">()[1] == "stop");
        CHECK(not opts.get<"--bar">());

        auto unprocessed = opts.unprocessed();
        REQUIRE(unprocessed.size() == 2);
        CHECK(unprocessed[0] == "--bar"sv);
        CHECK(unprocessed[1] == "--foo"sv);
    }

    SECTION("errors if there are missing required options") {
        std::array args = {
            "test",
            "stop"
        };

        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("is never required") {
        std::array args = {
            "test",
            "--foo",
            "arg",
        };

        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.get<"--foo">().size() == 1);
        CHECK(opts.get<"--foo">()[0] == "arg");
        CHECK(opts.unprocessed().empty());
    }

    SECTION("is effectively a no-op if it’s the last argument") {
        std::array args = {
            "test",
            "--foo",
            "arg",
            "stop",
        };

        auto opts = options::parse(args.size(), args.data(), error_handler);
        REQUIRE(opts.get<"--foo">().size() == 1);
        CHECK(opts.get<"--foo">()[0] == "arg");
        CHECK(opts.unprocessed().empty());
    }

    SECTION("uses '--' by default") {
        using options2 = clopts<
            flag<"--bar", "Bar option">,
            stop_parsing<>>;

        std::array args = {
            "test",
            "--",
            "--bar",
        };

        auto opts = options2::parse(args.size(), args.data(), error_handler);
        REQUIRE(not opts.get<"--bar">());

        auto unprocessed = opts.unprocessed();
        REQUIRE(unprocessed.size() == 1);
        CHECK(unprocessed[0] == "--bar"sv);
    }

    SECTION("can occur multiple times") {
        using options2 = clopts<
            flag<"--bar", "Bar option">,
            stop_parsing<>,
            stop_parsing<"stop">>;

        std::array args1 = {
            "test",
            "--",
            "--bar",
        };

        std::array args2 = {
            "test",
            "stop",
            "--baz",
        };

        auto opts1 = options2::parse(args1.size(), args1.data(), error_handler);
        auto opts2 = options2::parse(args2.size(), args2.data(), error_handler);
        auto unprocessed1 = opts1.unprocessed();
        auto unprocessed2 = opts2.unprocessed();

        REQUIRE(not opts1.get<"--bar">());
        REQUIRE(not opts2.get<"--bar">());
        REQUIRE(unprocessed1.size() == 1);
        REQUIRE(unprocessed2.size() == 1);
        CHECK(unprocessed1[0] == "--bar"sv);
        CHECK(unprocessed2[0] == "--baz"sv);
    }
}

TEST_CASE("Parser does not crash on invalid input") {
    std::array<const char*, 0> args1 = {};
    std::array args2 = { "test" };

    (void) basic_options::parse(args1.size(), args1.data(), error_handler);
    (void) basic_options::parse(args2.size(), args2.data(), error_handler);
}

TEST_CASE("Overridable options work") {
    std::array args = {
        "test",
        "-x", "a",
        "-x", "b",
        "-x", "c",
    };

    using options1 = clopts<option<"-x", "A string", std::string, false, true>>;
    using options2 = clopts<overridable<"-x", "A string">>;

    auto opts1 = options1::parse(args.size(), args.data(), error_handler);
    auto opts2 = options2::parse(args.size(), args.data(), error_handler);

    CHECK(*opts1.get<"-x">() == "c");
    CHECK(*opts2.get<"-x">() == "c");
}

TEST_CASE("Options can reference other options") {
    using options = clopts<
        overridable<"-x", "type">,
        multiple<option<"-y", "tagged", ref<std::string, "-x", "-x">>>
    >;

    std::array args = {
        "test",
        "-y", "x",
        "-x", "1",
        "-y", "4"
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);

    using tuple = std::tuple<
        std::string,
        std::optional<std::string>,
        std::optional<std::string>
    >;
    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(opts.get<"-y">())>,
        MutableSpan<tuple>
    >);

    auto vals = opts.get<"-y">();
    REQUIRE(vals.size() == 2);
    CHECK((vals[0] == tuple{"x", std::nullopt, std::nullopt}));
    CHECK((vals[1] == tuple{"4", "1", "1"}));
}

TEST_CASE("More complex option referencing examples") {
    std::array args = {
        "test",
        "-v", "a",
        "-v", "b",
        "--flag",
        "-v", "c",
        "-x", "foo",
        "-v", "d",
        "-v", "e",
        "-x", "bar",
        "-v", "f",
        "-v", "g",
        "-x", "",
        "-v", "h",
    };

    using options = clopts<
        flag<"--flag", "flag">,
        overridable<"-x", "switch">,
        multiple<option<"-v", "value", ref<std::string, "--flag", "-x">>>
    >;

    auto opts = options::parse(args.size(), args.data(), error_handler);
    auto vals = opts.get<"-v">();

    using tuple = std::tuple<std::string, bool, std::optional<std::string>>;
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(vals)>, MutableSpan<tuple>>);

    REQUIRE(vals.size() == 8);
    CHECK((vals[0] == tuple{"a", false, std::nullopt}));
    CHECK((vals[1] == tuple{"b", false, std::nullopt}));
    CHECK((vals[2] == tuple{"c", true, std::nullopt}));
    CHECK((vals[3] == tuple{"d", true, "foo"}));
    CHECK((vals[4] == tuple{"e", true, "foo"}));
    CHECK((vals[5] == tuple{"f", true, "bar"}));
    CHECK((vals[6] == tuple{"g", true, "bar"}));
    CHECK((vals[7] == tuple{"h", true, ""}));
}

TEST_CASE("multiple ref<> referencing a multiple<> option.") {
    using options = clopts<
        multiple<option<"-v", "value">>,
        multiple<option<"--all", "value", ref<std::string, "-v">>>
    >;

    std::array args = {
        "test",
        "--all", "a",
        "-v", "foo",
        "--all", "b",
        "-v", "bar",
        "--all", "c",
    };

    using vector = std::vector<std::string>;
    using tuple = std::tuple<std::string, vector>;
    auto opts = options::parse(args.size(), args.data(), error_handler);
    auto vals = opts.get<"-v">();
    auto all = opts.get<"--all">();

    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(vals)>,
        MutableSpan<std::string>
    >);

    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(all)>,
        MutableSpan<tuple>
    >);

    REQUIRE(vals.size() == 2);
    CHECK(vals[0] == "foo");
    CHECK(vals[1] == "bar");

    REQUIRE(all.size() == 3);
    CHECK((all[0] == tuple{"a", vector{}}));
    CHECK((all[1] == tuple{"b", vector{"foo"}}));
    CHECK((all[2] == tuple{"c", vector{"foo", "bar"}}));
}


TEST_CASE("ref<> referencing a multiple<> option.") {
    using options = clopts<
        multiple<option<"-v", "value">>,
        option<"--all", "value", ref<std::string, "-v">>
    >;

    std::array args1 = {
        "test",
        "--all", "a",
        "-v", "foo",
    };

    std::array args2 = {
        "test",
        "-v", "foo",
        "-v", "bar",
        "--all", "a",
    };

    using vector = std::vector<std::string>;
    using tuple = std::tuple<std::string, vector>;
    auto opts1 = options::parse(args1.size(), args1.data(), error_handler);
    auto opts2 = options::parse(args2.size(), args2.data(), error_handler);

    auto vals1 = opts1.get<"-v">();
    auto vals2 = opts2.get<"-v">();
    auto all1 = opts1.get<"--all">();
    auto all2 = opts2.get<"--all">();

    REQUIRE(vals1.size() == 1);
    REQUIRE(vals2.size() == 2);
    REQUIRE(vals1[0] == "foo");
    REQUIRE(vals2[0] == "foo");
    REQUIRE(vals2[1] == "bar");
    REQUIRE(all1);
    REQUIRE(all2);

    CHECK((*all1 == tuple{"a", vector{}}));
    CHECK((*all2 == tuple{"a", vector{"foo", "bar"}}));
}

TEST_CASE("multiple<positional<ref>> works") {
    using options = clopts<
        multiple<positional<"file", "The file to compile", ref<std::string, "-x">>>,
        short_option<"-x", "Override the language", std::string, false, true>,
        help<>
    >;

    std::array args = {
        "test",
        "-xfoo",
        "bar",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    auto files = opts.get<"file">();

    using tuple = std::tuple<std::string, std::optional<std::string>>;
    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(files)>,
        MutableSpan<tuple>
    >);

    REQUIRE(files.size() == 1);
    CHECK((files[0] == tuple{"bar", "foo"}));
}

TEST_CASE("Documentation compiles (example 1)") {
    using options = clopts<
        option<"--repeat", "How many times the output should be repeated (default 1)", int64_t>,
        positional<"file", "The file whose contents should be printed", file<>, /*required=*/true>,
        help<>>;

    std::array args = {
        "test",
        "--repeat",
        "3",
        __FILE__,
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    auto& file_contents = opts.get<"file">()->contents;
    auto repeat_count = opts.get<"--repeat">(1);
    std::string actual;
    for (std::int64_t i = 0; i < repeat_count; i++)
        actual += std::format("{}", file_contents);

    auto [path, contents] = this_file();
    CHECK(actual == contents + contents + contents);
}

TEST_CASE("Documentation compiles (example 2)") {
    using options = clopts<
        positional<"file", "The name of the file", file<std::vector<std::byte>>, true>,
        positional<"foobar", "[description goes here]", std::string, false>,
        option<"--size", "The size parameter (whatever that means)", int64_t>,
        multiple<option<"--int", "Integers", int64_t, true>>,
        flag<"--test", "Test flag">,
        option<"--prime", "A prime number that is less than 14", values<2, 3, 5, 7, 11, 13>>,
        func<"--func", "Print 42 and exit", print_number_and_exit>,
        help<>>;

    std::array args = {
        "test",
        __FILE__,
        "--int",
        "3",
        "--int",
        "42",
    };

    std::string out;
    int number = 42;
    auto opts = options::parse(args.size(), args.data(), nullptr, &number);

    auto ints = opts.get<"--int">();
    if (ints.empty()) out = "No ints!\n";
    else for (auto i : ints) out += std::to_string(i) + "\n";

    CHECK(out == "3\n42\n");
}

TEST_CASE("Help message is formatted correctly") {
    using options = clopts<
        positional<"pos", "Description of parameter pos">,
        positional<"int-pos", "Description of parameter int-pos", std::int64_t, false>,
        option<"--str", "Description of parameter --str", std::string>,
        option<"--int", "Description of parameter --int", std::int64_t>,
        flag<"--flag", "Description of parameter --flag">,
        option<"--str-values", "Description of parameter --str-values", values<"foo", "bar", "baz">>,
        option<"--int-values", "Description of parameter --int-values", values<1, 2, 3, 4, 5>>,
        overridable<"--ref", "Description of reference parameter", ref<double, "--int">>,
        help<>
    >;

    static constexpr auto expected = R"help(<pos> [<int-pos>] [options]

Arguments:
    <int-pos>     Description of parameter int-pos
    <pos>         Description of parameter pos

Options:
    --flag        Description of parameter --flag
    --help        Print this help information
    --int         Description of parameter --int
    --int-values  Description of parameter --int-values
    --ref         Description of reference parameter
    --str         Description of parameter --str
    --str-values  Description of parameter --str-values

Supported option values:
    --int-values: 1, 2, 3, 4, 5
    --str-values: foo, bar, baz
)help";

    CHECK(options::help() == expected);
}

static_assert(std::is_same_v<
    detail::concat<utils::list<int, double>, utils::list<char, short>>,
    utils::list<int, double, char, short>
>);

// The filter<> implementation used to be exponential, so er,
// this checks that we can actually handle a large amount of
// options.
//
// FIXME: This test *works* but it takes like 20 seconds to
// compile. Investigate why.
TEST_CASE("Stress test") {
    using options = clopts<
        option<"--1", "1">,
        option<"--2", "2">,
        option<"--3", "3">,
        option<"--4", "4">,
        option<"--5", "5">,
        option<"--6", "6">,
        option<"--7", "7">,
        option<"--8", "8">,
        option<"--9", "9">,
        option<"--10", "10">,
        option<"--11", "11">,
        option<"--12", "12">,
        option<"--13", "13">,
        option<"--14", "14">,
        option<"--15", "15">,
        option<"--16", "16">,
        option<"--17", "17">,
        option<"--18", "18">,
        option<"--19", "19">,
        option<"--20", "20">,
        option<"--21", "21">,
        option<"--22", "22">,
        option<"--23", "23">,
        option<"--24", "24">,
        option<"--25", "25">,
        option<"--26", "26">,
        option<"--27", "27">,
        option<"--28", "28">,
        option<"--29", "29">,
        option<"--30", "30">,
        option<"--31", "31">,
        option<"--32", "32">,
        option<"--33", "33">,
        option<"--34", "34">,
        option<"--35", "35">,
        option<"--36", "36">,
        option<"--37", "37">,
        option<"--38", "38">,
        option<"--39", "39">,
        option<"--40", "40">,
        option<"--41", "41">,
        option<"--42", "42">,
        option<"--43", "43">,
        option<"--44", "44">,
        option<"--45", "45">,
        option<"--46", "46">,
        option<"--47", "47">,
        option<"--48", "48">,
        option<"--49", "49">,
        option<"--50", "50">,
        option<"--51", "51">,
        option<"--52", "52">,
        option<"--53", "53">,
        option<"--54", "54">,
        option<"--55", "55">,
        option<"--56", "56">,
        option<"--57", "57">,
        option<"--58", "58">,
        option<"--59", "59">,
        option<"--60", "60">,
        option<"--61", "61">,
        option<"--62", "62">,
        option<"--63", "63">,
        option<"--64", "64">,
        option<"--65", "65">,
        option<"--66", "66">,
        option<"--67", "67">,
        option<"--68", "68">,
        option<"--69", "69">,
        option<"--70", "70">,
        option<"--71", "71">,
        option<"--72", "72">,
        option<"--73", "73">,
        option<"--74", "74">,
        option<"--75", "75">,
        option<"--76", "76">,
        option<"--77", "77">,
        option<"--78", "78">,
        option<"--79", "79">,
        option<"--80", "80">,
        option<"--81", "81">,
        option<"--82", "82">,
        option<"--83", "83">,
        option<"--84", "84">,
        option<"--85", "85">,
        option<"--86", "86">,
        option<"--87", "87">,
        option<"--88", "88">,
        option<"--89", "89">,
        option<"--90", "90">,
        option<"--91", "91">,
        option<"--92", "92">,
        option<"--93", "93">,
        option<"--94", "94">,
        option<"--95", "95">,
        option<"--96", "96">,
        option<"--97", "97">,
        option<"--98", "98">,
        option<"--99", "99">,
        option<"--100", "100">,
        help<>
    >;

    std::array args{"test"};
    (void) options::parse(args.size(), args.data(), error_handler);
}

TEST_CASE("Check that we handle an option being a prefix of another option properly") {
    using options = clopts<
        help<>,
        flag<"--ir", "">,
        flag<"--ir-generic", "">
    >;

    std::array args = {
        "test",
        "--ir",
        "--ir-generic",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    CHECK(opts.get<"--ir">());
    CHECK(opts.get<"--ir-generic">());
}

TEST_CASE("Option name must match exactly or be followed by '='") {
    using options = clopts<
        help<>,
        option<"--ir", "">
    >;

    std::array args = {
        "test",
        "--irx",
    };

    CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
}

TEST_CASE("get() with default") {
    std::array args = { "test" };
    auto opts = basic_options::parse(args.size(), args.data(), error_handler);
    CHECK(opts.get<"--string">("foo") == "foo");
    CHECK(opts.get<"--number">(42) == 42);
    CHECK(opts.get<"--float">(3.141592653589) == 3.141592653589_a);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CHECK(opts.get_or<"--string">("foo") == "foo");
    CHECK(opts.get_or<"--number">(42) == 42);
    CHECK(opts.get_or<"--float">(3.141592653589) == 3.141592653589_a);
#pragma clang diagnostic pop
}

TEST_CASE("Clopts: mutually_exclusive") {
    using options = clopts<
        help<>,
        option<"--a", "">,
        option<"--b", "">,
        mutually_exclusive<"--a", "--b">
    >;

    std::array a1 = {
        "test",
        "--a", "a",
        "--b", "b",
    };

    CHECK_THROWS_WITH(
        options::parse(a1.size(), a1.data(), error_handler),
        ContainsSubstring("Options '--a' and '--b' are mutually exclusive")
    );

    std::array a2 = { "test" };
    std::array a3 = { "test", "--a", "" };
    std::array a4 = { "test", "--b", "" };
    CHECK_NOTHROW(options::parse(a2.size(), a2.data(), error_handler));
    CHECK_NOTHROW(options::parse(a3.size(), a3.data(), error_handler));
    CHECK_NOTHROW(options::parse(a4.size(), a4.data(), error_handler));

    using options2 = clopts<
        help<>,
        flag<"--a", "">,
        flag<"--b", "">,
        flag<"--c", "">,
        flag<"--d", "">,
        mutually_exclusive<"--a", "--b", "--c", "--d">
    >;

    std::array a5 = { "test", "--a", "--b" };
    CHECK_THROWS_WITH(
        options2::parse(a5.size(), a5.data(), error_handler),
        ContainsSubstring("Options '--a' and '--b' are mutually exclusive")
    );

    std::array a6 = { "test", "--a", "--c" };
    CHECK_THROWS_WITH(
        options2::parse(a6.size(), a6.data(), error_handler),
        ContainsSubstring("Options '--a' and '--c' are mutually exclusive")
    );

    std::array a7 = { "test", "--b", "--c" };
    CHECK_THROWS_WITH(
        options2::parse(a7.size(), a7.data(), error_handler),
        ContainsSubstring("Options '--b' and '--c' are mutually exclusive")
    );

    std::array a8 = { "test", "--b", "--d" };
    CHECK_THROWS_WITH(
        options2::parse(a8.size(), a8.data(), error_handler),
        ContainsSubstring("Options '--b' and '--d' are mutually exclusive")
    );

    std::array a9 = { "test", "--d", "--c" };
    CHECK_THROWS_WITH(
        options2::parse(a9.size(), a9.data(), error_handler),
        ContainsSubstring("Options '--c' and '--d' are mutually exclusive")
    );

    std::array a10 = { "test", "--a", "--c", "--b" };
    CHECK_THROWS_WITH(
        options2::parse(a10.size(), a10.data(), error_handler),
        ContainsSubstring("Options '--a' and '--b' are mutually exclusive")
    );

    std::array a11 = { "test", "--d", "--a", "--c" };
    CHECK_THROWS_WITH(
        options2::parse(a11.size(), a11.data(), error_handler),
        ContainsSubstring("Options '--a' and '--c' are mutually exclusive")
    );

    std::array a12 = { "test", "--d", "--a" };
    CHECK_THROWS_WITH(
        options2::parse(a12.size(), a12.data(), error_handler),
        ContainsSubstring("Options '--a' and '--d' are mutually exclusive")
    );
}

TEST_CASE("Clopts: hidden<>") {
    using options = clopts<
        positional<"pos", "Description of parameter pos">,
        positional<"int-pos", "Description of parameter int-pos", std::int64_t, false>,
        option<"--str", "Description of parameter --str", std::string, false, false, true>,
        option<"--int", "Description of parameter --int", std::int64_t>,
        flag<"--flag", "Description of parameter --flag", true>,
        option<"--str-values", "Description of parameter --str-values", values<"foo", "bar", "baz">, false, false, true>,
        option<"--int-vals", "Description of parameter --int-values", values<1, 2, 3, 4, 5>>,
        overridable<"--ref", "Description of reference parameter", ref<double, "--int">>,
        help<>
    >;

    static constexpr auto expected = R"help(<pos> [<int-pos>] [options]

Arguments:
    <int-pos>   Description of parameter int-pos
    <pos>       Description of parameter pos

Options:
    --help      Print this help information
    --int       Description of parameter --int
    --int-vals  Description of parameter --int-values
    --ref       Description of reference parameter

Supported option values:
    --int-vals: 1, 2, 3, 4, 5
)help";

    CHECK(options::help() == expected);
}

TEST_CASE("Clopts: Omit supported values if all values<> options are hidden") {
    using options = clopts<
        positional<"pos", "Description of parameter pos">,
        positional<"int-pos", "Description of parameter int-pos", std::int64_t, false>,
        hidden<"--str", "Description of parameter --str">,
        option<"--int", "Description of parameter --int", std::int64_t>,
        flag<"--flag", "Description of parameter --flag", true>,
        option<"--str-values", "Description of parameter --str-values", values<"foo", "bar", "baz">, false, false, true>,
        option<"--int-vals", "Description of parameter --int-values", values<1, 2, 3, 4, 5>, false, false, true>,
        overridable<"--ref", "Description of reference parameter", ref<double, "--int">>,
        help<>
    >;

    static constexpr auto expected = R"help(<pos> [<int-pos>] [options]

Arguments:
    <int-pos>  Description of parameter int-pos
    <pos>      Description of parameter pos

Options:
    --help     Print this help information
    --int      Description of parameter --int
    --ref      Description of reference parameter
)help";

    CHECK(options::help() == expected);
}

TEST_CASE("Clopts: Option name sorting ignores ASCII case and dashes") {
    using options = clopts<
        positional<"a", "foobar">,
        positional<"b", "foobar">,
        positional<"A", "foobar">,
        positional<"B", "foobar">,
        option<"c", "foobar", values<1, 2>>,
        option<"d", "foobar", values<1, 2>>,
        option<"C", "foobar", values<1, 2>>,
        option<"D", "foobar", values<1, 2>>,
        option<"-c", "foobar", values<1, 2>>,
        option<"-C", "foobar", values<1, 2>>,
        option<"-d", "foobar", values<1, 2>>,
        option<"-D", "foobar", values<1, 2>>,
        option<"--c", "foobar", values<1, 2>>,
        option<"--C", "foobar", values<1, 2>>,
        option<"--D", "foobar", values<1, 2>>,
        option<"--d", "foobar", values<1, 2>>,
        help<>
    >;

    static constexpr auto expected = R"help(<a> <b> <A> <B> [options]

Arguments:
    <a>     foobar
    <A>     foobar
    <b>     foobar
    <B>     foobar

Options:
    c       foobar
    C       foobar
    -c      foobar
    -C      foobar
    --c     foobar
    --C     foobar
    d       foobar
    D       foobar
    -d      foobar
    -D      foobar
    --D     foobar
    --d     foobar
    --help  Print this help information

Supported option values:
    c:   1, 2
    C:   1, 2
    -c:  1, 2
    -C:  1, 2
    --c: 1, 2
    --C: 1, 2
    d:   1, 2
    D:   1, 2
    -d:  1, 2
    -D:  1, 2
    --D: 1, 2
    --d: 1, 2
)help";

    CHECK(options::help() == expected);
}

TEST_CASE("Clopts: subcommands") {
    using options = clopts<
        help<>,
        option<"ipa", "Convert [REDACTED] to IPA">,
        subcommand<"dictionary", "Generate the dictionary",
            option<"--file", "Dictionary file">,
            option<"--imports", "Imports file">
        >
    >;

    SECTION("1") {
        std::array args = {
            "test",
            "ipa", "foobar"
        };

        auto opts = options::parse(args.size(), args.data(), error_handler);
        CHECK(*opts.get<"ipa">() == "foobar");
    }

    SECTION("2") {
        std::array args = {
            "test",
            "ipa", "foobar",
            "--file", "1"
        };

        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("3") {
        std::array args = {
            "test",
            "--file", "1"
        };

        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("4") {
        std::array args = {
            "test",
            "dictionary",
            "--file", "1",
            "--imports", "2",
        };

        auto opts = options::parse(args.size(), args.data(), error_handler);
        CHECK(*opts.get<"dictionary">()->get<"--file">() == "1");
        CHECK(*opts.get<"dictionary">()->get<"--imports">() == "2");
    }

    SECTION("5") {
        std::array args = {
            "test",
            "dictionary",
            "--file", "1",
            "ipa", "2",
        };

        CHECK_THROWS(options::parse(args.size(), args.data(), error_handler));
    }

    SECTION("help message") {
        static constexpr auto root_help = R"help([options]

Subcommands:
    dictionary  Generate the dictionary

Options:
    --help      Print this help information
    ipa         Convert [REDACTED] to IPA
)help";

        static constexpr auto subcommand_help = R"help([options]

Options:
    --file     Dictionary file
    --help     Print this help information
    --imports  Imports file
)help";

        CHECK(options::help() == root_help);
        CHECK(options::command<"dictionary">::help() == subcommand_help);
    }
}

TEST_CASE("Clopts: Integer types") {
    using options = clopts<
        option<"--i8", "", i8>,
        option<"--u8", "", u8>,
        option<"--i16", "", i16>,
        option<"--u16", "", u16>,
        option<"--i32", "", i32>,
        option<"--u32", "", u32>,
        option<"--i64", "", i64>,
        option<"--u64", "", u64>
    >;

    std::array args = {
        "test",
        "--i8", "42",
        "--u8", "43",
        "--i16", "44",
        "--u16", "45",
        "--i32", "46",
        "--u32", "47",
        "--i64", "48",
        "--u64", "49",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    static_assert(utils::is<decltype(*opts.get<"--i8">()), i8>);
    static_assert(utils::is<decltype(*opts.get<"--u8">()), u8>);
    static_assert(utils::is<decltype(*opts.get<"--i16">()), i16>);
    static_assert(utils::is<decltype(*opts.get<"--u16">()), u16>);
    static_assert(utils::is<decltype(*opts.get<"--i32">()), i32>);
    static_assert(utils::is<decltype(*opts.get<"--u32">()), u32>);
    static_assert(utils::is<decltype(*opts.get<"--i64">()), i64>);
    static_assert(utils::is<decltype(*opts.get<"--u64">()), u64>);

    CHECK(*opts.get<"--i8">() == 42);
    CHECK(*opts.get<"--u8">() == 43);
    CHECK(*opts.get<"--i16">() == 44);
    CHECK(*opts.get<"--u16">() == 45);
    CHECK(*opts.get<"--i32">() == 46);
    CHECK(*opts.get<"--u32">() == 47);
    CHECK(*opts.get<"--i64">() == 48);
    CHECK(*opts.get<"--u64">() == 49);
}

#ifdef LIBBASE_I128_AVAILABLE
TEST_CASE("Clopts: i128") {
    using options = clopts<
        option<"--i128", "", i128>,
        option<"--u128", "", u128>
    >;

    std::array args = {
        "test",
        "--i128", "42",
        "--u128", "43",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    static_assert(utils::is<decltype(*opts.get<"--i128">()), i128>);
    static_assert(utils::is<decltype(*opts.get<"--u128">()), u128>);

    CHECK(*opts.get<"--i128">() == 42);
    CHECK(*opts.get<"--u128">() == 43);
}
#endif

template <typename T>
static void TestInt(const char* value) {
    std::array args = { "test", "x", value };
    (void) clopts<option<"x", "", T>>::parse(args.size(), args.data(), error_handler);
}

TEST_CASE("Clopts: integer types: edge cases") {
    CHECK_NOTHROW(TestInt<i8>("0"));
    CHECK_NOTHROW(TestInt<i8>("1"));
    CHECK_NOTHROW(TestInt<i8>("127"));
    CHECK_NOTHROW(TestInt<i8>("-1"));
    CHECK_NOTHROW(TestInt<i8>("-128"));
    CHECK_THROWS(TestInt<i8>("128"));
    CHECK_THROWS(TestInt<i8>("-129"));
    CHECK_THROWS(TestInt<i8>("asdadasd"));
    CHECK_THROWS(TestInt<i8>("-asdadasd"));


    CHECK_NOTHROW(TestInt<i16>("0"));
    CHECK_NOTHROW(TestInt<i16>("1"));
    CHECK_NOTHROW(TestInt<i16>("-1"));
    CHECK_NOTHROW(TestInt<i16>("32767"));
    CHECK_NOTHROW(TestInt<i16>("-32768"));
    CHECK_THROWS(TestInt<i16>("32768"));
    CHECK_THROWS(TestInt<i16>("-32769"));
    CHECK_THROWS(TestInt<i16>("foo"));

    CHECK_NOTHROW(TestInt<i32>("0"));
    CHECK_NOTHROW(TestInt<i32>("1"));
    CHECK_NOTHROW(TestInt<i32>("-1"));
    CHECK_NOTHROW(TestInt<i32>("2147483647"));
    CHECK_NOTHROW(TestInt<i32>("-2147483648"));
    CHECK_THROWS(TestInt<i32>("2147483648"));
    CHECK_THROWS(TestInt<i32>("-2147483649"));
    CHECK_THROWS(TestInt<i32>("foo"));

    CHECK_NOTHROW(TestInt<i64>("0"));
    CHECK_NOTHROW(TestInt<i64>("1"));
    CHECK_NOTHROW(TestInt<i64>("-1"));
    CHECK_NOTHROW(TestInt<i64>("9223372036854775807"));
    CHECK_NOTHROW(TestInt<i64>("-9223372036854775808"));
    CHECK_THROWS(TestInt<i64>("9223372036854775808"));
    CHECK_THROWS(TestInt<i64>("-9223372036854775809"));
    CHECK_THROWS(TestInt<i64>("foo"));

    CHECK_NOTHROW(TestInt<u8>("0"));
    CHECK_NOTHROW(TestInt<u8>("1"));
    CHECK_NOTHROW(TestInt<u8>("255"));
    CHECK_THROWS(TestInt<u8>("256"));
    CHECK_THROWS(TestInt<u8>("-1"));
    CHECK_THROWS(TestInt<u8>("-255"));
    CHECK_THROWS(TestInt<i8>("asdadasd"));
    CHECK_THROWS(TestInt<i8>("-asdadasd"));

    CHECK_NOTHROW(TestInt<u16>("0"));
    CHECK_NOTHROW(TestInt<u16>("1"));
    CHECK_NOTHROW(TestInt<u16>("65535"));
    CHECK_THROWS(TestInt<u16>("65536"));
    CHECK_THROWS(TestInt<u16>("-1"));
    CHECK_THROWS(TestInt<u16>("foo"));

    CHECK_NOTHROW(TestInt<u32>("0"));
    CHECK_NOTHROW(TestInt<u32>("1"));
    CHECK_NOTHROW(TestInt<u32>("4294967295"));
    CHECK_THROWS(TestInt<u32>("4294967296"));
    CHECK_THROWS(TestInt<u32>("-1"));
    CHECK_THROWS(TestInt<u32>("foo"));

    CHECK_NOTHROW(TestInt<u64>("0"));
    CHECK_NOTHROW(TestInt<u64>("1"));
    CHECK_NOTHROW(TestInt<u64>("18446744073709551615"));
    CHECK_THROWS(TestInt<u64>("18446744073709551616"));
    CHECK_THROWS(TestInt<u64>("-1"));
    CHECK_THROWS(TestInt<u64>("foo"));
}

#ifdef LIBBASE_I128_AVAILABLE
TEST_CASE("Clopts: i128: edge cases") {
    CHECK_NOTHROW(TestInt<i128>("0"));
    CHECK_NOTHROW(TestInt<i128>("1"));
    CHECK_NOTHROW(TestInt<i128>("-1"));
    CHECK_NOTHROW(TestInt<i128>("170141183460469231731687303715884105727"));
    CHECK_NOTHROW(TestInt<i128>("-170141183460469231731687303715884105728"));
    CHECK_THROWS(TestInt<i128>("170141183460469231731687303715884105728"));
    CHECK_THROWS(TestInt<i128>("-170141183460469231731687303715884105729"));
    CHECK_THROWS(TestInt<i128>("foo"));

    CHECK_NOTHROW(TestInt<u128>("0"));
    CHECK_NOTHROW(TestInt<u128>("1"));
    CHECK_NOTHROW(TestInt<u128>("340282366920938463463374607431768211455"));
    CHECK_THROWS(TestInt<u128>("340282366920938463463374607431768211456"));
    CHECK_THROWS(TestInt<u64>("-1"));
    CHECK_THROWS(TestInt<u128>("foo"));
}
#endif

/*TEST_CASE("Aliased options are equivalent") {
    using options = clopts<
        multiple<option<"--string", "A string", std::string>>,
        alias<"-s", "--string">
    >;

    std::array args = {
        "test",
        "--string",
        "123",
        "-s",
        "456",
    };

    auto opts = options::parse(args.size(), args.data(), error_handler);
    REQUIRE(opts.get<"--string">() == opts.get<"-s">());
    REQUIRE(opts.get<"-s">()->size() == 2);
    CHECK(opts.get<"-s">()->at(0) == "123");
    CHECK(opts.get<"-s">()->at(1) == "456");
}*/

/// TODO:
///  - alias<"-f", "--filename">; alternatively: option<names<"-f", "--filename">, "description">
///  - Finish short_option
