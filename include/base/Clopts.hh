#ifndef LIBBASE_CLOPTS_HH
#define LIBBASE_CLOPTS_HH

#include <algorithm>
#include <array>
#include <base/FS.hh>
#include <base/Numeric.hh>
#include <base/Size.hh>
#include <base/Types.hh>
#include <base/Utils.hh>
#include <bitset>
#include <functional>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

// ===========================================================================
//  Forward Declarations.
// ===========================================================================
namespace base::cmd {
template <
    utils::static_string name,
    utils::static_string description,
    typename ...options
> struct subcommand;

template <typename>
struct parser;

/// A file.
template <typename contents_type_t = fs::FileContents, typename path_type_t = fs::Path>
struct file {
    using contents_type = contents_type_t;
    using path_type = path_type_t;
    static constexpr bool is_file_data = true;

    /// The file path.
    path_type path;

    /// The contents of the file.
    contents_type contents;
};

/// For backwards compatibility.
using file_data = file<>;

/// Properties common to all options.
struct opt_props {
    bool required = false;
    bool overridable = false;
    bool hidden = false;
};

/// Properties of flags.
struct flag_props {
    bool hidden = false;
    bool default_value = false;
};
}

namespace base::detail {
using namespace std::literals;
using namespace cmd;
using utils::static_string;
using utils::list;

template <typename... opts>
class clopts_impl;

// ===========================================================================
//  Metaprogramming Helpers.
// ===========================================================================
/// Concatenate two type lists.
template <typename, typename> struct concat_impl;
template <typename ...Ts, typename ...Us>
struct concat_impl<list<Ts...>, list<Us...>> {
    using type = list<Ts..., Us...>;
};

template <typename T, typename U>
using concat = typename concat_impl<T, U>::type;

/// Filter a pack of types.
template <template <typename> typename, typename...>
struct filter_impl;

template < // clang-format off
    template <typename> typename cond,
    typename ...processed,
    typename next,
    typename ...rest
> struct filter_impl<cond, list<processed...>, next, rest...> {
    using type = std::conditional_t<cond<next>::value,
        filter_impl<cond, list<processed..., next>, rest...>,
        filter_impl<cond, list<processed...>, rest...>
    >::type;
}; // clang-format on

template <template <typename> typename cond, typename... processed>
struct filter_impl<cond, list<processed...>> {
    using type = list<processed...>;
};

template <template <typename> typename cond, typename... types>
using filter = typename filter_impl<cond, list<>, types...>::type;

/// See that one talk (by Daisy Hollman, I think) about how this works.
template <template <typename> typename get_key, typename... types>
struct sort_impl {
private:
    static constexpr auto sorter = []<usz ...i>(std::index_sequence<i...>) {
        static constexpr auto sorted = [] {
            std::array indices{i...};
            std::array lookup_table{get_key<types>::value...};
            std::sort(indices.begin(), indices.end(), [&](usz a, usz b) {
                return lookup_table[a] < lookup_table[b];
            });
            return indices;
        }();
        return list<types...[sorted[i]]...>{};
    };

public:
    using type = decltype(sorter(std::index_sequence_for<types...>()));
};

template <template <typename> typename get_key, typename... types>
struct sort_impl<get_key, list<types...>> {
    using type = typename sort_impl<get_key, types...>::type;
};

// Special case because an array of size 0 is not going to work...
template <template <typename> typename get_key>
struct sort_impl<get_key, list<>> { using type = list<>; };

/// Sort a type list. The trick here is to sort the indices.
template <template <typename> typename get_key, typename type_list>
using sort = typename sort_impl<get_key, type_list>::type;

/// Iterate over a pack while a condition is true.
template <typename... pack>
constexpr void While(bool& cond, auto&& lambda) {
    auto impl = [&]<typename t>() -> bool {
        if (not cond) return false;
        lambda.template operator()<t>();
        return true;
    };

    (void) (impl.template operator()<pack>() and ...);
}

// ===========================================================================
//  Type Traits and Metaprogramming Types.
// ===========================================================================
/// Empty type.
struct empty {};

/// Check if an option is a positional option.
template <typename opt>
struct is_positional {
    static constexpr bool value = requires {{typename opt::is_positional_{}} -> std::same_as<std::true_type>; };
    using type = std::bool_constant<value>;
};

template <typename opt> using positional_t = typename is_positional<opt>::type;
template <typename opt> concept is_positional_v = is_positional<opt>::value;
template <typename opt> concept is_short_option = requires { opt::is_short; };
template <typename opt> concept is_multiple = requires { opt::is_multiple; };
template <typename opt> concept is_flag = requires { opt::is_flag; };
template <typename opt> concept is_subcommand = requires { opt::is_subcommand; };
template <typename ty> concept is_values = requires { ty::is_values; };

template <typename opt>
struct is_regular_option {
    static constexpr bool value = not is_positional<opt>::value and not is_subcommand<opt>;
    using type = std::bool_constant<value>;
};

template <typename opt>
struct is_subcommand_option {
    static constexpr bool value = is_subcommand<opt>;
    using type = std::bool_constant<value>;
};

/// Callback that takes an argument.
using callback_arg_type = void (*)(void*, std::string_view, std::string_view);

/// Callback that takes no arguments.
using callback_noarg_type = void (*)(void*, std::string_view);

/// Check whether a type is a callback.
template <typename T>
concept is_callback = utils::is<T, callback_arg_type, callback_noarg_type>;

/// Check if an option type takes an argument.
template <typename opt>
concept has_argument =
    not utils::is<typename opt::declared_type, callback_noarg_type> and
    not is_subcommand<opt>;

/// Check if this is the help<> option.
template <typename opt> concept is_help_option = requires { opt::is_help_option; };

/// Whether we should include the argument type of an option in the
/// help text. This is true for all options that take arguments, except
/// the builtin help option.
template <typename opt>
concept should_print_argument_type = has_argument<opt> and not is_help_option<opt>;

/// Helper for static asserts.
template <typename t>
concept always_false = false;

/// Not a concept because we can’t pass packs as the first parameter of a concept.
template <typename first, typename ...rest>
static constexpr bool assert_same_type = (std::is_same_v<first, rest> and ...);

/// Wrap an arbitrary function in a lambda.
template <auto cb> struct make_lambda_s;

template <auto cb>
requires std::is_invocable_v<decltype(cb)>
struct make_lambda_s<cb> {
    using lambda = decltype([](void*, std::string_view) { cb(); });
    using type = callback_noarg_type;
};

template <auto cb>
requires std::is_invocable_v<decltype(cb), void*>
struct make_lambda_s<cb> {
    using lambda = decltype([](void* data, std::string_view) { cb(data); });
    using type = callback_noarg_type;
};

template <auto cb>
requires std::is_invocable_v<decltype(cb), std::string_view>
struct make_lambda_s<cb> {
    using lambda = decltype([](void*, std::string_view, std::string_view arg) { cb(arg); });
    using type = callback_arg_type;
};

template <auto cb>
requires std::is_invocable_v<decltype(cb), void*, std::string_view>
struct make_lambda_s<cb> {
    using lambda = decltype([](void* data, std::string_view, std::string_view arg) { cb(data, arg); });
    using type = callback_arg_type;
};

template <auto cb>
requires std::is_invocable_v<decltype(cb), std::string_view, std::string_view>
struct make_lambda_s<cb> {
    using lambda = decltype([](void*, std::string_view name, std::string_view arg) { cb(name, arg); });
    using type = callback_arg_type;
};

template <auto cb>
requires std::is_invocable_v<decltype(cb), void*, std::string_view, std::string_view>
struct make_lambda_s<cb> {
    using lambda = decltype([](void* data, std::string_view name, std::string_view arg) { cb(data, name, arg); });
    using type = callback_arg_type;
};

template <auto cb>
using make_lambda = make_lambda_s<cb>; // clang-format on

template <typename first, typename...>
struct first_type {
    using type = first;
};

/// Get the first element of a pack.
template <typename... rest>
using first_type_t = typename first_type<rest...>::type;

/// Execute code at end of scope.
template <typename lambda>
struct at_scope_exit {
    lambda l;
    ~at_scope_exit() { l(); }
};

/// Tag used for options that modify the options (parser) but
/// do not constitute actual options in an of themselves.
struct special_tag;

/// 'std::integral', but without 'bool' and the character types.
template <typename T> concept is_integer = utils::is_same<
    T,
    char,
    signed char,
    unsigned char,
    short,
    unsigned short,
    int,
    unsigned,
    long,
    unsigned long,
    long long,
    unsigned long long
#ifdef LIBBASE_I128_AVAILABLE
    , i128
    , u128
#endif
>;

#ifdef LIBBASE_I128_AVAILABLE
using MaxInt = i128;
#else
using MaxInt = i64;
#endif

/// Constexpr to_string for integers. Returns the number of bytes written.
template <is_integer T>
constexpr std::string constexpr_to_string(T i) {
    if (i == 0) return "0"; // Special handling for 0.

    std::string out;
    bool negative = false;
    if constexpr (std::signed_integral<T>) {
        if (i < 0) {
            out += '-';
            negative = true;
            i = -i;
        }
    }

    while (i) {
        out += char('0' + char(i % 10));
        i /= 10;
    }

    std::reverse(out.data() + negative, out.data() + out.size());
    return out;
}

template <usz size>
struct string_or_int {
    static_string<size> s{};
    i64 integer{};
    bool is_integer{};

