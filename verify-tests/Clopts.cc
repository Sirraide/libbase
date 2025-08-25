#include <clopts.hh> // expected-warning@clopts.hh:4 {{The header <clopts.hh> is deprecated; include <base/Clopts.hh> instead.}}

using namespace command_line_options;

void callback(std::string_view) {}

using o1 = clopts<multiple<multiple<option<"--bar", "Bar">>>>; // expected-error@base/Clopts.hh:* {{multiple<multiple<>> is invalid}}
using o2 = clopts<multiple<stop_parsing<>>>; // expected-error@base/Clopts.hh:* {{multiple<stop_parsing<>> is invalid}}
using o3 = clopts<multiple<func<"foo", "bar", callback>>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be a callback}}
using o4 = clopts<multiple<help<>>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be a callback}}
using o5 = clopts<multiple<flag<"foo", "bar">>>; // expected-error@base/Clopts.hh:* {{Type of multiple<> cannot be bool}}
using o9 = clopts<multiple<overridable<"foo", "bar">>>; // expected-error@base/Clopts.hh:* {{multiple<> cannot be overridable}}

int a(int argc, char** argv) {
    using o6 = clopts<multiple<positional<"foo", "bar">>, multiple<positional<"baz", "bar">>>;
    (void) o6::parse(argc, argv); // expected-error@base/Clopts.hh:* {{Cannot have more than one multiple<positional<>> option}}

    using o7 = clopts<option<"foo", "bar">, flag<"foo", "baz">>;
    (void) o7::parse(argc, argv); // expected-error@base/Clopts.hh:* {{Two different options may not have the same name}}

    using o8 = clopts<>;
    (void) o8::parse(argc, argv); // expected-error@base/Clopts.hh:* {{At least one option is required}}
}


// expected-note@*           0+ {{}}
// expected-note@base/Clopts.hh:* 0+ {{}}
