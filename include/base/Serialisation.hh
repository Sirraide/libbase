#ifndef LIBBASE_SERIALISATION_HH
#define LIBBASE_SERIALISATION_HH

#include <array>
#include <base/Result.hh>
#include <base/StringUtils.hh>
#include <base/Types.hh>
#include <base/Utils.hh>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#define LIBBASE_SERIALISE(Class, ...)                                                                      \
    template <std::endian E> friend class ::base::ser::Reader;                                             \
    template <std::endian E> friend class ::base::ser::Writer;                                             \
    template <std::endian E> void serialise(::base::ser::Writer<E>& w) const { w(__VA_ARGS__); }           \
    template <std::endian E> static auto deserialise(::base::ser::Reader<E>& r) -> Result<Class> {         \
        return std::apply(                                                                                 \
            [](auto&&... args) {                                                                           \
                if constexpr (requires { Class{::base::ser::deserialise_tag{}, LIBBASE_FWD(args)...}; }) { \
                    return Class{::base::ser::deserialise_tag{}, LIBBASE_FWD(args)...};                    \
                } else {                                                                                   \
                    return Class{LIBBASE_FWD(args)...};                                                    \
                }                                                                                          \
            },                                                                                             \
            Try(r.template read<decltype(std::tuple(__VA_ARGS__))>())                                      \
        );                                                                                                 \
    }

/// ====================================================================
///  Serialisation
/// ====================================================================
///
/// This defines a Reader and Writer pair for serialising and deserialising
/// objects. The simplest way of using this for your own types is to use the
/// LIBBASE_SERIALISE() macro.
///
/// class Foo {
///     LIBBASE_SERIALISE(Foo, field1, field2, field3, ...);
///     ...
/// };
///
/// The deserialiser will then invoke a constructor that takes values of type
/// 'decltype(field1)', 'decltype(field2)', etc. in that order. If a constructor
/// exists whose first parameter is of type 'base::ser::deserialise_tag', then
/// that constructor is called instead.
///
/// If this is not possible, perhaps because the type in question is defined
/// in another library, you can implement specialisations of Serialiser<T>
/// for your type:
///
/// template <std::endian E>
/// struct base::Serialiser<Foo, E> {
///     static auto deserialise(Reader<E>& r) -> Result<Foo> { ... }
///     static void serialise(Writer<E>& w, const Foo& f) { ... }
/// };
namespace base::ser {
struct deserialise_tag {};

template <std::endian SerialisedEndianness>  class Reader;
template <std::endian SerialisedEndianness>  class Writer;

using ReaderLE = Reader<std::endian::little>;
using ReaderBE = Reader<std::endian::big>;
using WriterLE = Writer<std::endian::little>;
using WriterBE = Writer<std::endian::big>;

/// Span of bytes that can be constructed from various other
/// representations of ‘a blob of data’.
struct InputSpan : std::span<const std::byte> {
    using std::span<const std::byte>::span;

    explicit InputSpan(const char* data, usz size)
        : std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size) {}

    explicit InputSpan(const u8* data, usz size)
        : std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size) {}

    InputSpan(std::span<const std::byte> data) : std::span<const std::byte>(data) {}
    InputSpan(std::span<const char> data) : InputSpan(data.data(), data.size()) {}
    InputSpan(std::span<const u8> data) : InputSpan(data.data(), data.size()) {}

    template <usz n>
    explicit InputSpan(const char (&str)[n])
        : std::span<const std::byte>(reinterpret_cast<const std::byte*>(str), n) {}
};