    constexpr string_or_int(const char (&data)[size]) {
        std::copy_n(data, size, s.arr);
        s.len = size - 1;
        is_integer = false;
    }

    constexpr string_or_int(i64 integer)
        : integer{integer}
    , is_integer{true} {}
};

string_or_int(i64) -> string_or_int<1>;

// ===========================================================================
//  Types.
// ===========================================================================
/// Struct for storing allowed option values.
template <typename _type, auto... data>
struct values_impl {
    using type = _type;
    constexpr values_impl() = delete;
    static constexpr bool is_values = true;

    static constexpr bool is_valid_option_value(const type& val) {
        auto test = [val]<auto value>() -> bool {
            if constexpr (value.is_integer) return value.integer == val;
            else return value.s == val;
        };

        return (test.template operator()<data>() or ...);
    }

    static constexpr void print_values(std::string& out) {
        bool first = true;
        auto append = [&]<auto value> {
            if (first) first = false;
            else out += ", ";
            if constexpr (value.is_integer) out += constexpr_to_string(value.integer);
            else out += value.s.sv();
        };
        (append.template operator()<data>(), ...);
    }
};

template <string_or_int... data>
concept values_must_be_all_strings_or_all_ints = (data.is_integer and ...) or (not data.is_integer and ...);

/// Values type.
template <string_or_int... data>
requires values_must_be_all_strings_or_all_ints<data...>
struct values : values_impl<std::conditional_t<(data.is_integer and ...), i64, std::string>, data...> {};

// ===========================================================================
//  Option Implementation.
// ===========================================================================
template <
    static_string _name,
    static_string _description,
    typename ty_param,
    opt_props props>
struct opt_impl {
    /// The actual type that was passed in.
    using declared_type = ty_param;

    /// Type used to parse this.
    using parser = parser<declared_type>;

    /// Make sure this is a valid option.
    static_assert(sizeof _description.arr < 512, "Description may not be longer than 512 characters");
    static_assert(_name.len > 0, "Option name may not be empty");
    static_assert(sizeof _name.arr < 256, "Option name may not be longer than 256 characters");
    static constexpr decltype(_name) name = _name;
    static constexpr decltype(_description) description = _description;
    static constexpr bool is_values = detail::is_values<declared_type>;
    static constexpr auto properties = props;
    static constexpr bool is_required = props.required;
    static constexpr bool is_overridable = props.overridable;
    static constexpr bool is_hidden = props.hidden;
    static_assert(not is_required or not is_hidden, "Required options cannot be hidden");

