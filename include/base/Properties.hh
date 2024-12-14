#ifndef LIBBASE_PROPERTIES_HH
#define LIBBASE_PROPERTIES_HH

#ifndef LIBBASE_PROPERTIES
#    define LIBBASE_PROPERTIES __clang__
#endif

#if LIBBASE_PROPERTIES
#define ComputedProperty(type, name, ...)                               \
public:                                                                 \
    [[nodiscard]] type get_##name() const __VA_OPT__({ return ) LIBBASE_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) ;) __VA_OPT__(;}) \
    void set_##name(type new_value);                                    \
    __declspec(property(get = get_##name, put = set_##name)) type name; \
private:

#define Property(ty, name, ...)                                                                                \
private:                                                                                                       \
    ::base::detail::PropertyType<ty>::type _##name __VA_OPT__(=) __VA_ARGS__;                                  \
public:                                                                                                        \
    [[nodiscard]] decltype(auto) get_##name() const { return ::base::detail::PropertyType<ty>::get(_##name); } \
    void set_##name(ty new_value);                                                                             \
    __declspec(property(get = get_##name, put = set_##name)) ty name;                                          \
private:

#define ComputedReadonly(type, name, ...)             \
public:                                               \
    [[nodiscard]] type get_##name() const __VA_OPT__({ return ) LIBBASE_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) ;) __VA_OPT__(;}) \
    __declspec(property(get = get_##name)) type name; \
private:

#define Readonly(ty, name, ...)                                                                                \
private:                                                                                                       \
    ::base::detail::PropertyType<ty>::type _##name __VA_OPT__(=) __VA_ARGS__;                                  \
public:                                                                                                        \
    [[nodiscard]] decltype(auto) get_##name() const { return ::base::detail::PropertyType<ty>::get(_##name); } \
    __declspec(property(get = get_##name)) ty name;                                                            \
private:

#define Writeonly(type, name, ...)                    \
public:                                               \
    void set_##name(type new_value);                  \
    __declspec(property(put = set_##name)) type name; \
private:

namespace base::detail {
template <typename Ty>
struct PropertyType {
    using type = Ty;

private:
    using get_return_ty = std::conditional_t<
        sizeof(type) <= 16 and std::is_trivially_copyable_v<type>,
        type,
        const type&
    >;

public:
    static auto get(const type& t) -> get_return_ty { return t; }
};

template <typename Ty>
struct PropertyType<Ty&> {
    using type = Ty*;

    static auto get(type t) -> decltype(auto) { return *t; }
};
}

#endif // LIBBASE_PROPERTIES
#endif // LIBBASE_PROPERTIES_HH
