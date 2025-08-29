#ifndef TESTCOMMON_HH
#define TESTCOMMON_HH

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <base/Base.hh>

using Catch::Matchers::ContainsSubstring;

// Like string view, but escape chars are printed escaped.
struct RawString {
    std::string str;
    constexpr auto operator<=>(const RawString&) const = default;
};

template <typename Ty>
struct Catch::StringMaker<base::Result<Ty>> {
    static std::string convert(const base::Result<Ty>& res) {
        if (not res) return std::format("<Error> {}", res.error());
        return std::format("{}", res.value());
    }
};

template <>
struct Catch::StringMaker<RawString> {
    static std::string convert(const RawString& res) {
        std::string s;
        for (auto c : res.str) {
            if (c < 32) {
                s += std::format("\\x{:02x}", c);
            } else {
                s += c;
            }
        }
        return s;
    }
};

template <std::formattable<char> T>
struct Catch::StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& res) {
        if (not res.has_value()) return "nullopt";
        return std::format("some({})", res.value());
    }
};

inline auto operator""_raw(const char* str, size_t len) -> RawString {
    return {std::string{str, len}};
}

#endif //TESTCOMMON_HH
