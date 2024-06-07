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

    // Allow construction from char buffer.
    constexpr InputView(const CharBuffer auto& buf)
        : std::span<const std::byte>(
              reinterpret_cast<const std::byte*>(buf.data()),
              buf.size()
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

/// Do NOT use this as a bitmask! Only ever supply at most one of
/// these. The values of the enumerators are an implementation detail.
enum struct OpenMode : u8 {
    /// Open the file for reading. The file must exist.
    Read = 1,

    /// Open the file for writing. Truncates the file if it
    /// exists, creates it if it does not.
    Write = 2,

    /// Same as Write, but does not perform truncation.
    Append = 4,

    /// Open the file for reading and writing.
    ReadWrite = Read | Write,

    /// Open the file for reading and appending.
    ReadAppend = Read | Write | Append,
};
} // namespace base

#endif // LIBBASE_FS_COMMON_HH_
