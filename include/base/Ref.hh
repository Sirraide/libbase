#ifndef LIBBASE_REF_HH
#define LIBBASE_REF_HH

#include <atomic>
#include <base/Assert.hh>
#include <concepts>

namespace base {
class RefBase;
template <std::derived_from<RefBase> T> class Ref;

/// Base mixin class for defining reference-counted types.
class RefBase {
    LIBBASE_IMMOVABLE(RefBase);

    template <std::derived_from<RefBase> T>
    friend class Ref;

    mutable std::atomic<int> ref_count = 0;

protected:
    /// Constructor.
    RefBase() = default;

    /// Destructor.
#ifndef NDEBUG
    ~RefBase() LIBBASE_NOEXCEPT_UNLESS_TESTING {
        Assert(
            ref_count.load(std::memory_order_acquire) == 0,
            "Destructor called with non-zero refcount?"
        );
    }
#else
    ~RefBase() = default;
#endif

public:
    /// Create a reference-counted pointer from this.
    template <typename Self>
    [[nodiscard]] auto ref(this auto& self) -> Ref<Self> {
        return {std::addressof(self)};
    }

private:
    void _retain() noexcept { ref_count.fetch_add(1, std::memory_order_release); }
    void _release(this const auto& self) noexcept {
        auto count = self.ref_count.fetch_sub(1, std::memory_order_acquire) - 1;
        DebugAssert(count >= 0);
        if (count == 0) delete std::addressof(self);
    }
};

/// Reference-counted pointer.
template <std::derived_from<RefBase> T>
class Ref {
    template <std::derived_from<RefBase> U>
    friend class Ref;

    T* ptr = nullptr;

public:
    /// Default constructor.
    explicit Ref() = default;

    /// Explicit constructor from nullptr.
    explicit Ref(std::nullptr_t) noexcept {}

    /// Constructor from an object.
    Ref(T* ptr) noexcept : ptr{ptr} { retain(); }

    /// Copy constructor.
    Ref(const Ref& other) noexcept : ptr{other.ptr} { retain(); }

    /// Move constructor.
    Ref(Ref&& other) noexcept : ptr{std::exchange(other.ptr, nullptr)} {}

    /// Converting constructor.
    template <std::derived_from<RefBase> U>
    requires std::convertible_to<U*, T*>
    Ref(Ref<U> other) noexcept : ptr{std::exchange(other.ptr, nullptr)} {}

    /// Destructor.
    ~Ref() noexcept { release(); }

    /// Assignment.
    Ref& operator=(Ref other) noexcept {
        swap(*this, other);
        return *this;
    }

    /// Access to the underlying pointer.
    [[nodiscard]] T& operator*() const noexcept { return *ptr; }
    [[nodiscard]] T* operator->() const noexcept { return ptr; }
    [[nodiscard]] T* get() const noexcept { return ptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr; }

    /// Factory function.
    template <typename... Args>
    [[nodiscard]] static auto Create(Args&&... args) -> Ref {
        return Ref(new T(std::forward<Args>(args)...));
    }

    /// Swap to Refs.
    friend void swap(Ref& a, Ref& b) noexcept {
        std::swap(a.ptr, b.ptr);
    }

private:
    void retain() {
        if (ptr) ptr->_retain();
    }

    void release() {
        if (ptr) ptr->_release();
    }
};

template <typename A, typename B>
[[nodiscard]] bool operator==(const Ref<A>& a, const Ref<B>& b) noexcept {
    return a.get() == b.get();
}

template <typename A, std::derived_from<RefBase> B>
[[nodiscard]] bool operator==(const Ref<A>& a, B* b) noexcept {
    return a.get() == b;
}

template <typename A>
[[nodiscard]] bool operator==(const Ref<A>& a, std::nullptr_t) noexcept {
    return not a;
}
} // namespace base

#endif // LIBBASE_REF_HH
