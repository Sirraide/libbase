#ifndef LIBBASE_PROCESS_HH
#define LIBBASE_PROCESS_HH

#include <base/Chrono.hh>
#include <base/FS.hh>
#include <base/Result.hh>
#include <base/Stream.hh>
#include <error.h>
#include <functional>
#include <memory>
#include <utility>

#ifndef __unix__
#    error Sorry, Process.hh is currently only implemented on Linux/Mac.
#endif

namespace base {
class Process;

using ExitCode = int;

/// Represents the result of a process after it has exited.
class [[nodiscard]] ProcessResult {
    friend Process;

    enum struct Status : u8 {
        Exited,
        Signalled,
        SpawnFailed,
        WaitFailed,
        TimedOut,
    };

    // Platform-specific.
    int data1;
    int data2;

    // Result of execution.
    Status status;

    // Content of output streams.
    std::string out, err;

    ProcessResult() = default;

public:
    /// Get the contents of the error stream.
    [[nodiscard]] auto error_stream() const noexcept -> std::string_view { return err; }

    /// Get the contents of the output stream.
    [[nodiscard]] auto output_stream() const noexcept -> std::string_view { return out; }

    /// Whether the process exited successfully.
    [[nodiscard]] bool success() const noexcept;

    /// Convert this to a Result<>.
    ///
    /// This loses some information, e.g. whether the process
    /// was terminated by a signal.
    [[nodiscard]] auto to_result() const noexcept -> Result<>;

    /// Tests whether the process exited successfully.
    explicit operator bool() const noexcept { return success(); }
};

/// A class representing a spawned process.
///
/// The destructor will kill the process by default.
class [[nodiscard]] Process {
public:
    class Args;
    class Builder;

private:
    // This is a highly platform-specific class, and spawning
    // a process is expensive enough to where the overhead of
    // moving the actual implementation to the heap shouldn’t
    // really be an issue.
    struct Impl;
    Impl* impl;

    explicit Process(Impl* impl) noexcept : impl(impl) {}

public:
    Process(const Process&) = delete;
    Process(Process&& other) noexcept : impl(std::exchange(other.impl, nullptr)) {}
    auto operator=(const Process&) -> Process& = delete;
    auto operator=(Process&& other) noexcept -> Process&;
    ~Process();

    /// A stream provided by a process.
    enum struct Stream : i8 {
        /// Output: Discard the contents of this stream.
        /// Input: Close the stream.
        Null = -1,

        /// Output: Capture the contents of this stream.
        /// Input: Allow writing to it from this process.
        Capture = -2,

        /// Redirect to the process’s input stream.
        Stdin = STDIN_FILENO,

        /// Redirect to the process’s output stream.
        Stdout = STDOUT_FILENO,

        /// Redirect to the process’s error stream.
        Stderr = STDERR_FILENO,
    };

    /// Arguments to a process.
    class Args {
        friend Builder;
        std::vector<std::string> args;

    public:
        Args() = default;
        Args(std::vector<std::string> data): args(std::move(data)) {}
        Args(std::string_view arg) { add(arg); }
        Args(std::initializer_list<std::string_view> data) {
            for (auto arg : data)
                add(arg);
        }

        template <std::ranges::range Range>
        Args(Range r) { for (auto arg : r) add(arg); }

        void add(std::string_view arg) { args.emplace_back(arg); }
    };

    /// A builder for creating processes.
    class Builder {
        File::PathRef exe;
        Args args;
        std::array<Stream, 3> redirects = {
            Stream::Stdin,
            Stream::Stdout,
            Stream::Stderr,
        };

    public:
        Builder(File::PathRef exe, Args args) : exe(exe), args(std::move(args)) {}

        /// Add an argument.
        auto arg(std::string_view arg) noexcept -> Builder& {
            args.add(arg);
            return *this;
        }

        /// Capture the process’s stream.
        auto capture(Stream s) noexcept -> Builder& {
            return redirect(s, Stream::Capture);
        }

        /// Redirect the process’s stream.
        auto redirect(Stream from, Stream to) noexcept -> Builder&;

        /// Spawn a process that executes in the background.
        auto spawn_async() noexcept -> Result<Process>;

        /// Spawn a process and wait until it has exited.
        auto spawn_blocking() noexcept -> ProcessResult;

    private:
        auto SpawnImpl() noexcept -> std::expected<Process, int>;
    };

    /// Get the current polling interval.
    [[nodiscard]] auto polling_interval() const noexcept -> chr::microseconds;

    /// Print to the file.
    template <typename... Args>
    auto print(std::format_string<Args...> fmt, Args&&... args) noexcept -> Result<> {
        return write(std::format(fmt, std::forward<Args>(args)...));
    }

    /// Read from the process.
    ///
    /// \param into The buffer to read into. This will try to read
    ///        `into.size()` bytes from the file.
    /// \param s The stream to read from. This must be a stream that
    ///        was captured when spawning the process and cannot be
    ///        stdin.
    /// \param timeout How long to wait before erroring; set to zero
    ///        to wait indefinitely.
    /// \param stop_if A function that will be called with the data
    ///        read so far and the position at which new data starts;
    ///        if it returns true, we stop reading.
    ///
    /// \return A `Result` containing the number of bytes read, or an
    ///         error if the process could not be read from. A short read
    ///         is not an error and can happen if the file is smaller
    ///         than the buffer.
    auto read(
        OutputView into,
        Stream s = Stream::Stdout,
        chr::milliseconds timeout = 0ms,
        std::function<bool(base::stream, usz)> stop_if = [](auto, auto) { return false; }
    ) noexcept -> Result<usz>;

    /// Set the polling interval.
    ///
    /// This controls how long we try to sleep between attempts to read
    /// or write to the process in non-blocking mode. A value of 0 will
    /// effectively cause the CPU to spin, which is not recommended as
    /// it is likely to confuse the scheduler.
    void set_polling_interval(chr::microseconds interval) noexcept;

    /// Wait until the process has finished executing.
    auto wait(chr::milliseconds timeout = 0ms) noexcept -> ProcessResult;

    /// Write to the file.
    auto write(InputView data) noexcept -> Result<>;

    /// Write to the file using scatter/gather I/O.
    auto writev(InputVector data) noexcept -> Result<>;

    /// Spawn a process that executes in the background.
    static auto SpawnAsync(File::PathRef exe, Args args) noexcept -> Result<Process> {
        return Builder(exe, std::move(args)).spawn_async();
    }

    /// Spawn a process and wait until it has exited.
    static auto SpawnBlocking(File::PathRef exe, Args args) noexcept -> ProcessResult {
        return Builder(exe, std::move(args)).spawn_blocking();
    }
};
} // namespace base

#endif // LIBBASE_PROCESS_HH
