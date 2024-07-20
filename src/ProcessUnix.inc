#include <base/Assert.hh>
#include <base/Base.hh>
#include <base/Macros.hh>
#include <fcntl.h>
#include <memory>
#include <ranges>
#include <sys/types.h> // MacOS needs this, apparently.
#include <sys/wait.h>
#include <thread>
#include <unicode/urename.h>
#include <unistd.h>
#include <vector>

using namespace base;
using Stream = Process::Stream;

// Sanity checks.
static_assert(Stream::Stdin != Stream::Null);
static_assert(Stream::Stdout != Stream::Null);
static_assert(Stream::Stderr != Stream::Null);
static_assert(Stream::Stdin != Stream::Capture);
static_assert(Stream::Stdout != Stream::Capture);
static_assert(Stream::Stderr != Stream::Capture);
static_assert(Stream::Null != Stream::Capture);

namespace {
struct Pipe {
    LIBBASE_IMMOVABLE(Pipe);
    int pipe[2]{-1, -1};

public:
    explicit Pipe() {}

    void close_read() { ::close(read()); }
    void close_write() { ::close(write()); }

    auto open() -> std::expected<void, int> {
        if (::pipe(pipe) < 0) return std::unexpected{errno};
        return {};
    }

    int read() { return pipe[0]; }

    void read_from(int fd) {
        ::dup2(read(), fd);
        close_write();
    }

    int write() { return pipe[1]; }

    void write_into(int fd) {
        ::dup2(write(), fd);
        close_read();
    }

    ~Pipe() {
        if (pipe[0] != -1) ::close(pipe[0]);
        if (pipe[1] != -1) ::close(pipe[1]);
    }
};

template <typename ...Args>
auto DoFcntl(int fd, int cmd, Args... args) -> Result<int> {
    auto res = ::fcntl(fd, cmd, args...);
    if (res < 0) return Error("fcntl() failed on '{}' with cmd '{}': {}", fd, cmd, strerror(errno));
    return res;
}

// Handles creating pipes etc.
struct IOStream {
    Stream s;
    Stream target{};
    Pipe pipe;
    int error = 0;
    bool blocking = true;
    bool done = true;

    IOStream(Stream s) : s{s} {}

    bool captured() const { return target == Stream::Capture; }

    void child() {
        if (s == target) return;
        if (target == Stream::Null) {
            ::close(+s);
            return;
        }

        if (captured()) {
            if (s == Stream::Stdin) pipe.read_from(STDIN_FILENO);
            else pipe.write_into(+s);
            return;
        }

        dup2(+target, +s);
    }

    auto init(Stream redirect_to) -> std::expected<void, int> {
        target = redirect_to;
        if (captured()) {
            done = false;
            return pipe.open();
        }
        return {};
    }

    void parent() {
        if (not captured()) return;
        if (s == Stream::Stdin) pipe.close_read();
        else pipe.close_write();
    }

    template <typename Callable>
    auto read_once(OutputView into, Callable ShouldRetryIfWouldBlock) -> Result<usz> {
        Assert(captured(), "Reading stream that hasn’t been captured???");
        if (done) return 0;
        for (;;) {
            auto n_read = ::read(pipe.read(), into.data(), into.size());
            if (n_read < 0) {
                if (errno == EINTR) continue;

                // If we’re blocking we may get an error signalling that we
                // have no data to read; try again in that case.
                if (not blocking and (errno == EAGAIN or errno == EWOULDBLOCK)) {
                    if (ShouldRetryIfWouldBlock()) continue;
                    return 0;
                }

                // Otherwise, any error here is a problem.
                return Error("Could not read from stream: {}", strerror(errno));
            }

            // We are done when a *successful* read returns 0.
            if (n_read == 0) done = true;

            // Done reading.
            return usz(n_read);
        }
    }

    auto set_blocking(bool should_block) -> Result<> {
        if (blocking == should_block) return {};
        auto fd = s == Stream::Stdin ? pipe.write() : pipe.read();
        auto flags = Try(DoFcntl(fd, F_GETFL));
        flags = should_block ? flags & ~O_NONBLOCK : flags | O_NONBLOCK;
        Try(DoFcntl(fd, F_SETFL, flags));
        blocking = should_block;
        return {};
    }
};
} // namespace

/// Check if a timer has timed out and wait if it hasn’t.
///
/// @return false if the timeout has expired and we should stop.
bool StepTimer(
    chr::microseconds timeout,
    chr::steady_clock::time_point start,
    chr::microseconds interval
) {
    auto elapsed = chr::steady_clock::now() - start;
    if (elapsed >= timeout) return false;
    std::this_thread::sleep_for(interval);
    return true;
}

struct Process::Impl : detail::ProcessImplCommon {
    std::array<IOStream, 3> streams{Stream::Stdin, Stream::Stdout, Stream::Stderr};
    pid_t pid{};

    auto err() -> IOStream& { return streams[usz(Stream::Stderr)]; }
    auto out() -> IOStream& { return streams[usz(Stream::Stdout)]; }

    bool ReadStreams(ProcessResult& res, chr::milliseconds timeout);
    auto ReadStreamsImpl(ProcessResult& res, chr::milliseconds timeout) -> Result<>;
};

bool Process::Impl::ReadStreams(ProcessResult& res, chr::milliseconds timeout) {
    auto result = ReadStreamsImpl(res, timeout);
    if (not result.has_value()) {
        res.err.clear();
        res.err = std::move(result.error());
        res.status = ProcessResult::Status::WaitFailed;
        return false;
    }

    return true;
}

