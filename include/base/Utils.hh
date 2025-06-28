#ifndef LIBBASE_UTILS_HH
#define LIBBASE_UTILS_HH

#include <algorithm>
#include <base/Types.hh>
#include <format>
#include <functional>
#include <ranges>
#include <source_location>
#include <string>
#include <variant>

namespace base::utils {

/// Check if one or more types are the same, ignoring top-level references
/// and cv qualifiers.
template <typename T, typename... Us>
concept is = (std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Us>> or ...);

/// Check if one or more types are exactly the same.
template <typename T, typename... Us>
concept is_same = (std::is_same_v<T, Us> or ...);

/// Check if a type is convertible to any of a set of types.
template <typename T, typename... Us>
concept convertible_to_any = (std::convertible_to<T, Us> or ...);

/// Range of things that are convertible to a certain type.
template <typename T, typename ElemTy>
concept ConvertibleRange = rgs::range<T> and std::convertible_to<rgs::range_value_t<T>, ElemTy>;

/// A list of types.
template <typename... Types>
struct list {
    template <typename Callable>
    static void each(Callable&& callable) {
        (callable.template operator()<Types>(), ...);
    }
};

/// Check if a range is empty. This circumvents the stupid
/// amortised time constraint that the standard library’s
/// rgs::empty() has.
template <typename Range>
bool Empty(Range&& range) {
    if constexpr (requires { rgs::empty(std::forward<Range>(range)); }) {
        return rgs::empty(std::forward<Range>(range));
    } else {
        return range.begin() == range.end();
    }
}

/// Format a value as a human-readable number.
auto HumanReadable(u64 value) -> std::string;
auto HumanReadable(i64 value) -> std::string;

template <typename... Types>
struct Overloaded : Types... {
    using Types::operator()...;
};

template <typename... Types>
Overloaded(Types...) -> Overloaded<Types...>;

/// Libc++ supports zip but not enumerate, so use this instead.
auto enumerate(auto&& range) {
#ifdef __cpp_lib_ranges_enumerate
    return vws::enumerate(std::forward<decltype(range)>(range));
#else
    return vws::zip(vws::iota(0z), std::forward<decltype(range)>(range));
#endif
}

/// Erase an element from a container without maintaining order.
template <typename Container, typename Iterator>
void erase_unordered(Container& container, Iterator it) {
    if (container.size() == 1) container.erase(it);
    else {
        std::iter_swap(it, container.end() - 1);
        container.pop_back();
    }
}

/// Splice a value into a container at a specific position.
///
/// This function performs insertion and erasure at the same
/// time; this has the benefit of avoiding moving any elements
/// if the replacement has the same size as the number of elements
/// removed.
///
/// Constraints: begin_remove < end_remove, both pointing into
/// range, and input_range is not a subrange of range.
///
/// \param container The container to splice into.
/// \param begin_remove The start of the range to remove.
/// \param end_remove The end of the range to remove.
/// \param input_range The range to copy into \p range.
/// FIXME: Untested.
template <typename Container, typename Iterator, typename Input>
void splice(Container& container, Iterator begin_remove, Iterator end_remove, Input&& input_range) {
    // If the replacement is the same size as the range to remove,
    // we can just copy the elements in place.
    usz add_size = usz(rgs::distance(input_range));
    usz remove_size = usz(std::distance(begin_remove, end_remove));

    // If we’re adding elements, make space for them.
    if (add_size > remove_size) {
        auto old_size = container.size();
        auto extra = add_size - remove_size;
        container.resize(container.size() + extra);

        // Make space by moving 'extra' elements to the right,
        // starting at the end of the range to remove because
        // the part before it is going to be replaced anyway.
        rgs::move_backward(
            end_remove,
            container.begin() + old_size,
            end_remove + extra
        );
    }

    // If we’re removing elements, we need to move elements
    // back so we’re not left with a gap.
    else if (add_size < remove_size) {
        rgs::move(
            end_remove,
            container.end(),
            begin_remove + add_size
        );

        auto extra = remove_size - add_size;
        container.resize(container.size() - extra);
    }

    // Finally, copy the new elements into the range.
    rgs::copy(std::forward<Input>(input_range), begin_remove);
}

/// std::visit, but with a better order of arguments.
template <typename Variant, typename Visitor>
constexpr decltype(auto) Visit(Visitor&& visitor, Variant&& variant) {
    return std::visit(std::forward<Variant>(variant), std::forward<Visitor>(visitor));
}


/// Non-owning zero-terminated string.
///
/// This is meant to be used in function parameters only to avoid allocations if
/// possible while simultaneously ensuring that the string data is null-terminated.
///
/// Do NOT store a copy of this anywhere, ever.
template <typename CharType>
class basic_zstring {
    using char_type = CharType;
    using text_type = std::basic_string_view<char_type>;
    using string_type = std::basic_string<char_type>;

    text_type value;

public:
    /// Create an empty string.
    basic_zstring() {
        if constexpr (std::is_same_v<char_type, char>) {
            value = "";
        } else if constexpr (std::is_same_v<char_type, char8_t>) {
            value = u8"";
        } else if constexpr (std::is_same_v<char_type, char16_t>) {
            value = u"";
        } else if constexpr (std::is_same_v<char_type, char32_t>) {
            value = U"";
        } else if constexpr (std::is_same_v<char_type, wchar_t>) {
            value = L"";
        } else {
            static_assert(false, "Unsupported character type");
        }
    }

    /// Create a zstring from a string literal.
    template <usz n>
    basic_zstring(const char_type (&data)[n]) : value{text_type{data, n - 1}} {}

    /// Create a zstring from a std::string.
    basic_zstring(const string_type& str) : value{text_type{str}} {}

    /// Create a zstring from a pointer and size.
    basic_zstring(const char_type* data, usz size) : value{text_type{data, size}} {}

    /// Get the data pointer.
    [[nodiscard]] auto c_str() const -> const char_type* { return data(); }
    [[nodiscard]] auto data() const -> const char_type* { return str().data(); }

    /// Get the size of the string.
    [[nodiscard]] auto size() const -> usz { return str().size(); }

    /// Get the string.
    [[nodiscard]] auto str() const -> text_type {
        return value;
    }
};

using zstring = basic_zstring<char>;
using u8zstring = basic_zstring<char8_t>;
using u16zstring = basic_zstring<char16_t>;
using u32zstring = basic_zstring<char32_t>;
} // namespace base::utils

template <typename CharType>
struct std::formatter<base::utils::basic_zstring<CharType>> : std::formatter<std::basic_string_view<CharType>> {
    template <typename FormatContext>
    auto format(base::utils::basic_zstring<CharType> str, FormatContext& ctx) const {
        return std::formatter<std::basic_string_view<CharType>>::format(str.str(), ctx);
    }
};

#endif // LIBBASE_UTILS_HH