    constexpr opt_impl() = delete;
};

/// A directive modifies the parsing process, but doesn’t correspond to
/// any option on the command-line.
struct directive {
    // This is a hack to prevent follow-up errors if 'regular_option' is
    // instantiated with a directive as an argument. There is a constraint
    // that is supposed to prevent that, but it seems that Clang just ignores
    // that specialisation if instantiating the constraint fails (e.g. due
    // to a static_assert in the directive)...
    using declared_type = void;
};

// ===========================================================================
//  Parser Helpers.
// ===========================================================================
/// Default help handler.
[[noreturn]] inline void default_help_handler(std::string_view program_name, std::string_view msg) {
    std::print(stderr, "Usage: {} {}", program_name, msg);
    std::exit(1);
}

// ===========================================================================
//  Sort/filter helpers.
// ===========================================================================
template <typename opt>
struct get_option_name_for_help_msg_sort {
    static constexpr str value = [] {
        static constexpr static_string copy = [] {
            static_string buf = opt::name;
            buf.assign(str(opt::name).drop_while('-'));
            rgs::transform(buf, buf.begin(), [](char c) {
                if (text::IsUpper(c)) return char(c + ('a' - 'A'));
                return c;
            });
            return buf;
        }();
        return str(copy);
    }();
};

template <typename>
struct find_help_option {
    using type = std::false_type;
};

template <typename opt, typename ...opts>
struct find_help_option<list<opt, opts...>> {
    using type = std::conditional_t<
        is_help_option<opt>,
        std::type_identity<opt>,
        find_help_option<list<opts...>>
    >::type;
};

template <typename help, typename opt>
struct apply_to_subcommand;

template <
    typename help,
    static_string name,
    static_string description,
    typename ...options
>
struct apply_to_subcommand<help, subcommand<name, description, options...>> {
    using type = subcommand<name, description, options..., help>;
};

template <
    static_string name,
    static_string description,
    typename ...options
>
struct apply_to_subcommand<std::false_type, subcommand<name, description, options...>> {
    using type = subcommand<name, description, options...>;
};

template <typename help, typename opt>
struct preprocess_subcommand {
    using type = std::conditional_t<
        is_subcommand<opt>,
        apply_to_subcommand<help, opt>,
        std::type_identity<opt>
    >::type;
};

/// Helper to propagate the parent help<> option as well as the
/// subcommand path to any subcommands.
template <typename options>
struct preprocess_subcommands;

template <typename ...opts>
struct preprocess_subcommands<list<opts...>> {
    using help = find_help_option<list<opts...>>::type;
    using type = list<typename preprocess_subcommand<help, opts>::type...>;
};

template <typename opt>
struct is_values_option {
    static constexpr bool value = opt::is_values;
};

/// Check if an option is a directive.
template <typename opt>
struct is_directive {
    static constexpr bool value = std::derived_from<opt, directive>;
};

/// Check if an option is a regular option.
template <typename opt>
struct regular_option {
    static constexpr bool value = not utils::is<typename opt::declared_type, special_tag>;
};

template <typename opt>
requires std::derived_from<opt, directive>
struct regular_option<opt> {
    static constexpr bool value = false;
};

/// Check if an option is a special option.
template <typename opt>
struct special_option {
    static constexpr bool value = not regular_option<opt>::value;
};

template <typename opt>
requires std::derived_from<opt, directive>
struct special_option<opt> {
    static constexpr bool value = false;
};
}
// ===========================================================================
//  Parsers
// ===========================================================================
namespace base::cmd {
/// Parser for strings.
template <>
struct parser<std::string> {
    using storage_type = std::string;
    static auto parse(std::string_view arg) -> Result<std::string> {
        return std::string{arg};
    }

    static constexpr auto type_name() -> str { return "string"; }
};

/// Parser for 'bool'.
template <>
struct parser<bool> {
    using storage_type = bool;
    static auto parse(std::string_view arg) -> Result<bool> {
        return Parse<bool>(arg);
    }

    static constexpr auto type_name() -> str { return "bool"; }
};

/// Parser for 'values<>'.
template <auto... vs>
struct parser<detail::values<vs...>> {
    using vals = detail::values<vs...>;
    using element_parser = parser<typename vals::type>;
    using storage_type = element_parser::storage_type;

    static auto parse(std::string_view arg) -> Result<storage_type> {
        auto val = Try(element_parser::parse(arg));
        if (not vals::is_valid_option_value(val)) return Error("Invalid value");
        return val;
    }

    static constexpr auto type_name() { return element_parser::type_name(); }
};

/// Parser for integer types.
template <detail::is_integer Int>
struct parser<Int> {
    using storage_type = Int;
    static auto parse(std::string_view arg) -> Result<Int> {
        return Parse<Int>(arg);
    }

    static constexpr auto type_name() -> std::string {
        std::string buf = std::signed_integral<Int> ? "i" : "u";
        buf += detail::constexpr_to_string(Size::Of<Int>().bits());
        return buf;
    }
};

/// Parser for floating-point types.
template <std::floating_point Float>
struct parser<Float> {
    using storage_type = Float;
    static auto parse(std::string_view arg) -> Result<Float> {
        return Parse<Float>(arg);
    }

    static constexpr auto type_name() -> std::string {
        std::string buf = "f";
        buf += detail::constexpr_to_string(Size::Of<Float>().bits());
        return buf;
    }
};

/// Parser for files.
template <typename ContentsType, typename PathType>
struct parser<file<ContentsType, PathType>> {
    using storage_type = file<ContentsType, PathType>;
    static auto parse(std::string_view arg) -> Result<storage_type> {
        storage_type dat;
        dat.path = PathType{arg.begin(), arg.end()};
        Try(File::ReadInto(arg, dat.contents));
        return dat;
    }

    static constexpr auto type_name() -> str { return "file"; }
};
}
// ===========================================================================
//  Main Implementation.
// ===========================================================================
template <typename... opts, typename... special, typename... directives>
class base::detail::clopts_impl<
    base::utils::list<opts...>,
    base::utils::list<special...>,
    base::utils::list<directives...>