/// Magic number to check the serialised data is valid.
/// TODO: Add this back and use a static_string template parameter for it.
/*template <usz n>
class Magic {
    static_assert(n > 0, "Magic number must have at least one byte.");
    std::array<char, n> magic{};

public:
    template <usz sz>
    consteval explicit Magic(const char (&m)[sz]) {
        static_assert(sz == n + 1);
        std::copy_n(m, n, magic.begin());
    }

    template <utils::is<char, u8, std::byte>... Vals>
    consteval explicit Magic(Vals... vals) : magic{char(vals)...} {}

    template <std::endian E> auto deserialise(Reader<E>& r) const -> Result<Magic>;
    template <std::endian E> void serialise(Writer<E>& w) const;
};

template <usz n>
Magic(const char (&)[n]) -> Magic<n - 1>;

template <utils::is<char, u8, std::byte> ...Vals>
Magic(Vals...) -> Magic<sizeof...(Vals)>;*/

/// Class template that can be specialised to make custom types
/// serialisable.
///
/// Generally, you should prefer using the LIBBASE_SERIALISE()
/// macro instead.
template <typename T, std::endian E>
struct Serialiser;

/// Deserialise a type from a span of bytes.
///
/// @tparam SerialisedEndianness The output endianness of the serialised data.
template <typename T, std::endian SerialisedEndianness>
auto Deserialise(InputSpan data) -> Result<T>;

/// Serialise a type to a vector of bytes.
///
/// @tparam SerialisedEndianness The input endianness of the serialised data.
template <std::endian SerialisedEndianness, typename T>
void Serialise(std::vector<std::byte>& into, const T& t);

/// Serialise a type to a new vector of bytes.
///
/// @tparam SerialisedEndianness The input endianness of the serialised data.
template <std::endian SerialisedEndianness, typename T>
auto Serialise(const T& t) -> std::vector<std::byte>;
}

// TODO: Pass version number to deserialise().

/// ====================================================================
///  Reader/Writer
/// ====================================================================
namespace base::ser {
/// Guard against invalid enum values.
consteval bool CheckEndianness(std::endian e) {
    return e == std::endian::big or e == std::endian::little;
}
}

/// Helper to deserialise objects.
template <std::endian E>
class base::ser::Reader {
    static_assert(CheckEndianness(E));
    InputSpan data;

public:
    explicit Reader(InputSpan data) : data(data) {}

    /// Deserialise an object that provides a 'deserialise()' member.
    template <typename T, typename Self>
    requires requires (Self& self) { T::deserialise(self); }
    auto read(this Self& self) -> Result<T> {
        return T::deserialise(self);
    }

    /// Deserialise an object for which there is a 'Serialiser<T>' specialisation.
    template <typename T, typename Self>
    requires requires (Self& self) {
        { Serialiser<T, E>::deserialise(self) } -> std::convertible_to<Result<T>>;
    } auto read(this Self& self) -> Result<T> {
        return Serialiser<T, E>::deserialise(self);
    }

    /// Extraction operator.
    template <typename Self, typename T>
    requires requires (Self& self) { self.template read<T>(); }
    auto operator>>(this Self&& self, T&& t) -> Result<> {
        std::forward<T>(t) = Try(self.template read<T>());
        return {};
    }

    /// Check how many bytes are left in the buffer.
    [[nodiscard]] auto size() const -> usz { return data.size(); }

    /// Read bytes from the buffer.
    auto read_bytes(usz count) -> Result<std::span<const std::byte>>;

    /// Read bytes into a memory location.
    auto read_bytes_into(void* v, usz count) -> Result<>;
};

/// Helper to serialise objects.
template <std::endian E>
class base::ser::Writer {
    static_assert(CheckEndianness(E));
    std::vector<std::byte>& data;

public:
    explicit Writer(std::vector<std::byte>& data) : data(data) {}

    /// Write several fields to this buffer.
    template <typename... Fields>
    void operator()(Fields&&... fields) {
        (*this << ... << std::forward<Fields>(fields));
    }

    /// Serialise an object that provides a 'serialise()' member.
    template <typename Self, typename T>
    requires requires (Self& self, const T& t) { t.serialise(self); }
    void write(this Self& self, const T& t) { t.serialise(self); }

    /// Serialise an object for which there is a 'Serialiser<T>' specialisation.
    template <typename Self, typename T>
    requires requires (Self& self, const T& t) {
        Serialiser<std::remove_cvref_t<T>, E>::serialise(self, t);
    } void write(this Self& self, const T& t) {
        Serialiser<std::remove_cvref_t<T>, E>::serialise(self, t);
    }

