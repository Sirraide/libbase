#ifndef LIBBASE_FORMATTERS_HH
#define LIBBASE_FORMATTERS_HH

#include <format>
#include <vector>
#include <base/Utils.hh>

template <typename Inner>
struct std::formatter<std::vector<Inner>> : std::formatter<Inner> {
    template <typename FormatContext>
    auto format(const std::vector<Inner>& vec, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "[{}]", base::utils::join(vec));
    }
};

#endif // LIBBASE_FORMATTERS_HH
