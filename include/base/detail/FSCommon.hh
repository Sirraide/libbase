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

enum struct OpenFlags : u8 {
    /// Create the file if it doesn’t exist.
    Create = 0b1,

    /// Open the file for reading.
    Read = 0b10,

    /// Open the file for writing. Implies 'Create'.
    Write = 0b100,

    /// Open the file for appending. Implies 'Create'.
    Append = 0b1000,

    /// Open the file for reading and writing. Implies 'Create.'
    ReadWrite = Read | Write,

    /// Open the file for reading and appending. Implies 'Create.'
    ReadAppend = Read | Append,
};

LIBBASE_DEFINE_FLAG_ENUM(OpenFlags);
}

#endif // LIBBASE_FS_COMMON_HH_
