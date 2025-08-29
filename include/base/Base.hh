#ifndef LIBBASE_BASE_HH
#define LIBBASE_BASE_HH

#include <base/Assert.hh>
#include <base/DSA.hh>
#include <base/Formatters.hh>
#include <base/Macros.hh>
#include <base/Numeric.hh>
#include <base/Result.hh>
#include <base/Str.hh>
#include <base/StringUtils.hh>
#include <base/Types.hh>
#include <base/Utils.hh>
#include <chrono>

#define LIBBASE_SUPPRESS_STREAM_HH_DEPRECATION_WARNING
#    include <base/Stream.hh>
#undef LIBBASE_SUPPRESS_STREAM_HH_DEPRECATION_WARNING

namespace base {
using namespace std::literals;
namespace chr = std::chrono;
}

#endif