    /// Chaining operator.
    template <typename Self, typename T>
    requires requires (Self& self, const T& t) { self.write(t); }
    auto operator<<(this Self&& self, const T& t) -> Self&& {
        self.write(t);
        return LIBBASE_FWD(self);
    }

    /// Get a span of bytes to write data into; the span is valid until
    /// the next call to a member function of Writer.
    ///
    /// Example usage:
    ///
    ///    auto buf = Allocate(value.size());
    ///    ExternalSerialisationFunction(value, buf.data(), buf.size());
    ///
    auto allocate(u64 bytes) -> std::span<std::byte>;

    /// Append raw bytes.
    void append_bytes(const void* ptr, usz count);
};

extern template class base::ser::Reader<std::endian::little>;
extern template class base::ser::Reader<std::endian::big>;

extern template class base::ser::Writer<std::endian::little>;
extern template class base::ser::Writer<std::endian::big>;

template <typename T, std::endian SerialisedEndianness>
auto base::ser::Deserialise(InputSpan data) -> Result<T> {
    Reader<SerialisedEndianness> r{data};
    return r.template read<T>();
}

template <std::endian SerialisedEndianness, typename T>
auto base::ser::Serialise(const T& t) -> std::vector<std::byte> {
    std::vector<std::byte> vec;
    ser::Serialise<SerialisedEndianness>(vec, t);
    return vec;
}

template <std::endian SerialisedEndianness, typename T>
void base::ser::Serialise(std::vector<std::byte>& into, const T& t) {
    Writer<SerialisedEndianness> w{into};
    w << t;
}

/// ====================================================================
///  Built-in Serialisers
/// ====================================================================
/// Serialiser for integer types.
template <std::integral Int, std::endian E>
struct base::ser::Serialiser<Int, E> {
    static auto deserialise(Reader<E>& r) -> Result<Int> {
        Int val{};
        Try(r.read_bytes_into(&val, sizeof(Int)));
        if constexpr (E != std::endian::native) val = std::byteswap(val);
        return val;
    }

    static void serialise(Writer<E>& w, Int val) {
        if constexpr (E != std::endian::native) val = std::byteswap(val);
        w.append_bytes(&val, sizeof(Int));
    }
};

/// Serialiser for 'bool'.
template <std::endian E>
struct base::ser::Serialiser<bool, E> {
    static auto deserialise(Reader<E>& r) -> Result<bool> {
        return bool(Try(r.template read<u8>()));
    }

    static void serialise(Writer<E>& w, bool value) {
        // Store as a u8 to avoid serialising padding bits.
        w << u8(value);
    }
};

/// Serialiser for 'float'.
template <std::endian E>
struct base::ser::Serialiser<float, E> {
    static auto deserialise(Reader<E>& r) -> Result<float> {
        return std::bit_cast<float>(Try(r.template read<u32>()));
    }

    static void serialise(Writer<E>& w, float value) {
        w << std::bit_cast<u32>(value);
    }
};

/// Serialiser for 'double'.
template <std::endian E>
struct base::ser::Serialiser<double, E> {
    static auto deserialise(Reader<E>& r) -> Result<double> {
        return std::bit_cast<double>(Try(r.template read<u64>()));
    }

    static void serialise(Writer<E>& w, double value) {
        w << std::bit_cast<u64>(value);
    }
};

/// Serialiser for enums.
template <typename Enum, std::endian E>
requires std::is_enum_v<Enum>
struct base::ser::Serialiser<Enum, E> {
    using Base = std::underlying_type_t<Enum>;

    static auto deserialise(Reader<E>& r) -> Result<Enum> {
        return Enum(Try(r.template read<Base>()));
    }

    static void serialise(Writer<E>& w, Enum value) {
        w << Base(value);
    }
};

