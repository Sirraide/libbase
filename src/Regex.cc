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
        }                                                                                                                                 \
        auto GetCaptureByIndex##width(void* match_data, usz idx)->std::optional<regex_match> {                                            \
            auto data = static_cast<pcre2_match_data_##width*>(match_data);                                                               \
            auto ov = pcre2_get_ovector_pointer_##width(data);                                                                            \
            auto n = pcre2_get_ovector_count_##width(data);                                                                               \
            if (idx >= n) return std::nullopt;                                                                                            \
            auto start = ov[2 * idx];                                                                                                     \
            auto end = ov[2 * idx + 1];                                                                                                   \
            return regex_match{usz(start), usz(end)};                                                                                     \
        }                                                                                                                                 \
                                                                                                                                          \
        auto GetCaptureByName##width(void* code, void* match_data, utils::basic_zstring<char_type> name)->std::optional<regex_match> {    \
            auto data = static_cast<pcre2_match_data_##width*>(match_data);                                                               \
            auto ov = pcre2_get_ovector_pointer_##width(data);                                                                            \
            auto num = pcre2_substring_number_from_name_##width(                                                                          \
                static_cast<pcre2_code_##width*>(code),                                                                                   \
                reinterpret_cast<PCRE2_SPTR##width>(name.data())                                                                          \
            );                                                                                                                            \
            if (num < 0) return std::nullopt;                                                                                             \
            auto start = ov[2 * num];                                                                                                     \
            auto end = ov[2 * num + 1];                                                                                                   \
            return regex_match{usz(start), usz(end)};                                                                                     \
        }

namespace {
IMPL(char, 8);
IMPL(char32_t, 32);
}

#    define DISPATCH(func, ...)                                                                \
        [&] {                                                                                  \
            if constexpr (utils::is<char_type, char>) return ::func##8(__VA_ARGS__);           \
            else if constexpr (utils::is<char_type, char32_t>) return ::func##32(__VA_ARGS__); \
            else static_assert(false, "Unsupported character type");                           \
        }()

template <typename CharType>
basic_regex<CharType>::basic_regex(text_type pattern, regex_flags flags)
    : basic_regex(create(pattern, flags).value()) {}

template <typename CharType>
auto basic_regex<CharType>::create(text_type pattern, regex_flags flags) -> Result<basic_regex> {
    return basic_regex(Try(DISPATCH(Create, pattern, flags)));
}

template <typename CharType>
basic_regex<CharType>::~basic_regex() noexcept {
    DISPATCH(Delete, code, match_data);
}

template <typename CharType>
auto basic_regex<CharType>::match(text_type text) noexcept -> bool {
    return DISPATCH(Match, code, match_data, text);
}

template <typename CharType>
auto basic_regex<CharType>::find(text_type text) noexcept -> std::optional<regex_match> {
    return DISPATCH(Find, code, match_data, text);
}

template <typename CharType>
auto basic_regex<CharType>::operator[](std::size_t capture_index) -> std::optional<regex_match> {
    return DISPATCH(GetCaptureByIndex, match_data, capture_index);
}

template <typename CharType>
auto basic_regex<CharType>::operator[](utils::basic_zstring<CharType> capture_name) -> std::optional<regex_match> {
    return DISPATCH(GetCaptureByName, code, match_data, capture_name);
}

namespace base {
template class basic_regex<char>;
template class basic_regex<char32_t>;
}

#endif // LIBBASE_ENABLE_PCRE2
