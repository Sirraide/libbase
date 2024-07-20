#include <base/Process.hh>

namespace base::detail {
struct ProcessImplCommon {
    chr::microseconds polling_interval = 1ms;
};
}

#ifndef _WIN32
#    include "ProcessUnix.inc"
#else
#    error TODO: Windows support
#endif

Process::~Process() { delete impl; }
auto Process::operator=(Process&& other) noexcept -> Process& {
    delete impl;
    impl = std::exchange(other.impl, nullptr);
    return *this;
}

auto Process::polling_interval() const noexcept -> chr::microseconds {
    return impl->polling_interval;
}

void Process::set_polling_interval(chr::microseconds interval) noexcept {
    impl->polling_interval = interval;
}