/// Serialiser for strings.
template <base::utils::is_same<char, char8_t, char16_t, char32_t> Char, std::endian E>
struct base::ser::Serialiser<std::basic_string<Char>, E> {
    using SizeType = u64; // Not 'size_t' because that’s platform-dependent.

    static auto deserialise(Reader<E>& r) -> Result<std::basic_string<Char>> {
        std::basic_string<Char> str;
        auto size = Try(r.template read<SizeType>());
        if (size > SizeType(str.max_size())) [[unlikely]] {
            return Error(
                "Input size {} exceeds maximum size {} of std::basic_string<>",
                size,
                str.max_size()
            );
        }

        Result<> read_result;
        str.resize_and_overwrite(usz(size), [&](Char* ptr, usz sz) {
            read_result = r.read_bytes_into(ptr, sz * sizeof(Char));
            return sz;
        });
        Try(std::move(read_result));

        if constexpr (E != std::endian::native and sizeof(Char) != sizeof(char)) {
            rgs::transform(
                str.begin(),
                str.end(),
                str.begin(),
                std::byteswap<Char>
            );
        }

        return str;
    }

    static void serialise(Writer<E>& w, const std::basic_string<Char>& str) {
        w << SizeType(str.size());

        usz size_bytes = usz(str.size()) * sizeof(Char);
        if constexpr (E != std::endian::native and sizeof(Char) != sizeof(char)) {
            auto bytes = w.allocate(size_bytes);
            rgs::transform(
                str.begin(),
                str.end(),
                reinterpret_cast<Char*>(bytes.data()),
                std::byteswap<Char>
            );
        } else {
            w.append_bytes(str.data(), size_bytes);
        }
    }
};

/// Serialiser for vector-like types.
///
/// This handles 'std::vector', 'llvm::SmallVector', etc.
template <typename Vector, std::endian E>
requires requires (const Vector& cv, Vector& v, std::size_t sz)
{
    typename Vector::value_type;
    typename Vector::size_type;
    { cv.size() } -> std::convertible_to<std::size_t>;
    { cv.max_size() } -> std::convertible_to<std::size_t>;
    { cv.begin() };
    { cv.end() };
    { v.push_back(std::declval<typename Vector::value_type>()) };
}
struct base::ser::Serialiser<Vector, E> {
    using SizeType = u64; // Not 'size_t' because that’s platform-dependent.
    using Element = Vector::value_type;

    static auto deserialise(Reader<E>& r) -> Result<Vector> {
        Vector vec;
        auto size = Try(r.template read<SizeType>());
        if (size > SizeType(vec.max_size())) [[unlikely]] {
            return Error(
                "Input size {} exceeds maximum size {}",
                size,
                vec.max_size()
            );
        }

        if constexpr (requires { vec.reserve(size); })
            vec.reserve(size);

        for (SizeType i = 0; i < size; ++i)
            vec.push_back(Try(r.template read<Element>()));

        return vec;
    }

    static void serialise(Writer<E>& w, const Vector& v) {
        w << SizeType(v.size());
        for (const Element& val : v) w << val;
    }
};

/// Serialiser for 'std::array'.
template <typename Element, std::size_t N, std::endian E>
struct base::ser::Serialiser<std::array<Element, N>, E> {
    static auto deserialise(Reader<E>& r) -> Result<std::array<Element, N>> {
        std::array<Element, N> array;
        for (usz i = 0; i < N; ++i)
            array[i] = Try(r.template read<Element>());
        return array;
    }

    static void serialise(Writer<E>& w, const std::array<Element, N>& array) {
        for (const Element& val : array) w << val;
    }
};

/// Serialiser for 'std::optional'.
template <typename Value, std::endian E>
struct base::ser::Serialiser<std::optional<Value>, E> {
    static auto deserialise(Reader<E>& r) -> Result<std::optional<Value>> {
        bool present = Try(r.template read<bool>());
        if (not present) return std::nullopt;
        return Try(r.template read<Value>());
    }

