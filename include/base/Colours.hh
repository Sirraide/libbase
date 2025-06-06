#ifndef LIBBASE_COLOURS_HH
#define LIBBASE_COLOURS_HH

#include <format>
#include <print>
#include <string>

namespace base::text {
/// Mixin to simplify formatting strings with colours.
///
/// A formatting code starts with a per cent sign, and the text to
/// be formatted is enclosed in parentheses; the format it reset to
/// what it was before after the closing parenthesis.
///
/// Example: format("%b({} %1({}%) {}%)", 1, 2, 3%) outputs "1 2 3", with
/// the '1' and '3' formatted in bold, and the '2' in red and bold.
///
/// Formatting codes can also be combined, e.g. "%b1(...%)" is bold and red.
///
/// Supported formatting codes:
/// - %b: bold.
/// - %1-%9: ANSI terminal colours (e.g. %4 becomes \033[34m).
/// - %r: reset everything. This must be specified before any other formatting codes.
/// - %u: curly underline; if the next character is a digit, it becomes the underline colour.
/// - %%: literal %.
class ColourFormatter;

/// Render colours in a string.
[[nodiscard]] auto RenderColours(bool use_colours, std::string_view fmt) -> std::string;
} // namespace base::text

class base::text::ColourFormatter {
public:
    template <typename... Args>
    auto format(
        this auto&& self,
        std::format_string<Args...> fmt,
        Args&&... args
    ) -> std::string {
        return text::RenderColours(
            self.use_colour(),
            std::format(fmt, std::forward<Args>(args)...)
        );
    }

    template <typename... Args>
    void print(
        this auto&& self,
        std::format_string<Args...> fmt,
        Args&&... args
    ) {
        std::print(stdout, "{}", self.format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void print(
        this auto&& self,
        FILE* f,
        std::format_string<Args...> fmt,
        Args&&... args
    ) {
        std::print(f, "{}", self.format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void write(
        this auto&& self,
        std::string& out,
        std::format_string<Args...> fmt,
        Args&&... args
    ) {
        out += self.format(fmt, std::forward<Args>(args)...);
    }

    template <typename OutputStream, typename... Args>
    requires requires (OutputStream& os, std::string s) { os << s; }
    void write(
        this auto&& self,
        OutputStream& os,
        std::format_string<Args...> fmt,
        Args&&... args
    ) {
        os << self.format(fmt, std::forward<Args>(args)...);
    }
};

#endif