> {
    LIBBASE_IMMOVABLE(clopts_impl);

    template <typename...>
    friend class clopts_impl;

    // This should never be instantiated by or returned to the user.
    explicit clopts_impl() = default;
    ~clopts_impl() = default;

    // =======================================================================
    //  Option Access by Name.
    // =======================================================================
    /// Get the index of an option.
    template <usz index, static_string option>
    static constexpr usz optindex_impl() {
        if constexpr (index >= sizeof...(opts)) return index;
        else if constexpr (opts...[index] ::name.sv() == option) return index;
        else return optindex_impl<index + 1, option>();
    }

    template <static_string option>
    static consteval auto format_invalid_option_name() -> static_string<option.size() + 45> {
        static_string<option.size() + 45> ret;
        ret.append("There is no option with the name '"sv);
        ret.append(option.sv());
        ret.append("'"sv);
        return ret;
    }

    template <bool ok, static_string option>
    static consteval void assert_valid_option_name() {
        static_assert(ok, format_invalid_option_name<option>());
    }

    /// Get the index of an option and raise an error if the option is not found.
    template <static_string option>
    static constexpr usz optindex() {
        constexpr usz sz = optindex_impl<0, option>();
        assert_valid_option_name<(sz < sizeof...(opts)), option>();
        return sz;
    }

    /// Get an option by name.
    template <static_string name>
    using opt_by_name = opts...[optindex<name>()];

    // =======================================================================
    //  Validation.
    // =======================================================================
    static_assert(sizeof...(opts) > 0, "At least one option is required");

    /// Make sure no two options have the same name.
    static consteval bool check_duplicate_options() {
        // State is ok initially.
        bool ok = true;
        usz i = 0;

        // Iterate over each option for each option.
        While<opts...>(ok, [&]<typename opt>() {
            usz j = 0;
            While<opts...>(ok, [&]<typename opt2>() {
                // If the options are not the same, but their names are the same
                // then this is an error. Iteration will stop at this point because
                // \c ok is also the condition for the two loops.
                ok = i == j or opt::name != opt2::name;
                j++;
            });
            i++;
        });

        // Return whether everything is ok.
        return ok;
    }

    // This check is currently broken on MSVC 19.38 and later, for some reason.
#if !defined(_MSC_VER) || defined(__clang__) || _MSC_VER < 1938
    /// Make sure that no option has a prefix that is a short option.
    static consteval bool check_short_opts() {
        // State is ok initially.
        bool ok = true;
        usz i = 0;

        // Iterate over each option for each option.
        While<opts...>(ok, [&]<typename opt>() {
            usz j = 0;
            While<opts...>(ok, [&]<typename opt2>() {
                // Check the condition.
                ok = i == j or not is_short_option<opt> or not opt2::name.sv().starts_with(opt::name.sv());
                j++;
            });
            i++;
        });

        // Return whether everything is ok.
        return ok;
    }

    static_assert(check_short_opts(), "Option name may not start with the name of a short option");
#endif

    /// Make sure there is at most one multiple<positional<>> option.
    static consteval usz validate_multiple() {
        auto is_mul = []<typename opt>() { return is_multiple<opt>; };
        return (... + (is_mul.template operator()<opts>() and detail::is_positional_v<opts>) );
    }

    /// Validate that mutually_exclusive options exist.
    static consteval bool check_mutually_exclusive_exist() {
        bool ok = true;
        While<directives...>(ok, [&]<typename dir> {
            if constexpr (requires { dir::is_mutually_exclusive; }) {
                for (auto name : dir::options) {
                    if (not((opts::name == name) or ...)) {
                        ok = false;
                        return;
                    }
                }
            }
        });
        return ok;
    }

    /// Validate that we don’t require two required options to be mutually
    /// exclusive since that can’t possibly work.
    static consteval bool check_mutually_exclusive_required() {
        bool ok = true;
        list<directives...>::each([&]<typename dir> {
            if constexpr (requires { dir::is_mutually_exclusive; }) {
                [[maybe_unused]] bool found = false;
                list<opts...>::each([&]<typename opt> {
                    if (opt::is_required and rgs::contains(dir::options, str(opt::name))) {
                        if (not found) found = true;
                        else ok = false;
                    }
                });
            }
        });
        return ok;
    }

    /// Make sure we don’t have invalid option combinations.
    ///
    /// TODO: Can we use a 'consteval {}' block for these (once compiler support those) and print
    /// the option that causes the problem in a static assertion inside these validation functions?
    static_assert(check_duplicate_options(), "Two different options may not have the same name");
    static_assert(validate_multiple() <= 1, "Cannot have more than one multiple<positional<>> option");
    static_assert(check_mutually_exclusive_exist(), "mutually_exclusive<> must reference existing options");
    static_assert(check_mutually_exclusive_required(), "Cannot mark two required options as mutually_exclusive<>");
    static_assert(
        ((is_flag<opts> or not std::same_as<typename opts::declared_type, bool>) and ...),
        "Use flag<> for options with value type 'bool'"
    );

    // =======================================================================
    //  Option Storage.
    // =======================================================================
    template <typename opt>
    struct storage_type {
        using type = opt::parser::storage_type;
    };

    template <is_multiple opt>
    struct storage_type<opt> {
        // We can just delegate to the parser since all the special cases
        // (e.g. multiple<>, subcommands, ...) aren’t valid as an argument
        // to 'multiple<>'.
        using type = std::vector<typename opt::parser::storage_type>;
    };

    template <is_subcommand opt>
    struct storage_type<opt> {
        using type = opt::declared_type::optvals_type;
    };

    template <typename opt>
    requires is_callback<typename opt::declared_type>
    struct storage_type<opt> {
        using type = empty;
    };

    template <typename opt>
    using storage_type_t = storage_type<opt>::type;

    template <typename opt>
    struct get_return_type {
        using type = storage_type_t<opt>*;
    };

    template <is_multiple opt>
    struct get_return_type<opt> {
        using type = MutableSpan<typename opt::parser::storage_type>;
    };

    template <is_flag opt>
    struct get_return_type<opt> {
        using type = bool;
    };

    template <typename opt>
    using get_return_type_t = get_return_type<opt>::type;

    /// Various types.
    /// FIXME: Compute this size properly.
    using help_string_t = static_string<1'024 + 1'024 * sizeof...(opts)>; // Size should be ‘big enough’™.
    using optvals_tuple_t = std::tuple<storage_type_t<opts>...>;

    static constexpr bool has_stop_parsing = (requires { special::is_stop_parsing; } or ...);

public:
#ifdef __cpp_lib_copyable_function
    using error_handler_t = std::copyable_function<bool(std::string&&)>;
#else
    using error_handler_t = std::function<bool(std::string&&)>;
#endif

    // =======================================================================
    //  Option Access.
    // =======================================================================
    /// Result type.
    class optvals_type {
        friend clopts_impl;
        optvals_tuple_t optvals{};
        std::bitset<sizeof...(opts)> opts_found{};
        [[no_unique_address]] std::conditional_t<has_stop_parsing, Span<const char*>, empty> unprocessed_args{};

        // This implements get<>() and get_or<>().
        template <usz opt_idx>
        constexpr auto get_impl() -> get_return_type_t<opts...[opt_idx]> {
            using opt = opts...[opt_idx];
            using declared = opt::declared_type;

            // Bool options don’t have a value. Instead, we just return whether the option was found.
            if constexpr (std::is_same_v<declared, bool>) return opts_found[opt_idx];

            // We always return a span to multiple<> options because the user can just check if it’s empty.
            else if constexpr (is_multiple<opt>) return std::get<opt_idx>(optvals);

            // Otherwise, return nullptr if the option wasn’t found, and a pointer to the value otherwise.
            else return not opts_found[opt_idx] ? nullptr : std::addressof(std::get<opt_idx>(optvals));
        }

    public:
        constexpr optvals_type() {
            // Initialise flags to their default values.
            list<opts...>::each([&]<typename opt>{
                if constexpr (is_flag<opt>) {
                    if constexpr (opt::default_value) {
                        opts_found[optindex<opt::name>()] = true;
                    }
                }
            });
        }

        /// \brief Get the value of an option.
        ///
        /// This is not \c [[nodiscard]] because that raises an ICE when compiling
        /// with some older versions of GCC.
        ///
        /// \return \c true / \c false if the option is a flag
        /// \return \c nullptr if the option was not found
        /// \return a pointer to the value if the option was found
        ///
        /// \see get_or()
        template <static_string s>
        constexpr auto get() {
            // Check if the option exists before calling get_impl<>() so we trigger the static_assert
            // below before hitting a complex template instantiation error.
            constexpr auto sz = optindex_impl<0, s>();
            if constexpr (sz < sizeof...(opts)) return get_impl<sz>();
            else assert_valid_option_name<(sz < sizeof...(opts)), s>();
        }

        /// \brief Get the value of an option or a default value if the option was not found.
        ///
        /// The default value is \c static_cast to the type of the option value.
        ///
        /// \param default_ The default value to return if the option was not found.
        /// \return \c default_ if the option was not found.
        /// \return a copy of the option value if the option was found.
        ///
        /// \see get()
        template <static_string s>
        constexpr auto get(auto&& default_) {
            constexpr auto idx = optindex_impl<0, s>();
            if constexpr (idx < sizeof...(opts)) {
                if (opts_found[idx]) return *get_impl<idx>();
                return static_cast<std::remove_cvref_t<decltype(*get_impl<idx>())>>(std::forward<decltype(default_)>(default_));
            } else {
                assert_valid_option_name<(idx < sizeof...(opts)), s>();
            }
        }

        template <static_string s>
        [[deprecated("Use get(value) instead of get_or(value)")]]
        constexpr decltype(auto) get_or(auto&& default_) {
            return get<s>(std::forward<decltype(default_)>(default_));
        }

        /// \brief Get unprocessed options.
        ///
        /// If the \c stop_parsing\<> option was encountered, this will return the
        /// remaining options that have not been processed by this parser. If there
        /// is no \c stop_parsing\<> option, this will always return an empty span.
        ///
        /// If there was an error during parsing, the return value of this function
        /// is unspecified.
        [[nodiscard]] auto unprocessed() const -> Span<const char*> {
            if constexpr (has_stop_parsing) return unprocessed_args;
            else return {};
        }
    };

private:
    // =======================================================================
    //  Parser State.
    // =======================================================================
    /// Variables for the parser and for storing parsed options.
    optvals_type optvals{};
    bool has_error = false;
    int argc{};
    int argi{};
    const char** argv{};
    void* user_data{};
    std::string_view program_name;
    error_handler_t error_handler{};

    // =======================================================================
    //  Helpers.
    // =======================================================================
    /// Error handler that is used if the user doesn’t provide one.
    bool default_error_handler(std::string&& errmsg) {
        if (not program_name.empty()) std::print(stderr, "{}: ", program_name);
        std::println(stderr, "{}", errmsg);

        // Invoke the help option.
        bool invoked = false;
        auto invoke = [&]<typename opt> {
            if constexpr (is_help_option<opt>) {
                invoked = true;
                invoke_help_callback<opt>();
            }
        };

        // If there is a help option, invoke it.
        (invoke.template operator()<opts>(), ...);

        // If no help option was found, print the help message.
        if (not invoked) {
            std::print(stderr, "Usage: ");
            if (not program_name.empty()) std::print(stderr, "{} ", program_name);
            std::print(stderr, "{}", help());
        }

        std::exit(1);
    }

    /// Invoke the error handler and set the error flag.
    template <typename... Args>
    void handle_error(std::format_string<Args...> fmt, Args&&... args) {
        has_error = not error_handler(std::format(fmt, std::forward<Args>(args)...));
    }

    /// Invoke the help callback of the help option.
    template <typename opt>
    void invoke_help_callback() {
        // New API: program name + help message [+ user data].
        using sv = std::string_view;
        if constexpr (requires { opt::help_callback(sv{}, sv{}, user_data); })
            opt::help_callback(sv{program_name}, sv{}, user_data);
        else if constexpr (requires { opt::help_callback(sv{}, sv{}); })
            opt::help_callback(sv{program_name}, help_message_raw.sv());

        // Compatibility for callbacks that don’t take the program name.
        else if constexpr (requires { opt::help_callback(sv{}, user_data); })
            opt::help_callback(help_message_raw.sv(), user_data);
        else if constexpr (requires { opt::help_callback(sv{}); })
            opt::help_callback(help_message_raw.sv());

        // Invalid help option callback.
        else static_assert(
            detail::always_false<opt>,
            "Invalid help option signature. Consult the README for more information"
        );
    }

    // =======================================================================
    //  Internal Option Access.
    // =======================================================================
    /// Check if an option was found.
    template <static_string option>
    bool found() { return optvals.opts_found[optindex<option>()]; }

    /// Get a reference to an option value.
    template <static_string s>
    [[nodiscard]] constexpr auto ref_to_storage() -> decltype(std::get<optindex<s>()>(optvals.optvals))& {
        return std::get<optindex<s>()>(optvals.optvals);
    }

    /// Mark an option as found.
    template <typename opt>
    void set_found(bool value = true) {
        // Check if this option accepts multiple values.
        if constexpr (
            not is_multiple<opt> and
            not is_flag<opt> and
            not detail::is_callback<typename opt::declared_type> and
            not opt::is_overridable
        ) {
            if (found<opt::name>()) handle_error("Duplicate option: '{}'", opt::name);
        }

        optvals.opts_found[optindex<opt::name>()] = value;
    }

    /// Store an option value.
    template <typename opt>
    void store_option_value(auto value) {
        if constexpr (is_flag<opt>) set_found<opt>(value);
        else if constexpr (is_multiple<opt>) ref_to_storage<opt::name>().push_back(std::move(value));
        else ref_to_storage<opt::name>() = std::move(value);
    }

    // =======================================================================
    //  Help Message.
    // =======================================================================
    /// Create the help message.
    static constexpr auto make_help_message() -> help_string_t { // clang-format off
        using positional_unsorted = filter<is_positional, opts...>;
        using positional = sort<get_option_name_for_help_msg_sort, positional_unsorted>;
        using regular = sort<get_option_name_for_help_msg_sort, filter<is_regular_option, opts...>>;
        using subcommands = sort<get_option_name_for_help_msg_sort, filter<is_subcommand_option, opts...>>;
        using values_opts = sort<get_option_name_for_help_msg_sort, filter<is_values_option, opts...>>;
        std::string msg{};

        auto ShowInHelp = [&]<typename opt> { return not opt::is_hidden; };
        auto TypeName = [&]<typename opt> {
            if constexpr (std::same_as<typename opt::declared_type, callback_arg_type>) return str("arg");
            else return opt::parser::type_name();
        };

        // Append the positional options.
        //
        // Do NOT sort them here as this is where we print in what order
        // they’re supposed to appear in, so sorting would be very stupid
        // here.
        positional_unsorted::each([&]<typename opt> {
            if constexpr (opt::is_hidden) return;
            if (not opt::is_required) msg += "[";
            msg += "<";
            msg += str(opt::name.arr, opt::name.len);
            msg += ">";
            if (not opt::is_required) msg += "]";
            msg += " ";
        });

        // End of first line.
        msg += "[options]\n";

        // Determine the length of the longest name + typename so that
        // we know how much padding to insert before actually printing
        // the description. Also factor in the <> signs around and the
        // space after the option name, as well as the type name.
        usz max_vals_opt_name_len{};
        usz max_len{};
        list<opts...>::each([&]<typename opt> {
            if constexpr (opt::is_hidden) return;
            if constexpr (opt::is_values)
                max_vals_opt_name_len = std::max(max_vals_opt_name_len, opt::name.len);

            // If we’re printing the type, we have the following formats:
            //
            //     name=<type>    Description (option)
            //     name[=<bool>]  Description (flag)
            //     name <type>    Description (short option)
            //     <name> : type  Description (positional option)
            //
            // Apart from the type name, we also need to account for the extra
            // formatting characters.
            if constexpr (should_print_argument_type<opt>) {
                max_len = std::max(
                    max_len,
                    opt::name.len + TypeName.template operator()<opt>().size() + (is_positional_v<opt> or is_flag<opt> ? 5 : 3)
                );
            }

            // Otherwise, we only care about the name of the option and the
            // extra '<>' of positional options.
            else {
                if constexpr (is_positional_v<opt>) max_len = std::max(max_len, opt::name.len + 2);
                else max_len = std::max(max_len, opt::name.len);
            }
        });

        // Append an argument.
        auto Append = [&]<typename opt> {
            if constexpr (opt::is_hidden) return;
            msg += "    ";
            auto old_len = msg.size();

            // Append name.
            if constexpr (is_positional_v<opt>) msg += "<";
            msg += str(opt::name.arr, opt::name.len);
            if constexpr (is_positional_v<opt>) msg += ">";

            // Append type.
            if constexpr (should_print_argument_type<opt>) {
                if constexpr (is_positional_v<opt>) {
                    msg += " : ";
                    msg += TypeName.template operator()<opt>();
                } else if constexpr (is_flag<opt>) {
                    msg += "[=<bool>]";
                } else {
                    msg += is_short_option<opt> ? " <" : "=<";
                    msg += TypeName.template operator()<opt>();
                    msg += ">";
                }
            }

            // Align to right margin.
            auto len = msg.size() - old_len;
            for (usz i = 0; i < max_len - len; i++) msg += " ";

            // Two extra spaces between this and the description.
            msg += "  ";
            msg += str(opt::description.arr, opt::description.len);
            msg += "\n";
        };

        // Append the descriptions of positional options.
        if (positional_unsorted::any(ShowInHelp)) {
            msg += "\nArguments:\n";
            positional::each(Append);
        }

        // Append subcommands.
        if (subcommands::any(ShowInHelp)) {
            msg += "\nSubcommands:\n";
            subcommands::each(Append);
        }

        // Append non-positional options.
        if (regular::any(ShowInHelp)) {
            msg += "\nOptions:\n";
            regular::each(Append);
        }

        // If we have any values<> types, print their supported values.
        if constexpr (((opts::is_values and not opts::is_hidden) or ...)) {
            msg += "\nSupported option values:\n";
            values_opts::each([&] <typename opt> {
                if constexpr (opt::is_hidden) return;
                if constexpr (opt::is_values) {
                    msg += "    ";
                    msg += str(opt::name.arr, opt::name.len);
                    msg += ":";

                    // Padding after the name.
                    for (usz i = 0; i < max_vals_opt_name_len - opt::name.len + 1; i++)
                        msg += " ";

                    // Option values.
                    opt::declared_type::print_values(msg);
                    msg += "\n";
                }
            });
        }

        // Return the combined help message.
        help_string_t s;
        s.append(msg);
        return s;
    } // clang-format on

    /// Help message is created at compile time.
    static constexpr help_string_t help_message_raw = make_help_message();

public:
    /// Get the help message.
    static consteval auto help() -> std::string_view {
        return help_message_raw.sv();
    }

private:
    // =======================================================================
    //  Parsing and Dispatch.
    // =======================================================================
    /// Handle an option value.
    template <typename opt>
    void dispatch_option_with_arg(std::string_view opt_val) {
        // Callbacks can’t use multiple<>, so checking the declared type is fine.
        using declared = opt::declared_type;

        // Mark the option as found.
        set_found<opt>();

        // If this is a function option, simply call the callback and we're done.
        if constexpr (detail::is_callback<declared>) {
            if constexpr (utils::is<declared, callback_noarg_type>) opt::callback(user_data, opt::name.sv());
            else opt::callback(user_data, opt::name.sv(), opt_val);
        }

        // Otherwise, parse the argument.
        else {
            auto res = opt::parser::parse(opt_val);
            if (not res) return handle_error(
                "Error parsing argument '{}' of option '{}': {}",
                opt_val,
                opt::name,
                res.error()
            );

            store_option_value<opt>(std::move(res.value()));
        }
    }

    /// Handle an option that may take an argument.
    ///
    /// Both --option value and --option=value are valid ways of supplying a
    /// value. We test for both of them.
    template <typename opt>
    bool handle_non_positional_with_arg(std::string_view opt_str) {
        if (not opt_str.starts_with(opt::name.sv())) return false;

        // --option=value or short opt.
        if (opt_str.size() > opt::name.size()) {
            // Parse the rest of the option as the value if we have a '=' or if this is a short option.
            if (opt_str[opt::name.size()] == '=' or is_short_option<opt>) {
                // Otherwise, parse the value.
                auto opt_start_offs = opt::name.size() + (opt_str[opt::name.size()] == '=');
                auto opt_val = opt_str.substr(opt_start_offs);
                dispatch_option_with_arg<opt>(opt_val);
                return true;
            }

            // Otherwise, this isn’t the right option.
            return false;
        }

        // Handle the option. If we get here, we know that the option name that we’ve
        // encountered matches the option name exactly. If this is a func option that
        // doesn’t take arguments, just call the callback and we’re done.
        //
        // Callbacks can’t use multiple<>, so checking the declared type is fine.
        if constexpr (utils::is<typename opt::declared_type, callback_noarg_type>) {
            opt::callback(user_data, opt_str);
            return true;
        }

        // Otherwise, try to consume the next argument as the option value.
        else {
            // No more command line arguments left.
            if (++argi == argc) {
                handle_error("Missing argument for option '{}'", opt_str);
                return false;
            }

            // Parse the argument.
            dispatch_option_with_arg<opt>(argv[argi]);
            return true;
        }
    }

    template <typename opt>
    bool handle_flag(str opt_str) {
        // Flag w/o argument.
        if (opt_str == opt::name.sv()) {
            set_found<opt>();
            return true;
        }

        // Make sure we have '--name=', and not just some other option that
        // happens to share a prefix with this flag.
        if (not opt_str.consume(opt::name) or not opt_str.consume('=')) return false;

        // Ok, the user passed an argument to this flag.
        dispatch_option_with_arg<opt>(opt_str);
        return true;
    }

    template <typename opt>
    bool handle_non_positional(std::string_view opt_str) {
        // Check if the name of this flag matches the entire option string that
        // we encountered. If we’re just a prefix, then we don’t handle this.
        if (opt_str != opt::name.sv()) return false;

        // Mark the option as found.
        set_found<opt>();

        // If it’s a callable, call it.
        if constexpr (detail::is_callback<typename opt::declared_type>) {
            // The builtin help option is handled here. We pass the help message as an argument.
            if constexpr (is_help_option<opt>) invoke_help_callback<opt>();

            // If it’s not the help option, just invoke it.
            else { opt::callback(user_data, opt_str); }
        }

        // If it’s a subcommand, dispatch all remaining arguments to its parser.
        if constexpr (detail::is_subcommand<opt>) {
            auto parsed = opt::declared_type::parse_impl(
                program_name,
                argc - argi - 1,
                argv + argi + 1,
                error_handler,
                user_data
            );

            argi = argc;
            store_option_value<opt>(std::move(parsed));
        }

        // Option has been handled.
        return true;
    }

    /// Handle a positional option.
    template <typename opt>
    bool handle_positional_impl(std::string_view opt_str) {
        static_assert(not detail::is_callback<typename opt::declared_type>, "positional<>s may not have a callback");

        // If we've already encountered this positional option, then return.
        if constexpr (not is_multiple<opt>) {
            if (found<opt::name>()) return false;
        }

        // Otherwise, attempt to parse this as the option value.
        dispatch_option_with_arg<opt>(opt_str);
        return true;
    }

    /// Invoke handle_regular_impl on every option until one returns true.
    bool handle_non_positional(std::string_view opt_str) {
        const auto handle = [this]<typename opt>(std::string_view str) {
            // `this->` is to silence a warning.
            if constexpr (is_positional_v<opt>) return false;
            else if constexpr (is_flag<opt>) return this->handle_flag<opt>(str);
            else if constexpr (not has_argument<opt>) return this->handle_non_positional<opt>(str);
            else return this->handle_non_positional_with_arg<opt>(str);
        };

        return (handle.template operator()<opts>(opt_str) or ...);
    }

    /// Invoke handle_positional_impl on every option until one returns true.
    bool handle_positional(std::string_view opt_str) {
        const auto handle = [this]<typename opt>(std::string_view str) {
            // `this->` is to silence a warning.
            if constexpr (is_positional_v<opt>) return this->handle_positional_impl<opt>(str);
            else return false;
        };

        return (handle.template operator()<opts>(opt_str) or ...);
    }

    /// Check if we should stop parsing.
    template <typename opt>
    bool stop_parsing(std::string_view opt_str) {
        if constexpr (requires { opt::is_stop_parsing; }) return opt_str == opt::name.sv();
        return false;
    }

    void parse() {
        // Main parser loop.
        for (argi = 0; argi < argc; argi++) {
            std::string_view opt_str{argv[argi]};

            // Stop parsing if this is the stop_parsing<> option.
            if ((stop_parsing<special>(opt_str) or ...)) {
                argi++;
                break;
            }

            // Attempt to handle the option.
            if (not handle_non_positional(opt_str) and not handle_positional(opt_str))
                handle_error("Unrecognized option: '{}'", opt_str);

            // Stop parsing if there was an error.
            if (has_error) return;
        }

        // Make sure all required options were found.
        list<opts...>::each([&]<typename opt> {
            if constexpr (opt::is_required) {
                if (not found<opt::name>()) {
                    handle_error("Option '{}' is required", opt::name);
                }
            }
        });

        // Make sure that mutually_exclusive is respected.
        list<directives...>::each([&]<typename dir> {
            if constexpr (requires { dir::is_mutually_exclusive; }) {
                std::optional<str> prev;
                list<opts...>::each([&]<typename opt> {
                    if (found<opt::name>() and rgs::contains(dir::options, str(opt::name))) {
                        if (not prev) prev = opt::name;
                        else handle_error("Options '{}' and '{}' are mutually exclusive", *prev, opt::name);
                    }
                });
            }
        });

        // Save unprocessed options.
        if constexpr (has_stop_parsing) {
            optvals.unprocessed_args = Span<const char*>{
                argv + argi,
                static_cast<usz>(argc - argi),
            };
        }
    }

    static auto parse_impl(
        std::string_view program_name,
        int argc,
        const char** argv,
        error_handler_t error_handler,
        void* user_data
    ) -> optvals_type {
        clopts_impl self;
        if (error_handler) self.error_handler = std::move(error_handler);
        else self.error_handler = [&](auto&& e) { return self.default_error_handler(std::forward<decltype(e)>(e)); };
        self.argc = argc;
        self.argv = argv;
        self.program_name = program_name;
        self.user_data = user_data;
        self.parse();
        return std::move(self.optvals);
    }

public:
    /// Access a subcommand.
    template <static_string s>
    requires is_subcommand<opt_by_name<s>>
    using command = opt_by_name<s>::declared_type;

    /// \brief Parse command line options.
    ///
    /// \param argc The argument count.
    /// \param argv The arguments (including the program name).
    /// \param user_data User data passed to any func\<\> options that accept a \c void*.
    /// \param error_handler A callback that is invoked whenever an error occurs. If
    ///        \c nullptr is passed, the default error handler is used. The error handler
    ///        should return \c true if parsing should continue and \c false otherwise.
    /// \return The parsed option values.
    static auto parse(
        int argc,
        const char* const* argv,
        error_handler_t error_handler = nullptr,
        void* user_data = nullptr
    ) -> optvals_type {
        std::string_view program_name;
        if (argv and argc) {
            program_name = argv[0];
            argc--;
            argv++;
        }

        // The const_cast is safe because we don’t attempt to modify argv anyway.
        // This is just so we can pass in both e.g. a `const char**` and a `char **`.
        return parse_impl(
            program_name,
            argc,
            const_cast<const char**>(argv),
            std::move(error_handler),
            user_data
        );
    }
}; // namespace detail

