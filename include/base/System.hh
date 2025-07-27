#ifndef LIBBASE_SYSTEM_HH
#define LIBBASE_SYSTEM_HH

#include <string_view>

#ifdef __unix__
#    include <unistd.h>
#endif

namespace base::sys {
#ifdef __unix__
/// Log to a file descriptor in a signal-safe manner.
///
/// \param msg The message to print.
/// \param fd The file to print to; defaults to stderr.
///
/// This is only available on UNIX systems.
void atomic_log(std::string_view msg, int fd = STDERR_FILENO);
#endif

}

#endif // LIBBASE_SYSTEM_HH
