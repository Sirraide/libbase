#include <base/Base.hh>
#include <base/Colours.hh>
#include <print>

using namespace base;

// ============================================================================
//  Text Formatting
// ============================================================================
struct Context {
    struct Style {
        u8 fg_colour{};        // 0 means unset.
        u8 underline_colour{}; // 0 means inherit fg colour.
        bool bold{};
        bool underline{};

        constexpr auto operator<=>(const Style&) const = default;

        // This style subsumes another style, if applying the other style after
        // this one with no reset would have no effect; that is, for each setting,
        // either the other style doesn’t modify it, or it modifies it in the same
        // manner as this style.
        //
        // (If Walter E. Brown somehow ends up reading this, I’m candidly still not
        // sure how useful it would be in the long run, but I can see now why you want
        // an implication operator in C++...)
        bool subsumes(Style other) {
            return (other.fg_colour == 0 or fg_colour == other.fg_colour) and
                   (other.underline_colour == 0 or underline_colour == other.underline_colour) and
                   (not other.bold or bold) and
                   (not other.underline or underline);
        }
    };

    std::vector<Style> styles;
    std::string_view fmt;
    std::string out;
    str s;
    bool use_colours;

    Context(std::string_view fmt, bool use_colours) : fmt(fmt), s(fmt), use_colours(use_colours) {
        styles.emplace_back();
    }

    void ApplyStyle(Style prev, Style s) {
        // If the previous style is equal to the current one, do nothing; since
        // we already combine a style with its parent style when we parse them,
        // this takes care of eliminating nested styles that don’t change anything,
        // e.g. '%b1(%1(abc%)y%)' is parsed as '%b1(%b1(abc%)y%)' and output as though
        // it were '%b1(abcy%)'
        if (prev == s) return;

        // Otherwise, reset if the new style doesn’t subsume the previous one.
        auto subsumes = s.subsumes(prev);
        if (not subsumes) {
            out += "\033[m";
            if (s == Style{}) return;
        }

        // Otherwise, combine all formatting codes that need to change
        // into a single directive.
        out += "\033[";

        // If the new style subsumes the old one, only include the differences.
        if (subsumes) {
            if (s.bold and not prev.bold) out += "1;";
            if (s.fg_colour != prev.fg_colour) out += "3" + std::to_string(s.fg_colour) + ";";
            if (s.underline and not prev.underline) out += "4:3;";
            if (s.underline_colour != prev.underline_colour) out += "58:5:" + std::to_string(s.underline_colour) + ";";
        }

        // Otherwise, include everything.
        else {
            if (s.bold) out += "1;";
            if (s.fg_colour) out += "3" + std::to_string(s.fg_colour) + ";";
            if (s.underline) out += "4:3;";
            if (s.underline_colour) out += "58:5:" + std::to_string(s.underline_colour) + ";";
        }

        out.back() = 'm';
    }

    // Don’t bother checking the string length in this function at all; the
    // [] operator returns an optional, so just crash on the empty optional
    // if the format string is somehow invalid.
    auto ParseFormat(Style style) -> Style {
        // Handle resetting.
        if (s[0] == 'r') {
            style = {};
            s.drop();
        }

        while (not s.consume('(')) {
            switch (auto c = s.take()[0]) {
                default: utils::ThrowOrAbort(std::format("Invalid formatting character in '{}': '{}'", fmt, c));
                case 'b':
                    style.bold = true;
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    style.fg_colour = u8(c - '0');
                    break;

                case 'u':
                    style.underline = true;
                    if ("123456789"sv.contains(s[0])) {
                        style.underline_colour = u8(s[0] - '0');
                        s.drop();
                    }
                    break;
            }
        }

        return style;
    }

    void Render() {
        for (;;) {
            // Append everything we have so far.
            out += s.take_until('%');

            // Stop if we don’t have any formatting codes anymore.
            if (s.empty()) break;

            // Drop the per cent sign.
            s.drop();
            if (s.empty()) utils::ThrowOrAbort("Missing formatting character after '%'");

            // Handle special chars.
            switch (s.front().value()) {
                // Literal per cent sign.
                case '%': {
                    s.drop();
                    out += '%';
                    continue;
                }

                // Closing parenthesis. Pop the last style.
                case ')': {
                    s.drop();

                    // The bottommost style is empty; never pop it. Treat
                    // this as a literal '%)'.
                    if (styles.size() == 1) {
                        out += "%)";
                        break;
                    }

                    // Optimisation: collapse adjacent closing groups so
                    // we don’t reset to the previous style if there is
                    // nothing left to append; e.g. in '%1(%b(e))', we
                    // can emit '\033[31m\033[1me\033[m' instead of
                    // '\033[31m\033[1me\033[m\033[31m\033[m'.
                    //
                    // The optimisation below also lets us compress the
                    // two adjacent formatting codes at the start.
                    auto curr = styles.back();
                    do styles.pop_back();
                    while (styles.size() > 1 and s.consume("%)"));
                    if (use_colours) ApplyStyle(curr, styles.back());
                } break;

                // Per cent sign; parse the formatting codes. Also compress
                // adjacent opening groups into one. While hopefully no-one
                // is going to write those, they can still result from string
                // concatenation; e.g. compress '\033[1m\033[31m' into
                // '\033[1;31m'.
                default:
                    auto curr = styles.back();
                    for (;;) {
                        styles.push_back(ParseFormat(styles.back()));
                        if (s.starts_with('%') and not s.starts_with("%)") and not s.starts_with("%%")) s.drop();
                        else break;
                    }
                    if (use_colours) ApplyStyle(curr, styles.back());
                    break;
            }
        }

        if (styles.size() != 1) utils::ThrowOrAbort(
            std::format("Unterminated formatting sequence in '{}'", fmt)
        );
    }
};

/// Render colours in a string.
auto text::RenderColours(bool use_colours, std::string_view fmt) -> std::string {
    Context ctx{fmt, use_colours};
    ctx.Render();
    return std::move(ctx.out);
}
