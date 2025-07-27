#include <base/System.hh>

#ifdef __unix__
void base::sys::atomic_log(std::string_view msg, int fd) {
    write(fd, msg.data(), msg.size());
}
#endif
