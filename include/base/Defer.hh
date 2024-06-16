#ifndef LIBBASE_DEFER_HH_
#define LIBBASE_DEFER_HH_

#include <utility>

#define defer ::base::detail::DeferImpl _ = [&]
#define tempset auto _ = ::base::detail::Tempset{}->*

namespace base::detail {
template <typename Callable>
class DeferImpl {
    Callable c;

public:
    DeferImpl(Callable c) : c(std::move(c)) {}
    ~DeferImpl() { c(); }
};

template <typename T, typename U>
class TempsetStage2 {
    T& lvalue;
    T old_value;

public:
    TempsetStage2(T& lvalue, U&& value)
        : lvalue(lvalue),
          old_value(std::exchange(lvalue, std::forward<U>(value))) {}
    ~TempsetStage2() { lvalue = std::move(old_value); }
};

template <typename T>
struct TempsetStage1 {
    T& lvalue;
    auto operator=(auto&& value) {
        return TempsetStage2{lvalue, std::forward<decltype(value)>(value)};
    }
};

struct Tempset {
    auto operator->*(auto& lvalue) {
        return TempsetStage1{lvalue};
    }
};
}

#endif //LIBBASE_DEFER_HH_
