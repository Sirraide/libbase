#ifdef LIBBASE_ENABLE_PCRE2

#    include <base/Macros.hh>
#    include <base/Regex.hh>
#    include <base/Text.hh>

#    define PCRE2_CODE_UNIT_WIDTH 0
#    include <pcre2.h>

using namespace base;

#    define IMPL(char_type, width)                                                                                                        \
        auto Create##width(std::basic_string_view<char_type> pattern, regex_flags flags)->Result<std::pair<void*, void*>> {               \
            int err{};                                                                                                                    \
            usz err_offs{};                                                                                                               \
                                                                                                                                          \
            /* Convert flags. */                                                                                                          \
            u32 pcre_flags = PCRE2_UTF;                                                                                                   \
            if (flags & regex_flags::dotall) pcre_flags |= PCRE2_DOTALL;                                                                  \
            if (flags & regex_flags::anchored) pcre_flags |= PCRE2_ANCHORED;                                                              \
                                                                                                                                          \
            /* Compile the expression. */                                                                                                 \
            auto code = pcre2_compile_##width(                                                                                            \
                reinterpret_cast<PCRE2_SPTR##width>(pattern.data()),                                                                      \
                pattern.size(),                                                                                                           \
                pcre_flags,                                                                                                               \
                &err,                                                                                                                     \
                &err_offs,                                                                                                                \
                nullptr                                                                                                                   \
            );                                                                                                                            \
                                                                                                                                          \
            /* Check for errors. */                                                                                                       \
            if (not code) {                                                                                                               \
                std::basic_string<char_type> buffer;                                                                                      \
                buffer.resize(4'096);                                                                                                     \
                auto sz = pcre2_get_error_message_##width(                                                                                \
                    err,                                                                                                                  \
                    reinterpret_cast<PCRE2_UCHAR##width*>(buffer.data()),                                                                 \
                    buffer.size()                                                                                                         \
                );                                                                                                                        \
                                                                                                                                          \
                if (sz == PCRE2_ERROR_BADDATA) return Error("Regex error: PCRE error code is invalid");                                   \
                if (sz == PCRE2_ERROR_NOMEMORY) return Error("Regex error: PCRE error buffer is too small to accommodate error message"); \
                buffer.resize(usz(sz));                                                                                                   \
                return Error("Regex error: in expression '{}': {}", pattern, buffer);                                                     \
            }                                                                                                                             \
                                                                                                                                          \
            /* JIT-compile it if possible. */                                                                                             \
            if (flags & regex_flags::jit)                                                                                                 \
                pcre2_jit_compile_##width(code, PCRE2_JIT_COMPLETE);                                                                      \
                                                                                                                                          \
            /* Allocate the match data. */                                                                                                \
            auto match_data = pcre2_match_data_create_from_pattern_##width(code, nullptr);                                                \
            return std::pair<void*, void*>{code, match_data};                                                                             \
        }                                                                                                                                 \
                                                                                                                                          \
        void Delete##width(void* code, void* match_data) noexcept {                                                                       \
            pcre2_code_free_##width(static_cast<pcre2_code_##width*>(code));                                                              \
            pcre2_match_data_free_##width(static_cast<pcre2_match_data_##width*>(match_data));                                            \
        }                                                                                                                                 \
                                                                                                                                          \
        bool Match##width(void* code, void* match_data, std::basic_string_view<char_type> text) noexcept {                                \
            auto re = static_cast<pcre2_code_##width*>(code);                                                                             \
            auto data = static_cast<pcre2_match_data_##width*>(match_data);                                                               \
            int res = pcre2_match_##width(                                                                                                \
                re,                                                                                                                       \
                reinterpret_cast<PCRE2_SPTR##width>(text.data()),                                                                         \
                text.size(),                                                                                                              \
                0,                                                                                                                        \
                0,                                                                                                                        \
                data,                                                                                                                     \
                nullptr                                                                                                                   \
            );                                                                                                                            \
                                                                                                                                          \
            return res >= 0;                                                                                                              \
        }                                                                                                                                 \
                                                                                                                                          \
        auto Find##width(void* code, void* match_data, std::basic_string_view<char_type> text) noexcept -> std::optional<regex_match> {   \
            if (not Match##width(code, match_data, text)) return std::nullopt;                                                            \
            auto v = pcre2_get_ovector_pointer_##width(static_cast<pcre2_match_data_##width*>(match_data));                               \
            return regex_match{usz(v[0]), usz(v[1])};                                                                                     \
        }

namespace {
IMPL(char, 8);
IMPL(char32_t, 32);
}

template <typename CharType>
basic_regex<CharType>::basic_regex(text_type pattern, regex_flags flags)
    : basic_regex(create(pattern, flags).value()) {}

template <typename CharType>
auto basic_regex<CharType>::create(text_type pattern, regex_flags flags) -> Result<basic_regex> {
    if constexpr (utils::is<char_type, char>) return basic_regex{Try(Create8(pattern, flags))};
    else if constexpr (utils::is<char_type, char32_t>) return basic_regex{Try(Create32(pattern, flags))};
    else static_assert(false, "Unsupported character type");
}

template <typename CharType>
basic_regex<CharType>::~basic_regex() noexcept {
    if constexpr (utils::is<char_type, char>) Delete8(code, match_data);
    else if constexpr (utils::is<char_type, char32_t>) Delete32(code, match_data);
    else static_assert(false, "Unsupported character type");
}

template <typename CharType>
auto basic_regex<CharType>::match(text_type text) noexcept -> bool {
    if constexpr (utils::is<char_type, char>) return ::Match8(code, match_data, text);
    else if constexpr (utils::is<char_type, char32_t>) return ::Match32(code, match_data, text);
    else static_assert(false, "Unsupported character type");
}

template <typename CharType>
auto basic_regex<CharType>::find(text_type text) noexcept -> std::optional<regex_match> {
    if constexpr (utils::is<char_type, char>) return ::Find8(code, match_data, text);
    else if constexpr (utils::is<char_type, char32_t>) return ::Find32(code, match_data, text);
    else static_assert(false, "Unsupported character type");
}

namespace base {
template class basic_regex<char>;
template class basic_regex<char32_t>;
}

#endif // LIBBASE_ENABLE_PCRE2
