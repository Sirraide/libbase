#ifndef LIBBASE_THREADING_HH
#define LIBBASE_THREADING_HH

#include <base/Assert.hh>
#include <base/Macros.hh>
#include <condition_variable>
#include <functional>
#include <mutex>

namespace base {
/// Wrapper around std::condition_variable that ensures we can’t call
/// notify_one/notify_all without holding a lock.
///
/// Consider:
///
///   - Thread 1:
///
///     std::unique_lock lock{mtx};
///     if (should_stop.load()) return;
///     // (1)
///     cond.wait(lock, ...);
///
///   - Thread 2:
///
///     // NO lock is held here.
///     should_stop.store(true);
///     cond.notify_one();
///
/// It’s possible for a notification to be missed if it arrives while Thread 1
/// is suspended at the program point (1). If Thread 2 acquiring a lock before
/// it stores 'true' and notifies the condition variable, this race becomes
/// impossible since Thread 1 cannot be at (1) at the same time, because it
/// would be holding the lock, which would prevent the notification.
class ConditionVariable {
    std::condition_variable var;

public:
    void notify_one(const std::unique_lock<std::mutex>& l) {
        Assert(l.owns_lock());
        var.notify_one();
    }

    void notify_all(const std::unique_lock<std::mutex>& l) {
        Assert(l.owns_lock());
        var.notify_one();
    }

    void wait(std::unique_lock<std::mutex>& l, auto&& pred) {
        var.wait(l, LIBBASE_FWD(pred));
    }
};

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
    ConditionVariable cond_var;

public:
    using ThreadSafe<T>::ThreadSafe;

    /// Update the value and notify all listeners.
    void update_all(auto&& cb) {
        std::unique_lock lock{this->mutex};
        std::invoke(LIBBASE_FWD(cb), this->val);
        cond_var.notify_all(lock);
    }

    /// Update the value and notify a single listener.
    void update_one(auto&& cb) {
        std::unique_lock lock{this->mutex};
        std::invoke(LIBBASE_FWD(cb), this->val);
        cond_var.notify_one(lock);
    }

    /// Wait until a condition is true, then run a callback.
    void wait(auto&& predicate, auto&& cb) {
        std::unique_lock lock{this->mutex};
        cond_var.wait(lock, [&] -> bool { return std::invoke(predicate, this->val); });
        std::invoke(LIBBASE_FWD(cb), this->val);
    }
};

/// Thread-safe concurrent queue.
template <typename T>
class ThreadSafeQueue : Notifiable<Queue<T>> {
    std::atomic_flag closed = false;

public:
    ThreadSafeQueue() = default;

    /// Close the queue.
    void close() {
        std::unique_lock lock{this->mutex};
        closed.test_and_set(std::memory_order_release);
        this->cond_var.notify_all(lock);
    }

    /// Add an object to the queue.
    void enqueue(T val) {
        std::unique_lock lock{this->mutex};
        this->val.push(std::move(val));
        this->cond_var.notify_one(lock);
    }

    /// Stream the contents of the queue in a thread-safe manner.
    ///
    /// The iteration returns as soon as the queue is both empty and closed.
    [[nodiscard]] auto stream() -> std::generator<T> {
        for (;;) {
            std::unique_lock lock{this->mutex};
            if (not this->val.empty()) {
                auto v = this->val.dequeue();
                lock.unlock();
                co_yield v;
                continue;
            }

            if (should_stop())
                co_return;

            this->cond_var.wait(lock, [&] {
                return not this->val.empty() or should_stop();
            });
        }
    }

private:
    [[nodiscard]] bool should_stop() const {
        return closed.test(std::memory_order_acquire);
    }
};
} // namespace base

#endif // LIBBASE_THREADING_HH
