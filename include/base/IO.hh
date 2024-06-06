#ifndef LIBBASE_IO_HH_
#define LIBBASE_IO_HH_

#include <format>
#include <print>
#include <string>
#include <vector>

template <typename Inner>
struct std::formatter<std::vector<Inner>> : std::formatter<Inner> {
    template <typename FormatContext>
    auto format(const std::vector<Inner>& vec, FormatContext& ctx) const {
        ctx.out() = "["sv;
        bool first = true;
        for (const auto& elem : vec) {
            if (first) first = false;
            else ctx.out() = ", "sv;
            ctx.out() = std::format("{}", elem);
        }
        ctx.out() = "]"sv;
        return ctx.out();
    }
};

#endif // LIBBASE_IO_HH_