/// ===========================================================================
///  API
/// ===========================================================================
namespace base::cmd {
/// Main command-line options type.
///
/// See docs/clopts.md
template <typename... opts>
using clopts = detail::clopts_impl< // clang-format off
    typename detail::preprocess_subcommands<detail::filter<detail::regular_option, opts...>>::type,
    detail::filter<detail::special_option, opts...>,
    detail::filter<detail::is_directive, opts...>
>; // clang-format on

/// Types.
using detail::values;

/// Base option type.
template <
    detail::static_string name,
    detail::static_string description,
    typename type = std::string,
    opt_props props = {}>
struct option : detail::opt_impl<name, description, type, props> {};

/// Identical to 'option', but overridable by default.
template <
    detail::static_string _name,
    detail::static_string _description,
    typename type = std::string,
    bool required = false>
struct overridable : option<_name, _description, type, {.required = required, .overridable = true}> {};

/// Identical to 'option', but hidden by default.
template <
    detail::static_string _name,
    detail::static_string _description,
    typename type = std::string>
struct hidden : option<_name, _description, type, {.hidden = true}> {};

/// Base short option type.
template <
    detail::static_string _name,
    detail::static_string _description = "",
    typename _type = std::string,
    bool required = false,
    bool overridable = false,
    bool hidden = false>
struct short_option : detail::opt_impl<_name, _description, _type, {
    .required = required,
    .overridable = overridable,
    .hidden = hidden
}> {
    static constexpr decltype(_name) name = _name;
    static constexpr decltype(_description) description = _description;
    static constexpr bool is_short = true;

    constexpr short_option() = delete;
};

namespace experimental {
template <
    detail::static_string _name,
    detail::static_string _description = "",
    typename _type = std::string,
    bool required = false,
    bool overridable = false>
using short_option [[deprecated("Use short_option instead of experimental::short_option")]] =
    short_option<_name, _description, _type, required, overridable>;
} // namespace experimental

/// A positional option.
///
/// Positional options cannot be overridable; use multiple<positional<>>
/// instead.
template <
    detail::static_string _name,
    detail::static_string _description,
    typename _type = std::string,
    bool required = true>
struct positional : option<_name, _description, _type, {.required = required}> {
    using is_positional_ = std::true_type;
};

/// Func option implementation.
template <
    detail::static_string _name,
    detail::static_string _description,
    typename lambda,
    bool required = false>
struct func_impl : option<_name, _description, typename lambda::type, {.required = required}> {
    static constexpr typename lambda::lambda callback = {};
};

/// A function option.
template <
    detail::static_string _name,
    detail::static_string _description,
    auto cb,
    bool required = false>
struct func : func_impl<_name, _description, detail::make_lambda<cb>, required> {};

/// A flag option.
///
/// Flags are never required because that wouldn’t make much sense.
template <
    detail::static_string _name,
    detail::static_string _description = "",
    flag_props props = {}>
struct flag : option<_name, _description, bool, {.hidden = props.hidden}> {
    static constexpr bool default_value = props.default_value;
    static constexpr bool is_flag = true;
};

/// The help option.
template <auto _help_cb = detail::default_help_handler>
struct help : func<"--help", "Print this help information", [] {}> {
    static constexpr decltype(_help_cb) help_callback = _help_cb;
    static constexpr bool is_help_option = true;
};

/// Multiple meta-option.
template <typename opt>
struct multiple : option<opt::name, opt::description, typename opt::declared_type, opt::properties> {
    static_assert(not utils::is<typename opt::declared_type, bool>, "Type of multiple<> cannot be bool");
    static_assert(not utils::is<typename opt::declared_type, detail::callback_arg_type>, "Type of multiple<> cannot be a callback");
    static_assert(not utils::is<typename opt::declared_type, detail::callback_noarg_type>, "Type of multiple<> cannot be a callback");
    static_assert(not detail::is_multiple<opt>, "multiple<multiple<>> is invalid");
    static_assert(not requires { opt::is_stop_parsing; }, "multiple<stop_parsing<>> is invalid");
    static_assert(not requires { opt::is_subcommand; }, "multiple<subcommand<>> is invalid");
    static_assert(not opt::is_overridable, "multiple<> cannot be overridable");

