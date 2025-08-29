#ifndef LIBBASE_SUPPRESS_STREAM_HH_DEPRECATION_WARNING
#    warning <base/Stream.hh> is deprecated; include <base/Str.hh> instead and use 'str' instead of 'stream'
#endif

#ifndef LIBBASE_STREAM_HH
#define LIBBASE_STREAM_HH

#include <base/Str.hh>

#define LIBBASE_STREAM_STRING_LITERAL(lit) [] [[deprecated("Macro LIBBASE_STREAM_STRING_LITERAL is deprecated")]] { \
    if constexpr (std::is_same_v<char_type, char>)          \
        return lit;                                         \
    else if constexpr (std::is_same_v<char_type, wchar_t>)  \
        return L"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char8_t>)  \
        return u8"" lit;                                    \
    else if constexpr (std::is_same_v<char_type, char16_t>) \
        return u"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char32_t>) \
        return U"" lit;                                     \
    else                                                    \
        static_assert(false, "Unsupported character type"); \
}()

namespace base {
using stream [[deprecated("Use str instead")]] = str;
using wstream [[deprecated("Use basic_str<wchar_t> instead")]] = basic_str<wchar_t>;
using u8stream [[deprecated("Use str8 instead")]] = str8;
using u16stream [[deprecated("Use str16 instead")]] = str16;
using u32stream [[deprecated("Use str32 instead")]] = str32;
} // namespace base

#endif
