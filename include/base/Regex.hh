#ifndef BASE_REGEX_HH
#define BASE_REGEX_HH

#ifdef LIBBASE_ENABLE_PCRE2

#    include <base/Assert.hh>
#    include <base/Numeric.hh>
#    include <base/Result.hh>
#    include <base/Utils.hh>
#    include <string_view>

namespace base {
enum struct regex_flags : u64 {
    none,
    jit = 1 << 0,           // JIT-compile if possible.
    dotall = 1 << 5,        // '.' also matches newlines.
    anchored = u64(1) << 31 // Match only at the start of the input.
};

LIBBASE_DEFINE_FLAG_ENUM(regex_flags);

struct regex_match {
    usz start{};
    usz end{};

    /// Extract the matched data from the input buffer.
    ///
    /// The buffer passed to this MUST be the EXACT SAME that was passed
    /// to 'regex::find()' or 'regex::match()', otherwise, the behaviour
    /// is undefined.
    template <typename CharType>
    [[nodiscard]] constexpr auto extract(
        std::basic_string_view<CharType> buffer
    ) -> std::basic_string_view<CharType> {
        return buffer.substr(start, size());
    }

    [[nodiscard]] constexpr usz size() const noexcept { return end - start; }
    [[nodiscard]] constexpr auto operator<=>(const regex_match&) const = default;
};

/// A compiled regular expression.
template <typename CharType>
class basic_regex {
    LIBBASE_NO_COPY(basic_regex);

    static_assert(
        utils::is_same<CharType, char, char32_t>,
        "base::basic_regex only supports char and char32_t"
    );

public:
    using char_type = CharType;
    using text_type = std::basic_string_view<char_type>;
    using size_type = std::size_t;

private:
    // Type-erased so we don’t have to expose PCRE2 headers.
    void* code{};
    void* match_data{};

    explicit basic_regex(std::pair<void*, void*> impl) noexcept
        : code(impl.first), match_data(impl.second) {}

public:
    /// Create a new regex from a pattern.
    ///
    /// \throw std::runtime_error if the pattern could not be compiled.
    /// \see create() for a non-throwing factory function.
    basic_regex(
        text_type pattern,
        regex_flags flags = regex_flags::jit | regex_flags::dotall
    );

    /// Move constructor.
    basic_regex(basic_regex&& other) noexcept
        : code(std::exchange(other.code, nullptr)),
          match_data(std::exchange(other.match_data, nullptr)) {}

    /// Move assignment operator.
    auto operator=(basic_regex&& other) noexcept -> basic_regex& {
        std::swap(code, other.code);
        std::swap(match_data, other.match_data);
        return *this;
    }

    /// Destructor.
    ~basic_regex() noexcept;

    /// Create a new regex from a pattern.
    ///
    /// This may fail if the regex doesn’t parse.
    [[nodiscard]] static auto create(
        text_type pattern,
        regex_flags flags = regex_flags::jit | regex_flags::dotall
    ) -> Result<basic_regex>;

    /// Find the first match in a string.
    ///
    /// This function is not 'const' because it reuses internal state to avoid
    /// allocating new match state every time a match is performed.
    [[nodiscard]] auto find(text_type text) noexcept -> std::optional<regex_match>;

    /// Check whether the input matches this regex.
    [[nodiscard]] bool match(text_type text) noexcept;

    /// Access a capture by index.
    ///
    /// Note that, as usual, index 0 is the entire match.
    ///
    /// This may fail if the index is out of bounds. The return value is
    /// unspecified if 'match()' or 'find()' have never been called on this
    /// expression.
    [[nodiscard]] auto operator[](std::size_t capture_index) -> std::optional<regex_match>;

    /// Access a capture by name.
    ///
    /// This may fail if there is no capture with the specified name. The return
    /// value is unspecified if 'match()' or 'find()' have never been called on
    /// this expression.
    [[nodiscard]] auto operator[](utils::basic_zstring<CharType> capture_name) -> std::optional<regex_match>;
};

extern template class basic_regex<char>;
extern template class basic_regex<char32_t>;

using regex = basic_regex<char>;
using u32regex = basic_regex<char32_t>;
} // namespace base

#endif // LIBBASE_ENABLE_PCRE2

#endif // BASE_REGEX_HH