    constexpr multiple() = delete;
    static constexpr bool is_multiple = true;
    static constexpr bool is_short = detail::is_short_option<opt>;
    using is_positional_ = detail::positional_t<opt>;
};

/// Subcommand; this essentially introduces a set of options that is only parsed if
/// the subcommand is specified.
template <
    detail::static_string name,
    detail::static_string description,
    typename ...options
>
struct subcommand : option<name, description, clopts<options...>> {
    static constexpr bool is_subcommand = true;
};

/// Stop parsing when this option is encountered.
template <detail::static_string stop_at = "--">
struct stop_parsing : option<stop_at, "Stop parsing command-line arguments", detail::special_tag> {
    static constexpr bool is_stop_parsing = true;
};

/// Mark that of a set of options, only one can be specified.
template <detail::static_string ...opts>
struct mutually_exclusive : detail::directive {
    static_assert(sizeof...(opts) > 1, "mutually_exclusive<> must have at least 2 arguments");
    static constexpr std::array<str, sizeof...(opts)> options{str(opts.sv())...};
    static constexpr bool is_mutually_exclusive = true;

private:
    static consteval bool validate() {
        for (auto [i, o1] : vws::enumerate(options))
            for (auto [j, o2] : vws::enumerate(options))
                if (i != j and o1 == o2)
                    return false;
        return true;
    }

    static_assert(validate(), "mutually_exclusive<>: an option cannot be exclusive with itself");
};
} // namespace base::cmd

#endif // LIBBASE_CLOPTS_HH
