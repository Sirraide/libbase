#ifndef BASE_FIXEDVECTOR_HH
#define BASE_FIXEDVECTOR_HH

#include <base/Macros.hh>
#include <base/Types.hh>
#include <filesystem>
#include <limits>
#include <memory>

namespace base {
namespace detail {
template <usz Elements>
using FixedVectorSizeType = // clang-format off
    std::conditional_t<Elements < std::numeric_limits<u8>::max(), u8,
    std::conditional_t<Elements < std::numeric_limits<u16>::max(), u16,
    std::conditional_t<Elements < std::numeric_limits<u32>::max(), u32,
    u64
>>>; // clang-format on
}

/// A fixed sized vector.
///
/// This is more or less a combination of std::array and std::vector
/// in that it stores its elements inline, but unlike an array it can
/// contain fewer than the maximum number of elements.
template <typename ValueType, usz Capacity>
class FixedVector {
    static_assert(Capacity > 0, "Element count must be at least 1");
    static_assert(
        std::is_same_v<std::remove_cv_t<ValueType>, ValueType>,
        "value_type of FixedVector must be a non-const, non-volatile, non-reference type"
    );

    using SizeType = detail::FixedVectorSizeType<Capacity>;

    static constexpr bool can_move =
        std::is_move_assignable_v<ValueType> and
        std::is_move_constructible_v<ValueType>;

    static constexpr bool can_copy =
        std::is_copy_assignable_v<ValueType> and
        std::is_copy_constructible_v<ValueType>;

    alignas(ValueType) std::byte _m_data[sizeof(ValueType) * Capacity];
    SizeType _m_sz = 0;

public:
    using value_type = ValueType;
    using size_type = usz;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /// Construct an empty vector.
    FixedVector() = default;

    /// Copy constructor.
    explicit(not std::is_trivially_copy_constructible_v<value_type>)
    FixedVector(const FixedVector& other) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        requires std::is_copy_constructible_v<value_type> {
        std::uninitialized_copy(other.begin(), other.end(), begin());
        _m_sz = other._m_sz;
    }

    /// Move constructor.
    explicit(not std::is_trivially_move_constructible_v<value_type>)
    FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        requires std::is_move_constructible_v<value_type> {
        std::uninitialized_move(other.begin(), other.end(), begin());
        std::destroy(other.begin(), other.end());
        _m_sz = std::exchange(other._m_sz, 0);
    }

    /// Construct a vector containing a number of elements.
    FixedVector(std::initializer_list<value_type> init) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        requires std::is_copy_constructible_v<value_type> {
        if (init.size() > Capacity) utils::ThrowOrAbort(
            "Initialiser list size exceeds FixedVector capacity"
        );

        std::uninitialized_copy(init.begin(), init.end(), begin());
        _m_sz = static_cast<SizeType>(init.size());
    }

    /// Construct a vector from a single element.
    explicit FixedVector(value_type val) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        requires std::is_move_constructible_v<value_type> {
        std::construct_at(begin(), std::move(val));
        _m_sz = static_cast<SizeType>(1);
    }

    /// Destroy the vector.
    ~FixedVector() noexcept(std::is_nothrow_destructible_v<value_type>) {
        std::destroy(begin(), end());
    }

    /// Copy assignment operator.
    FixedVector& operator=(const FixedVector& other) noexcept(
        std::is_nothrow_copy_assignable_v<value_type> and
        std::is_nothrow_copy_constructible_v<value_type>
    ) requires can_copy {
        if (this == std::addressof(other)) return *this;

        // Copy existing elements.
        auto existing = std::min(_m_sz, other._m_sz);
        std::copy(other.begin(), other.begin() + existing, begin());

        // If the other vector contains more elements, append the rest.
        if (other._m_sz > _m_sz) std::uninitialized_copy(
            other.begin() + existing,
            other.end(),
            begin() + existing
        );

        // If we contain more elements, destroy the rest.
        else if (_m_sz > other._m_sz) std::destroy(
            begin() + existing,
            end()
        );

        _m_sz = other._m_sz;
        return *this;
    }

    /// Move assignment operator.
    FixedVector& operator=(FixedVector&& other) noexcept(
        std::is_nothrow_move_assignable_v<value_type> and
        std::is_nothrow_move_constructible_v<value_type>
    ) requires can_move {
        if (this == std::addressof(other)) return *this;

        // Move existing elements.
        auto existing = std::min(_m_sz, other._m_sz);
        std::move(other.begin(), other.begin() + existing, begin());

        // If the other vector contains more elements, append the rest.
        if (other._m_sz > _m_sz) std::uninitialized_move(
            other.begin() + existing,
            other.end(),
            begin() + existing
        );

        // If we contain more elements, destroy the rest.
        else if (_m_sz > other._m_sz) std::destroy(
            begin() + existing,
            end()
        );

        std::destroy(other.begin(), other.end());
        _m_sz = std::exchange(other._m_sz, 0);
        return *this;
    }

    /// Get an iterator to the start of the vector.
    [[nodiscard]] auto begin() noexcept -> iterator { return data(); }
    [[nodiscard]] auto begin() const noexcept -> const_iterator { return data(); }

    /// Get an iterator to the end of the vector.
    [[nodiscard]] auto end() noexcept -> iterator { return begin() + _m_sz; }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return  begin() + _m_sz; }

