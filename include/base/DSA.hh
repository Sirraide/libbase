#ifndef LIBBASE_DSA_HH
#define LIBBASE_DSA_HH

#include <base/Assert.hh>
#include <base/Utils.hh>
#include <deque>
#include <functional>
#include <generator>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

/// ====================================================================
///  API
/// ====================================================================
/// This module provides wrappers around STL data structures
/// that add additional functionality, but don’t actually change
/// anything about the implementation of those data structures;
/// this also means that it is safe to e.g. slice a base::Queue
/// down to a std::queue.
namespace base {
/// Wrapper around 'std::unordered_map' that provides 'get()',
/// and 'get_or()'.
template <
    typename KeyTy,
    typename ValueTy,
    typename HashTy = std::hash<KeyTy>,
    typename Predicate = std::equal_to<KeyTy>,
    typename Alloc = std::allocator<std::pair<const KeyTy, ValueTy>>>
class HashMap;

/// Wrapper around 'std::queue' that provides 'dequeue()' to remove
/// and return an element because not having that is annoying...
template <typename ValueType, typename Sequence = std::deque<ValueType>>
class Queue;

/// Vector that stores its elements in a way that prevents them from
/// moving around in memory.
template <typename ValueTy, template <typename> class AllocTemplate = std::allocator>
requires (not std::is_reference_v<ValueTy>)
class StableVector;

/// Map whose keys are strings.
template <typename ValueTy>
class StringMap;

/// Set whose elements are strings.
class StringSet;

/// Wrapper around 'std::map' that provides 'get()', and 'get_or()'.
template <
    typename KeyTy,
    typename ValueTy,
    typename Comparator = std::less<KeyTy>,
    typename Alloc = std::allocator<std::pair<const KeyTy, ValueTy>>>
class TreeMap;

/// Wrapper around 'std::variant' w/ members to access and check for
/// individual variant members.
template <typename... Types>
class Variant;
} // namespace base

/// ====================================================================
///  Implementation
/// ====================================================================
namespace base::detail {
template <typename KeyTy, typename ValueTy>
struct MapMixin {
    /// Get an element if it exists.
    auto get(this auto& self, const KeyTy& key) -> std::optional<ValueTy> {
        auto it = self.find(key);
        if (it == self.end()) return std::nullopt;
        return it->second;
    }

    /// Get an element if it exists, or a default value otherwise.
    auto get_or(this auto& self, const KeyTy& key, ValueTy def) -> ValueTy {
        auto it = self.find(key);
        if (it == self.end()) return std::move(def);
        return it->second;
    }
};

struct StringHash {
    using is_transparent = void;
    template <typename T>
    requires requires (const T& t) { std::string_view{t}; }
    auto operator()(const T& t) const -> usz {
        return std::hash<std::string_view>{}(std::string_view{t});
    }
};
} // namespace base::detail

template <
    typename KeyTy,
    typename ValueTy,
    typename HashTy,
    typename Predicate,
    typename Alloc>
class base::HashMap : public std::unordered_map<KeyTy, ValueTy, HashTy, Predicate, Alloc>
    , public detail::MapMixin<KeyTy, ValueTy> {
    using Base = std::unordered_map<KeyTy, ValueTy, HashTy, Predicate, Alloc>;

public:
    using Base::Base;
};

template <typename ValueType, typename Sequence>
class base::Queue : public std::queue<ValueType, Sequence> {
    using Base = std::queue<ValueType, Sequence>;

public:
    using Base::Base;

    /// Pop the first element off the queue and return it.
    [[nodiscard]] auto dequeue() -> ValueType {
        auto v = std::move(Base::front());
        Base::pop();
        return v;
    }

    /// Streaming iterator that pops elements off the queue; it
    /// is safe to enqueue new elements while iterating.
    [[nodiscard]] auto stream() -> std::generator<ValueType> {
        while (not Base::empty())
            co_yield dequeue();
    }
};

template <typename ValueTy, template <typename> class AllocTemplate>
requires (not std::is_reference_v<ValueTy>)
class base::StableVector {
    using UniquePtr = std::unique_ptr<ValueTy>;
    using VectorTy = std::vector<UniquePtr, AllocTemplate<UniquePtr>>;
    VectorTy data;

    template <typename VectorIt, typename ValueType>
    class Iterator {
        VectorIt it;
        friend StableVector;
        explicit Iterator(VectorIt it) : it(it) {}

    public:
        using value_type = ValueType;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        Iterator() = default;

        [[nodiscard]] constexpr auto operator*() const -> reference { return **it; }
        [[nodiscard]] constexpr auto operator->() const -> pointer { return it->get(); }
        constexpr auto operator++() -> Iterator& {
            ++it;
            return *this;
        }

