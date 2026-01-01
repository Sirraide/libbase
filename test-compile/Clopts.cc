#include <base/Clopts.hh>
using namespace base::cmd;

void callback(std::string_view) {}

using o1 = clopts<multiple<multiple<option<"--bar", "Bar">>>>; // expected-error@base/Clopts.hh:* {{multiple<multiple<>> is invalid}}
using o2 = clopts<multiple<stop_parsing<>>>; // expected-error@base/Clopts.hh:* {{multiple<stop_parsing<>> is invalid}}
using o3 = clopts<multiple<func<"foo", "bar", callback>>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be a callback}}
using o4 = clopts<multiple<help<>>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be a callback}}
using o5 = clopts<multiple<flag<"foo", "bar">>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be bool}}
using o9 = clopts<multiple<overridable<"foo", "bar">>>; // expected-error@base/Clopts.hh:* {{multiple<> cannot be overridable}}
using o13 = clopts<option<"a", "">, mutually_exclusive<"a">>; // expected-error@base/Clopts.hh:* {{mutually_exclusive<> must have at least 2 arguments}}
using o14 = clopts<option<"a", "">, mutually_exclusive<"a", "a">>; // expected-error@base/Clopts.hh:* {{mutually_exclusive<>: an option cannot be exclusive with itself}}
using o15 = clopts<option<"a", "">, mutually_exclusive<"a", "b", "a">>; // expected-error@base/Clopts.hh:* {{mutually_exclusive<>: an option cannot be exclusive with itself}}
using o16 = clopts<option<"a", "", std::string, {.required = true, .hidden = true}>>; // expected-error@base/Clopts.hh:* {{Required options cannot be hidden}}
using o19 = clopts<multiple<subcommand<"foo", "bar", flag<"foo", "bar">>>>; // expected-error@base/Clopts.hh:* {{multiple<subcommand<>> is invalid}}

int a(int argc, char** argv) {
    // expected-error@base/Clopts.hh:* {{Cannot have more than one multiple<positional<>> option}}
    using o6 = clopts<multiple<positional<"foo", "bar">>, multiple<positional<"baz", "bar">>>;
    (void) o6::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Two different options may not have the same name}}
    using o7 = clopts<option<"foo", "bar">, flag<"foo", "baz">>;
    (void) o7::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{At least one option is required}}
    using o8 = clopts<>;
    (void) o8::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{mutually_exclusive<> must reference existing option}}
    using o10 = clopts<option<"a", "">, mutually_exclusive<"x", "y">>;
    (void) o10::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{mutually_exclusive<> must reference existing option}}
    using o11 = clopts<option<"a", "">, mutually_exclusive<"a", "b">>;
    (void) o11::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{mutually_exclusive<> must reference existing option}}
    using o12 = clopts<option<"b", "">, mutually_exclusive<"a", "b">>;
    (void) o12::parse(argc, argv);

    // Ok.
    using o16 = clopts<option<"a", "">, positional<"b", "">, mutually_exclusive<"a", "b">>;
    (void) o16::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Cannot mark two required options as mutually_exclusive<>}}
    using o17 = clopts<positional<"a", "">, positional<"b", "">, mutually_exclusive<"a", "b">>;
    (void) o17::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Cannot mark two required options as mutually_exclusive<>}}
    using o18 = clopts<positional<"a", "">, option<"c", "">, positional<"b", "">, mutually_exclusive<"a", "c", "b">>;
    (void) o18::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Alias 'z' already references option 'a'}}
    using o20 = clopts<option<"a", "">, option<"b", "">, alias<"z", "a">, alias<"z", "b">>;
    (void) o20::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Option 'b' referenced by alias 'z' does not exist}}
    using o21 = clopts<option<"a", "">, alias<"z", "b">>;
    (void) o21::parse(argc, argv);

    // expected-error@base/Clopts.hh:* {{Alias 'z' references a positional option: 'a'}}
    using o22 = clopts<positional<"a", "">, alias<"z", "a">>;
    (void) o22::parse(argc, argv);
}

// expected-note@*           0+ {{}}
// expected-note@base/Clopts.hh:* 0+ {{}}
// expected-error@base/Clopts.hh:* 0+ {{no type named 'declared_type' in}}
