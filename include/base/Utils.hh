#ifndef LIBBASE_UTILS_HH
#define LIBBASE_UTILS_HH

#include <algorithm>
#include <ranges>
#include <source_location>
#include <string>
#include <variant>
#include <format>
#include <base/Types.hh>

namespace base::utils {
template <typename T, typename... Us>
concept is = (std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Us>> or ...);

/// Range of things that are convertible to a certain type.
template <typename T, typename ElemTy>
concept ConvertibleRange = rgs::range<T> and std::convertible_to<rgs::range_value_t<T>, ElemTy>;

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

template <typename... Types>
struct Overloaded : Types... {
    using Types::operator()...;
};

template <typename... Types>
Overloaded(Types...) -> Overloaded<Types...>;

/// Erase an element from a container without maintaining order.
template <typename Container, typename Iterator>
void erase_unordered(Container& container, Iterator it) {
    if (container.size() == 1) container.erase(it);
    else {
        std::iter_swap(it, container.end() - 1);
        container.pop_back();
    }
}

/// Join a range of strings.
template <typename Range>
std::string join(const Range& range, std::string_view sep = ", ") {
    using V = rgs::range_value_t<Range>;
    std::string result;
    auto begin = rgs::begin(range);
    auto end = rgs::end(range);
    for (auto it = begin; it != end; ++it) {
        if (it != begin) result += sep;
        if constexpr (is<V, std::string, std::string_view, char>) result += *it;
        else result += std::format("{}", *it);
    }
    return result;
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

/// Replace all occurrences of `from` with `to` in `str`.
void ReplaceAll(std::string& str, std::string_view from, std::string_view to);

/// Used instead of 'Assert()' in some places so we can catch the
/// exception in unit tests.
[[noreturn]] void ThrowOrAbort(
    const std::string& message,
    std::source_location loc = std::source_location::current()
);

/// std::visit, but with a better order of arguments.
template <typename Variant, typename Visitor>
constexpr decltype(auto) Visit(Visitor&& visitor, Variant&& variant) {
    return std::visit(std::forward<Variant>(variant), std::forward<Visitor>(visitor));
}
} // namespace base::utils

#endif // LIBBASE_UTILS_HH
