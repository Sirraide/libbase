#ifndef TESTCOMMON_HH
#define TESTCOMMON_HH

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <base/Result.hh>

template <typename Ty>
struct Catch::StringMaker<base::Result<Ty>> {
    static std::string convert(const base::Result<Ty>& res) {
        if (not res) return std::format("<Error> {}", res.error());
        return std::format("{}", res.value());
    }
};

#endif //TESTCOMMON_HH