    /// Get a reverse iterator to the last element.
    [[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
        return  reverse_iterator(end());
    }

    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }

    /// Get a reverse iterator to the first element.
    [[nodiscard]] auto rend() noexcept -> reverse_iterator {
        return reverse_iterator(begin());
    }

    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }

    /// Get an element at an index.
    template <typename Self>
    [[nodiscard]] auto operator[](this Self&& self, usz idx) -> decltype(auto) {
        if (idx >= usz(self._m_sz)) utils::ThrowOrAbort(
            std::format(
                "FixedVector: Index {} out of range for vector of size {}",
                idx,
                self._m_sz
            )
        );

        return std::forward_like<Self>(self.data()[idx]);
    }

    /// Get the first element.
    template <typename Self>
    [[nodiscard]] auto front(this Self&& self) -> decltype(auto) {
        return std::forward<Self>(self).operator[](0);
    }

    /// Get the last element.
    template <typename Self>
    [[nodiscard]] auto back(this Self&& self) -> decltype(auto) {
        self._m_check_not_empty();
        return std::forward<Self>(self).operator[](self._m_sz - SizeType(1));
    }

    /// Get the data pointer.
    [[nodiscard]] auto data() noexcept -> pointer {
        return reinterpret_cast<pointer>(_m_get_ptr());
    }

    [[nodiscard]] auto data() const noexcept -> const_pointer {
        return reinterpret_cast<const_pointer>(_m_get_ptr());
    }

    /// Get the size of the vector.
    [[nodiscard]] auto size() const noexcept -> usz {
        return static_cast<usz>(_m_sz);
    }

    /// Check if this vector is empty.
    [[nodiscard]] bool empty() const noexcept {
        return size() == 0u;
    }

    /// Get the capacity of the vector.
    [[nodiscard]] static constexpr auto capacity() noexcept -> usz {
        return Capacity;
    }

    /// Clear the vector.
    void clear() {
        std::destroy(begin(), end());
        _m_sz = 0;
    }

    /// Insert an element before the element the iterator points to.
    void insert(iterator it, const_reference el) requires can_copy and can_move {
        _m_insert(it, el);
    }

    void insert(iterator it, value_type&& el) requires can_move {
        _m_insert(it, std::move(el));
    }

    /// Erase the specified element from the vector.
    void erase(iterator it) requires can_move {
        erase(it, it + 1);
    }

    /// Erase a range of elements.
    void erase(iterator first, iterator last_exclusive) requires can_move {
        auto removed = SizeType(last_exclusive - first);
        auto e = end();
        _m_sz -= removed;
        std::move(last_exclusive, e, first);
        std::destroy(end(), e);
    }

    /// Emplace an element at the back of the vector.
    template <typename... Args>
    auto emplace_back(Args&& ...args) -> reference {
        _m_check_size();
        std::construct_at(end(), std::forward<Args>(args)...);
        ++_m_sz;
        return back();
    }

    /// Push an element into the vector.
    void push_back(const_reference val) requires can_copy {
        emplace_back(val);
    }

    void push_back(value_type&& val) requires can_move {
        emplace_back(std::move(val));
    }

    /// Remove the last element.
    void pop_back() {
        _m_check_not_empty();
        --_m_sz;
        std::destroy_at(end());
    }

    /// Comparison.
    [[nodiscard]] bool operator==(const FixedVector& other) const
    requires requires (value_type v) { v == v; } {
        return _m_sz == other._m_sz and std::equal(begin(), end(), other.begin());
    }

    [[nodiscard]] auto operator<=>(const FixedVector& other) const
    requires requires (value_type v) { v == v; } {
        return std::lexicographical_compare_three_way(
            begin(), end(),
            other.begin(), other.end()
        );
    }

private:
    void _m_check_size(usz elements = 1) {
        if (usz(size()) + elements > Capacity) utils::ThrowOrAbort(
            std::format(
                "FixedVector: Not enough space to append {} elements",
                elements
            )
        );
    }

    void _m_check_not_empty() {
        if (empty()) utils::ThrowOrAbort("FixedVector is empty");
    }

    auto _m_get_ptr() const -> pointer {
        auto p = const_cast<pointer>(reinterpret_cast<const_pointer>(_m_data));

        // Calling clear() is supported even if value_type is not movable, in
        // which case we need to launder the pointer here.
        if constexpr (not can_move) {
            return std::launder(p);
        } else {
            return p;
        }
    }

    template <typename T>
    void _m_insert(iterator it, T&& ref) {
        _m_check_size();
        LIBBASE_DEFER { ++_m_sz; };

        if (it == end()) {
            std::construct_at(end(), std::forward<T>(ref));
            return;
        }

        // Can’t move into end() + 1 since that element isn’t constructed yet;
        // so move that one manually.
        std::construct_at(end(), std::move(*std::prev(end())));
        std::move_backward(it, end() - 1, end());
        *it = std::forward<T>(ref);
    }
};

template <typename ValueType, usz Capacity, typename Callback>
void erase_if(FixedVector<ValueType, Capacity>& vector, Callback&& callback) {
    vector.erase(std::remove_if(vector.begin(), vector.end(), callback), vector.end());
}
} // namespace base

#endif // BASE_FIXEDVECTOR_HH
