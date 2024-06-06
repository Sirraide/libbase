#ifndef LIBBASE_FS_COMMON_HH_
#define LIBBASE_FS_COMMON_HH_

#include <base/Numeric.hh>

namespace base {
template <typename Ty>
concept CharBuffer = requires (Ty t, usz sz) {
    { t.data() } -> std::same_as<char*>;
    { t.size() } -> std::integral;
    { t.resize(sz) };
};

struct InputView : std::span<const std::byte> {
    using std::span<const std::byte>::span;
    using std::span<const std::byte>::operator=;

    // Allow construction from string view.
    constexpr InputView(std::string_view sv)
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(sv.data()),
              sv.size()
          ) {}

    // Allow wrapping a compile-time constant char array.
    template <usz N>
    constexpr InputView(const char (&arr)[N])
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(arr),
              arr[N - 1] == '\0' ? N - 1 : N
          ) {}
};

struct OutputView : std::span<std::byte> {
    using std::span<std::byte>::span;
    using std::span<std::byte>::operator=;

    // Allow construction from char range.
    constexpr OutputView(std::span<char> buf)
        : std::span<std::byte>(
              reinterpret_cast<std::byte*>(buf.data()),
              buf.size()
          ) {}

    // Allow construction from char buffer.
    constexpr OutputView(CharBuffer auto& buf)
        : std::span<std::byte>(
              reinterpret_cast<std::byte*>(buf.data()),
              buf.size()
          ) {}
};

using InputVector = std::span<const InputView>;

enum struct OpenFlags : u8 {
    /// Create the file if it doesn’t exist.
    Create = 0b1,

    /// Open the file for reading.
    Read = 0b10,

    /// Open the file for writing. Implies 'Create'.
    Write = 0b100,

    /// Open the file for appending. Implies 'Create'.
    Append = 0b1000,

    /// Open the file for reading and writing.
    ReadWrite = Read | Write,

    /// Open the file for reading and appending.
    ReadAppend = Read | Append,
};

LIBBASE_DEFINE_FLAG_ENUM(OpenFlags);
} // namespace base

#endif // LIBBASE_FS_COMMON_HH_