        [[nodiscard]] constexpr auto operator++(int) -> Iterator {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr auto operator--() -> Iterator& {
            --it;
            return *this;
        }

        [[nodiscard]] constexpr auto operator--(int) -> Iterator {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        [[nodiscard]] constexpr auto operator+(difference_type n) const -> Iterator {
            auto tmp = *this;
            tmp.it += n;
            return tmp;
        }

        [[nodiscard]] friend constexpr auto operator+(
            difference_type n,
            const Iterator& it
        ) -> Iterator {
            return it + n;
        }

        [[nodiscard]] constexpr auto operator-(difference_type n) const -> Iterator {
            auto tmp = *this;
            tmp.it -= n;
            return tmp;
        }

        [[nodiscard]] constexpr auto operator-(const Iterator& other) const -> difference_type {
            return it - other.it;
        }

        [[nodiscard]] constexpr auto operator+=(difference_type n) -> Iterator& {
            it += n;
            return *this;
        }

        [[nodiscard]] constexpr auto operator-=(difference_type n) -> Iterator& {
            it -= n;
            return *this;
        }

        [[nodiscard]] constexpr auto operator[](difference_type n) const -> reference {
            return *it[n];
        }

        [[nodiscard]] constexpr auto operator<=>(const Iterator& other) const = default;
    };

public:
    using value_type = ValueTy;
    using iterator = Iterator<typename VectorTy::iterator, ValueTy>;
    using const_iterator = Iterator<typename VectorTy::const_iterator, const ValueTy>;

    /// Create an empty stable vector.
    constexpr StableVector() = default;

    /// Get the last element in the vector.
    [[nodiscard]] constexpr auto back(this auto& self) -> decltype(auto) {
        Assert(not self.empty(), "Vector is empty!");
        return std::forward_like<decltype(self)>(*self.data.back());
    }

    /// Get an iterator to the start of the vector.
    [[nodiscard]] constexpr auto begin() -> iterator {
        return iterator(data.begin());
    }

    [[nodiscard]] constexpr auto begin() const -> const_iterator {
        return const_iterator(data.begin());
    }

    /// Clear all elements from the vector.
    constexpr auto clear() -> void { data.clear(); }

    /// Construct a new element at the back of the vector.
    template <typename T = ValueTy, typename... Args>
    constexpr auto emplace_back(Args&&... args) -> T& {
        auto& ref = push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T&>(ref);
    }

    /// Get a range to the underlying element storage; this can be passed
    /// to algorithms such as 'std::sort' or 'std::shuffle' so they can operate
    /// on the elements directly.
    [[nodiscard]] constexpr auto elements(this auto& self) {
        return std::forward_like<decltype(self)>(self.data) | vws::all;
    }

    /// Check if the vector is empty.
    [[nodiscard]] constexpr auto empty() const -> bool { return data.empty(); }

    /// Get an iterator to the end of the vector.
    [[nodiscard]] constexpr auto end() -> iterator {
        return iterator(data.end());
    }

    [[nodiscard]] constexpr auto end() const -> const_iterator {
        return const_iterator(data.end());
    }

    /// Erase an element from the vector. This compares elements by *identity*,
    /// not by equivalence (see index_of). Returns whether an element was removed.
    constexpr bool erase(const ValueTy& value) {
        auto it = rgs::find_if(data, [&](const auto& ptr) { return ptr.get() == &value; });
        if (it == data.end()) return false;
        data.erase(it);
        return true;
    }

    /// Erase all elements that satisfy a predicate.
    template <typename Predicate>
    constexpr auto erase_if(Predicate pred) -> void {
        std::erase_if(data, [&](const auto& ptr) { return std::invoke(pred, *ptr); });
    }

    /// Get the first element in the vector.
    [[nodiscard]] constexpr auto front(this auto& self) -> decltype(auto) {
        Assert(not self.empty(), "Vector is empty!");
        return std::forward_like<decltype(self)>(*self.data.front());
    }

    /// Get the index of an element in the vector, if it is in the vector;
    /// this compares elements by *identity*, not by equivalence: it only
    /// succeeds if passed a reference to an element that is actually stored
    /// in this vector.
    [[nodiscard]] constexpr auto index_of(const ValueTy& value) const -> std::optional<usz> {
        auto it = rgs::find_if(data, [&](const auto& ptr) { return ptr.get() == &value; });
        if (it == data.end()) return std::nullopt;
        return it - data.begin();
    }

    /// Remove the last element from the vector.
    constexpr auto pop_back() -> UniquePtr {
        auto val = std::move(data.back());
        data.pop_back();
        return val;
    }

    /// Push a new element to the back of the vector.
    constexpr auto push_back(ValueTy value) -> ValueTy& {
        return push_back(std::make_unique<ValueTy>(std::move(value)));
    }

    constexpr auto push_back(std::unique_ptr<ValueTy> value) -> ValueTy& {
        data.push_back(std::move(value));
        return *data.back();
    }

    /// Get the number of elements in the vector.
    [[nodiscard]] constexpr auto size() const -> usz { return data.size(); }

    /// Swap the elements at two indices.
    constexpr void swap_indices(usz idx1, usz idx2) {
        Assert(idx1 < size(), "Index {} out of bounds!", idx1);
        Assert(idx2 < size(), "Index {} out of bounds!", idx2);
        std::swap(data[idx1], data[idx2]);
    }

    /// Swap the elements at two iterators.
    constexpr void swap_iterators(iterator it1, iterator it2) {
        std::iter_swap(it1.it, it2.it);
    }

    /// Get the element at an index.
    [[nodiscard]] constexpr auto operator[](this auto&& self, std::unsigned_integral auto idx) -> decltype(auto) {
        Assert(idx < self.size(), "Index {} out of bounds!", idx);
        return std::forward_like<decltype(self)>(*self.data[idx]);
    }

    [[nodiscard]] constexpr auto operator[](this auto&& self, std::signed_integral auto idx) -> decltype(auto) {
        Assert(idx >= 0, "Index {} out of bounds!", idx);
        Assert(usz(idx) < self.size(), "Index {} out of bounds!", usz(idx));
        return std::forward_like<decltype(self)>(*self.data[usz(idx)]);
    }
};

template <typename ValueType>
class base::StringMap : public HashMap<std::string, ValueType, detail::StringHash, std::equal_to<>> {
    using Base = HashMap<std::string, ValueType, detail::StringHash, std::equal_to<>>;

public:
    using Base::Base;
    using Base::operator[];
    using Base::at;
    using Base::get;
    using Base::get_or;

