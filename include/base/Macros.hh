#ifndef MACROS_HH
#define MACROS_HH

#define LIBBASE_STR_(X) #X
#define LIBBASE_STR(X)  LIBBASE_STR_(X)

#define LIBBASE_CAT_(X, Y) X##Y
#define LIBBASE_CAT(X, Y)  LIBBASE_CAT_(X, Y)

#define LIBBASE_IMMOVABLE(cls)           \
    cls(const cls&) = delete;            \
    cls& operator=(const cls&) = delete; \
    cls(cls&&) = delete;                 \
    cls& operator=(cls&&) = delete

#ifndef NDEBUG
#    define LIBBASE_DEBUG(...) __VA_ARGS__
#else
#    define LIBBASE_DEBUG(...)
#endif

#endif //MACROS_HH
