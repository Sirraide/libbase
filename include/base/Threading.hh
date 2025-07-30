#ifndef LIBBASE_THREADING_HH
#define LIBBASE_THREADING_HH

#include <base/Macros.hh>
#include <condition_variable>
#include <functional>
#include <mutex>

namespace base {
/// Class that wraps an object and allows only thread-safe access to it.
template <typename T>
class ThreadSafe {
    static_assert(
        std::is_same_v<std::remove_cvref_t<T>, T>,
        "ThreadSafe type must be a non-cv-qualified non-reference type"
    );

    LIBBASE_IMMOVABLE(ThreadSafe);

protected:
    std::mutex mutex;
    T val{};

public:
    /// Create a new default-initialised object.
    explicit ThreadSafe()
    requires std::is_default_constructible_v<T>
    {}

    /// Move-construct an object.
    explicit ThreadSafe(T val)
    requires std::is_move_constructible_v<T>
        : val(std::move(val)) {}

    /// Create a new object from a list of arguments.
    template <typename... Args>
    requires std::is_constructible_v<T, Args...>
    explicit ThreadSafe(Args&&... args) : val(LIBBASE_FWD(args)...) {}

    /// Run a callback on the wrapped value.
    auto with(auto&& cb) {
        std::unique_lock _{mutex};
        return std::invoke(LIBBASE_FWD(cb), val);
    }
};

/// Thread-safe object that comes with a condition variable.
template <typename T>
class Notifiable : protected ThreadSafe<T> {
    std::condition_variable cond_var;

public:
    using ThreadSafe<T>::ThreadSafe;

    /// Update the value and notify all listeners.
    void update_all(auto&& cb) {
        this->with(LIBBASE_FWD(cb));
        cond_var.notify_all();
    }

    /// Update the value and notify a single listener.
    void update_one(auto&& cb) {
        this->with(LIBBASE_FWD(cb));
        cond_var.notify_one();
    }

    /// Wait until a condition is true, then run a callback.
    void wait(auto&& predicate, auto&& cb) {
        std::unique_lock lock{this->mutex};
        cond_var.wait(lock, [&] -> bool { return std::invoke(predicate, this->val); });
        std::invoke(LIBBASE_FWD(cb), this->val); // Do not use with() as that would cause a deadlock.
    }
};
} // namespace base

#endif // LIBBASE_THREADING_HH