auto Process::Impl::ReadStreamsImpl(ProcessResult& res, chr::milliseconds timeout) -> Result<> {
    auto ReadOnce = [&](IOStream& s, std::string& to) -> Result<bool> {
        static constexpr usz ReadBlockSz = 4096;
        auto old_size = to.size();
        to.resize(old_size + ReadBlockSz);
        auto n_read = Try(s.read_once(OutputView{to}.skip(old_size), [] { return false; }));
        to.resize(old_size + n_read);
        return n_read != 0;
    };

    // No need to do anything if neither stream was captured.
    auto& e = err();
    auto& o = out();
    if (not e.captured() and not o.captured()) return {};

    // Make both non-blocking.
    if (e.captured()) Try(e.set_blocking(false));
    if (o.captured()) Try(o.set_blocking(false));

    // Poll both streams; if neither has any data, wait a bit before
    // trying again so we don’t spin.
    while (not e.done or not o.done) {
        bool changed = false;
        if (not o.done) changed |= Try(ReadOnce(o, res.out));
        if (not e.done) changed |= Try(ReadOnce(e, res.err));
        if (not StepTimer(timeout))
    }

    // If both streams are done, then we’re done.
    return {};
}


auto Process::Builder::SpawnImpl() noexcept -> std::expected<Process, int> {
    auto impl = std::make_unique<Impl>();

    // Prepare args.
    std::vector<char*> argv;
    argv.reserve(args.args.size() + 2);
    argv.push_back(const_cast<char*>(exe.c_str()));
    for (auto& arg : args.args) argv.push_back(const_cast<char*>(arg.data()));
    argv.push_back(nullptr);

    // Prepare redirects (create pipe etc.)
    for (auto [i, s] : impl->streams | vws::enumerate)
        Try(s.init(redirects[usz(i)]));

    // Fork.
    auto pid = fork();

    // Child.
    if (pid == 0) {
        for (auto& s : impl->streams) s.child();
        ::execvp(exe.c_str(), argv.data());
        std::abort();
    }

    // Parent.
    if (pid < 0) return std::unexpected{errno};
    for (auto& s : impl->streams) s.parent();
    return Process{impl.release()};
}

auto Process::read(
    OutputView into,
    Stream s,
    chr::milliseconds timeout,
    std::function<bool(base::stream, usz)> stop_if
) noexcept -> Result<usz> {
    // Sanity check.
    if (s != Stream::Stdout and s != Stream::Stderr)
        return Error("Cannot read from stream {}", +s);

    // Make the file descriptor (non-)blocking if need be.
    auto& st = impl->streams[usz(+s)];
    if (not st.captured()) return Error("Cannot read from stream {} that hasn’t been captured", +s);
    bool blocking = timeout != 0ms;
    Try(st.set_blocking(blocking));

    // Big read loop.
    usz total_read = 0;
    auto start = chr::steady_clock::now();
    while (total_read != into.size()) {
        auto Retry = [&] { StepTimer(timeout, start, impl->polling_interval); };

        // Dew it.
        auto n_read = Try(st.read_once(into.skip(total_read), Retry));

        // Check if we should stop.
        if (stop_if(into.take(total_read + usz(n_read)).sv(), total_read)) break;
        total_read += usz(n_read);
    }

    return total_read;
}

auto Process::Builder::redirect(Stream from, Stream to) noexcept -> Builder& {
    if (from != Stream::Null and from != Stream::Capture) redirects[usz(from)] = to;
    return *this;
}

auto Process::Builder::spawn_async() noexcept -> Result<Process> {
    auto p = SpawnImpl();
    if (not p) return Error("Spawn failed: {}", strerror(p.error()));
    return std::move(p.value());
}

auto Process::Builder::spawn_blocking() noexcept -> ProcessResult {
    auto p = SpawnImpl();
    if (not p) {
        ProcessResult result{};
        result.data1 = p.error();
        result.status = ProcessResult::Status::SpawnFailed;
        return result;
    }
    return p.value().wait();
}

auto Process::wait(chr::milliseconds timeout) noexcept -> ProcessResult {
    using PR = ProcessResult;
    PR result{};
    int stat{};

    // Loop until the process exits.
    auto start = chr::steady_clock::now();
    auto flags = timeout == 0ms ? 0 : WNOHANG;
    for (;;) {
        if (waitpid(impl->pid, &stat, flags) < 0) {
            if (errno == EAGAIN) {
                if (not StepTimer(timeout, start, impl->polling_interval)) {
                    result.status = PR::Status::TimedOut;
                    return result;
                }

                if (not impl->ReadStreams(result, timeout))
                    return result;
            }

            result.data1 = errno;
            result.status = PR::Status::WaitFailed;
            return result;
        }

        if (WIFEXITED(stat) or WIFSIGNALED(stat)) {
            result.data1 = stat;
            result.status = WIFEXITED(stat) ? PR::Status::Exited : PR::Status::Signalled;
            break;
        }
    }

    // Read captured streams.
    return result;
}

bool ProcessResult::success() const noexcept {
    return status == Status::Exited and data1 == 0;
}

auto ProcessResult::to_result() const noexcept -> Result<> {
    if (success()) return {};
    if (status == Status::SpawnFailed) return Error("Could not spawn process: {}", strerror(data1));
    if (status == Status::WaitFailed) return Error("Could not wait for process: {}", strerror(data1));
    if (status == Status::Signalled) return Error("Process was killed by signal {}", WTERMSIG(data1));
    return Error("Process exited with status {}", WEXITSTATUS(data1));
}