    static void serialise(Writer<E>& w, const std::optional<Value>& value) {
        w << value.has_value();
        if (value.has_value()) w << *value;
    }
};

/// Serialiser for 'std::variant'.
template <typename ...Ts, std::endian E>
struct base::ser::Serialiser<std::variant<Ts...>, E> {
    using IndexType = u64;
    using Variant = std::variant<Ts...>;

    static auto deserialise(Reader<E>& r) -> Result<Variant> {
        IndexType index = Try(r.template read<IndexType>());
        if (index >= sizeof...(Ts)) [[unlikely]] {
            return Error(
                "Variant index {} exceeds number of alternatives {}",
                index,
                sizeof...(Ts)
            );
        }

        std::optional<Result<Variant>> value;
        [&]<usz ...I>(std::index_sequence<I...>) {
            ([&] {
                if (index == IndexType(I)) {
                    value.emplace(r.template read<std::variant_alternative_t<I, Variant>>());
                    return true;
                }
                return false;
            }() or ...);
        }(std::index_sequence_for<Ts...>());

        Assert(value.has_value());
        return std::move(value.value());
    }

    static void serialise(Writer<E>& w, const Variant& value) {
        w << IndexType(value.index());
        std::visit([&](const auto& v) { w << v; }, value);
    }
};

/// Serialiser for 'std::monostate'.
template <std::endian E>
struct base::ser::Serialiser<std::monostate, E> {
    static auto deserialise(Reader<E>&) -> Result<std::monostate> { return std::monostate{}; }
    static void serialise(Writer<E>&, std::monostate) {}
};

/// Serialiser for 'std::tuple'.
template <typename ...Ts, std::endian E>
struct base::ser::Serialiser<std::tuple<Ts...>, E> {
    static_assert(
        (base::utils::is_same<std::remove_cvref_t<Ts>, Ts> and ...),
        "Cannot serialise a std::tuple that contains reference or const-qualified types"
    );

    static auto deserialise(Reader<E>& r) -> Result<std::tuple<Ts...>> {
        Result<> result;
        std::tuple<std::optional<Ts>...> elements;
        [&]<usz ...I>(std::index_sequence<I...>) {
            ([&] {
                auto res = r.template read<Ts...[I]>();
                if (not res) {
                    result = std::unexpected(std::move(res).error());
                    return false;
                }

                std::get<I>(elements) = std::move(res).value();
                return true;
            }() and ...);
        }(std::make_index_sequence<sizeof...(Ts)>());

        Try(std::move(result));
        return std::apply(
            [](auto&&... v) { return std::tuple<Ts...>{std::move(v).value()...}; },
            std::move(elements)
        );
    }

    static void serialise(Writer<E>& w, const std::tuple<Ts...>& ts) {
        std::apply([&] (const auto& ...v) { ((w << v), ...); }, ts);
    }
};

/// Serialiser for 'std::pair'.
template <typename A, typename B, std::endian E>
struct base::ser::Serialiser<std::pair<A, B>, E> {
    using Pair = std::pair<A, B>;

    static auto deserialise(Reader<E>& r) -> Result<Pair> {
        return Pair{
            Try(r.template read<A>()),
            Try(r.template read<B>()),
        };
    }

    static void serialise(Writer<E>& w, const Pair& pair) {
        w << pair.first << pair.second;
    }
};

/// Serialiser for magic numbers.
/*template <base::usz n>
template <std::endian E>
auto base::ser::Magic<n>::deserialise(Reader<E>& r) const -> Result<Magic> {
    auto buf = r.template read<std::array<char, n>>();
    if (r and buf != magic) return Error(
        "Magic number mismatch! Got [{}], expected [{}]",
        utils::join(buf, ", ", "{:#x}"),
        utils::join(magic, ", ", "{:#x}")
    );
    return *this;
}

template <base::usz n>
template <std::endian E>
void base::ser::Magic<n>::serialise(Writer<E>& w) const {
    w << magic;
}*/

#endif // LIBBASE_SERIALISATION_HH
