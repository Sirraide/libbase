#ifndef LIBBASE_SYSTEM_HH
#define LIBBASE_SYSTEM_HH

#include <string_view>

#ifdef __unix__
#    include <unistd.h>
#endif

namespace base::sys {
#ifdef __unix__
void atomic_log(std::string_view msg, int fd = STDERR_FILENO);
#endif

}

#endif // LIBBASE_SYSTEM_HH
