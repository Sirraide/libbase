#ifndef LIBBASE_UTILS_HH
#define LIBBASE_UTILS_HH

#include <base/Types.hh>
#include <base/Assert.hh>

#include <algorithm>
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
template <typename T, typename ...ElemTys>
concept ConvertibleRange = rgs::range<T> and (std::convertible_to<rgs::range_value_t<T>, ElemTys> or ...);

/// A list of types.
template <typename... Types>
struct list {
    template <typename Callable>
    static constexpr bool any(Callable&& callable) {
        return (callable.template operator()<Types>() or ...);
    }

    template <typename Callable>
    static constexpr void each(Callable&& callable) {
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
constexpr auto enumerate(auto&& range) {
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

/// Sort elements in a range and unique them.
template <typename Container>
void unique_sort(Container&& r) {
    rgs::sort(r);
    auto [first, last] = rgs::unique(r);
    r.erase(first, last);
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
    constexpr basic_zstring() {
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
    consteval basic_zstring(const char_type (&data)[n]) : value{text_type{data, n - 1}} {
        Assert(data[n - 1] == 0, "String must be null-terminated");
    }

    /// Create a zstring from a std::string.
    constexpr basic_zstring(const string_type& str) : value{text_type{str}} {}

    /// Create a zstring from a pointer and size.
    constexpr basic_zstring(const char_type* data, usz size) : value{text_type{data, size}} {
        Assert(data[size] == 0, "String must be null-terminated");
    }

    /// Get the data pointer.
    [[nodiscard]] constexpr auto c_str() const -> const char_type* { return data(); }
    [[nodiscard]] constexpr auto data() const -> const char_type* { return str().data(); }

    /// Get the size of the string.
    [[nodiscard]] constexpr auto size() const -> usz { return str().size(); }

    /// Get the string.
    [[nodiscard]] constexpr auto str() const -> text_type {
        return value;
    }
};

using zstring = basic_zstring<char>;
using u8zstring = basic_zstring<char8_t>;
using u16zstring = basic_zstring<char16_t>;
using u32zstring = basic_zstring<char32_t>;

/// Compile-time string.
template <usz sz>
struct static_string {
    char arr[sz]{};
    usz len{};

    /// Construct an empty string.
    constexpr static_string() {}

    /// Construct a string from a 'std::string'
    constexpr static_string(const std::string& s) {
        Assert(sz >= s.size() + 1);
        std::copy_n(s.data(), sz, arr);
        len = s.size();
    }

    /// Construct from a string literal.
    consteval static_string(const char (&_data)[sz]) {
        Assert(_data[sz - 1] == 0, "Expected null-terminated string");
        std::copy_n(_data, sz, arr);
        len = sz - 1;
    }

    /// Check if two strings are equal.
    template <usz n>
    [[nodiscard]] constexpr bool operator==(const static_string<n>& s) const {
        return sv() == s.sv();
    }

    /// Check if this is equal to a string.
    [[nodiscard]] constexpr bool operator==(std::string_view s) const {
        return sv() == s;
    }

    /// Overwrite the string data.
    consteval void assign(std::string_view data) {
        len = 0;
        append(data);
    }

    /// Append to this string.
    template <usz n>
    constexpr void append(const static_string<n>& str) {
        Assert(len + str.len < sz, "Cannot append string because it is too long");
        std::copy_n(str.arr, str.len, arr + len);
        len += str.len;
    }

    /// Append a string literal with a known length to this string.
    consteval void append(std::string_view s) {
        std::copy_n(s.data(), s.size(), arr + len);
        len += s.size();
    }

    /// Get the string as a \c std::string_view.
    [[nodiscard]] constexpr auto sv() const -> std::string_view { return {arr, len}; }

    /// Iterators.
    [[nodiscard]] constexpr auto begin() -> char* { return data(); }
    [[nodiscard]] constexpr auto begin() const -> const char* { return data(); }
    [[nodiscard]] constexpr auto end() -> char* { return data() + size(); }
    [[nodiscard]] constexpr auto end() const -> const char* { return data() + size(); }

    /// API for static_assert.
    [[nodiscard]] constexpr auto data() -> char* { return arr; }
    [[nodiscard]] constexpr auto data() const -> const char* { return arr; }
    [[nodiscard]] constexpr auto size() const -> usz { return len; }

    /// Check if this is empty.
    [[nodiscard]] constexpr bool empty() const { return len == 0; }

    /// Concatenate this with another string.
    template <usz n>
    [[nodiscard]] constexpr auto operator+(const static_string<n>& s) const -> static_string<sz + n> {
        static_string<sz + n> new_string;
        new_string.append(*this);
        new_string.append(s);
        return new_string;
    }

    static constexpr bool is_static_string = true;
};

/// Deduction guide to shut up nonsense CTAD warnings.
template <usz sz>
static_string(const char (&)[sz]) -> static_string<sz>;


/// A byte-sized type or 'void'.
///
/// Basically, this concept denotes any type 'T' such that 'T*' can reasonably
/// be used as ‘a pointer to a bunch of bytes’.
template <typename T>
concept ByteSizedPointee = utils::is_same<std::remove_cv_t<T>,
    void,
    char,
    signed char,
    unsigned char,
    std::byte
>;

/// A byte-sized pointer.
template <typename T>
concept BytePointer = std::is_pointer_v<T> and ByteSizedPointee<std::remove_pointer_t<T>>;

/// A range of bytes.
template <typename Range>
concept SizedByteRange = requires (Range r) {
    { r.data() } -> BytePointer;
    { r.size() } -> std::convertible_to<usz>;
};

/// A range of bytes that can be resized.
template <typename Range>
concept ResizableByteRange = requires (Range r, usz i) {
    { r.data() } -> BytePointer;
    { r.size() } -> std::convertible_to<usz>;
    { r.resize(i) };
};
} // namespace base::utils

namespace base {
using utils::BytePointer;
}

template <typename CharType>
struct std::formatter<base::utils::basic_zstring<CharType>> : std::formatter<std::basic_string_view<CharType>> {
    template <typename FormatContext>
    auto format(base::utils::basic_zstring<CharType> str, FormatContext& ctx) const {
        return std::formatter<std::basic_string_view<CharType>>::format(str.str(), ctx);
    }
};

template <std::size_t N>
struct std::formatter<base::utils::static_string<N>> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const base::utils::static_string<N>& str, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(str.sv(), ctx);
    }
};

#endif // LIBBASE_UTILS_HH