    auto operator[](this auto& self, std::string_view sv) -> decltype(auto) {
        return self[std::string{sv}];
    }

    auto at(this auto& self, std::string_view sv) -> decltype(auto) {
        return self.at(std::string{sv});
    }

    auto get(this auto& self, std::string_view sv) -> decltype(auto) {
        return self.get(std::string{sv});
    }

    template <usz n>
    auto get(this auto& self, const char (&str)[n]) -> decltype(auto) {
        return self.get(std::string_view{str, n - 1});
    }

    auto get_or(this auto& self, std::string_view sv, ValueType def) -> decltype(auto) {
        return self.get_or(std::string{sv}, std::move(def));
    }

    template <usz n>
    auto get_or(this auto& self, const char (&str)[n], ValueType def) -> decltype(auto) {
        return self.get_or(std::string_view{str, n - 1}, std::move(def));
    }
};

class base::StringSet : public std::unordered_set<std::string, detail::StringHash, std::equal_to<>> {
    using Base = std::unordered_set<std::string, detail::StringHash, std::equal_to<>>;

public:
    using Base::Base;
    using Base::insert;

    template <usz n>
    auto insert(const char (&str)[n]) -> std::pair<iterator, bool> {
        return Base::insert(std::string{str, n - 1});
    }

    auto insert(std::string_view sv) -> std::pair<iterator, bool> {
        return Base::insert(std::string{sv});
    }
};

template <
    typename KeyTy,
    typename ValueTy,
    typename Comparator,
    typename Alloc>
class base::TreeMap : public std::map<KeyTy, ValueTy, Comparator, Alloc>
    , public detail::MapMixin<KeyTy, ValueTy> {
    using Base = std::map<KeyTy, ValueTy, Comparator, Alloc>;

public:
    using Base::Base;
};

template <typename... Types>
class base::Variant : public std::variant<Types...> {
public:
    using std::variant<Types...>::variant;

    /// Get a reference to variant data, asserting on type mismatch.
    template <typename Ty, typename Self>
    [[nodiscard]] constexpr auto get(this Self&& self) -> decltype(std::get<Ty>(std::forward<Self>(self))) {
        auto* val = std::get_if<Ty>(&self);
        if (not val) utils::ThrowOrAbort("Variant does not hold the requested type.");

        // This should *really* just use `std::forward_like`, but due to a
        // problem between Clang treating it as a builtin and libstdc++’s
        // implementation, that doesn’t work currently, so we’re stuck with
        // this nonsense.
        return static_cast<decltype(std::get<Ty>(std::forward<Self>(self)))>(*val);
    }

    /// Get a pointer to the data of this variant as the given type.
    ///
    /// This purposefully only supports lvalues.
    template <typename Ty>
    [[nodiscard]] constexpr auto get_if(this auto& self) noexcept { return std::get_if<Ty>(&self); }

    /// Check if this variant holds (one of) the given type(s).
    template <typename... Ty>
    [[nodiscard]] constexpr bool is() const {
        return (std::holds_alternative<Ty>(*this) or ...);
    }

    /// Visit this variant.
    template <typename Visitor, typename Self>
    constexpr auto visit(this Self&& self, Visitor&& visitor)
        // Need to specify this explicitly, otherwise, we run into
        // problems with recursive visitors because it can’t deduce
        // the return type of this in that case.
        -> decltype(std::visit(std::forward<Visitor>(visitor), std::forward<Self>(self))) {
        return std::visit(std::forward<Visitor>(visitor), std::forward<Self>(self));
    }
};

#endif
