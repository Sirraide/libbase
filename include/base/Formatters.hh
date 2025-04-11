#ifndef LIBBASE_FORMATTERS_HH
#define LIBBASE_FORMATTERS_HH

#include <base/StringUtils.hh>
#include <base/Utils.hh>
#include <base/Size.hh>
#include <format>
#include <vector>

template <typename Inner>
struct std::formatter<std::vector<Inner>> : std::formatter<Inner> {
    template <typename FormatContext>
    auto format(const std::vector<Inner>& vec, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "[{}]", base::utils::join(vec));
    }
};

/// In keeping with the design principles of this class, users must explicitly
/// choose whether to print this as bits or bytes.
///
/// Use 'i' for bits, 'y' for bytes.
template <>
struct std::formatter<base::Size> {
    bool bits = false;
    bool human = false;

    template <class ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto it = ctx.begin();
        if (it == ctx.end() or *it == '}')
            base::utils::ThrowOrAbort("'i' or 'y' is required when formatting a Size");

        if (*it == 'i' or *it == 'y') {
            bits = *it == 'i';
            ++it;
        } else {
            base::utils::ThrowOrAbort("'i' or 'y' must be the first format specifier");
        }

        if (it != ctx.end() and *it == 'h') {
            human = true;
            ++it;
        }

        if (it != ctx.end() and *it != '}')
            base::utils::ThrowOrAbort("Unexpected format specifier");

        return it;
    }

    template <typename FormatContext>
    auto format(base::Size size, FormatContext& ctx) const {
        return not human ? std::format_to(ctx.out(), "{}", std::uint64_t(bits ? size.bits() : size.bytes()))
             : bits      ? std::format_to(ctx.out(), "{}{}", base::utils::HumanReadable(size.bits()), size.bits() == 0 ? "" : "Bit")
                         : std::format_to(ctx.out(), "{}", base::utils::HumanReadable(size.bytes()));
    }
};

template <>
struct std::formatter<base::Align> : std::formatter<base::u64> {
    template <typename FormatContext>
    auto format(base::Align a, FormatContext& ctx) const {
        return std::formatter<base::u64>::format(a.value().bytes(), ctx);
    }
};

#endif // LIBBASE_FORMATTERS_HH
