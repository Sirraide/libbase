#ifndef LIBBASE_SERIALISATION_HH
#define LIBBASE_SERIALISATION_HH

#include <base/Utils.hh>
#include <base/Types.hh>
#include <base/Result.hh>
#include <span>
#include <utility>
#include <vector>

#define LIBBASE_SERIALISE(...)                                                                       \
    template <std::endian E> void serialise(::base::ser::Writer<E>& buf) const { buf(__VA_ARGS__); } \
    template <std::endian E> void deserialise(::base::ser::Reader<E>& buf) { buf(__VA_ARGS__); }

/// ====================================================================
///  Serialisation
/// ====================================================================
///
/// This defines a Reader and Writer pair for serialising and deserialising
/// objects. The simplest way of using this for your own types is to use the
/// LIBBASE_SERIALISE() macro:
///
/// class Foo {
///     LIBBASE_SERIALISE(field1, field2, field3, ...);
///     ...
/// };
///
/// If this is not possible, perhaps because the type in question is defined
/// in another library, you can implement specialisations of Serialiser<T>
/// for your type:
///
/// template <std::endian E>
/// struct base::Serialiser<Foo, E> {
///     static void serialise(Writer<E>& w, const Foo& f) { ... }
///     static void deserialise(Reader<E>& r, const Foo& f) { ... }
/// };
namespace base::ser {
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
template <typename T, std::endian SerialisedEndianness = std::endian::little>
auto Deserialise(InputSpan data) -> Result<T>;

/// Serialise a type to a vector of bytes.
///
/// @tparam SerialisedEndianness The input endianness of the serialised data.
template <std::endian SerialisedEndianness = std::endian::little, typename T>
void Serialise(std::vector<std::byte>& into, const T& t);

/// Serialise a type to a new vector of bytes.
///
/// @tparam SerialisedEndianness The input endianness of the serialised data.
template <std::endian SerialisedEndianness = std::endian::little, typename T>
auto Serialise(const T& t) -> std::vector<std::byte>;
}

// TODO: Pass version number to deserialise().

/// ====================================================================
///  Reader/Writer
/// ====================================================================
namespace base::ser {
template <std::endian SerialisedEndianness = std::endian::little>  class Reader;
template <std::endian SerialisedEndianness = std::endian::little>  class Writer;

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
    Result<> result; // TODO: Do we want to use exceptions here?
    explicit Reader(InputSpan data) : data(data) {}

    /// Read several fields from the buffer.
    template <typename... Fields>
    void operator()(Fields&&... fields) {
        (void) ((*this >> std::forward<Fields>(fields), result.has_value()) and ...);
    }

    /// Read a type from the buffer and return it.
    template <typename T>
    requires requires (Reader& r, T& t) { r >> t; }
    auto read() -> T {
        T t;
        *this >> t;
        return t;
    }

    /// Read an integer.
    template <std::integral T>
    auto operator>>(T& t) -> Reader& {
        Reader::Copy(&t, sizeof(T));
        if constexpr (std::endian::native != E) t = std::byteswap(t);
        return *this;
    }

    /// Read a floating-point number.
    auto operator>>(float& t) -> Reader&;
    auto operator>>(double& t) -> Reader&;

    /// Read an enum.
    template <typename T>
    requires std::is_enum_v<T>
    auto operator>>(T& t) -> Reader& {
        t = static_cast<T>(read<std::underlying_type_t<T>>());
        return *this;
    }

    /// Deserialise an object that provides a 'deserialise()' member.
    ///
    /// This accepts any reference to accommodate proxy objects.
    template <typename T>
    requires requires (T&& t, Reader& r) { std::forward<T>(t).deserialise(r); }
    auto operator>>(T&& t) -> Reader& {
        std::forward<T>(t).deserialise(*this);
        return *this;
    }

    /// Deserialise an object for which there is a Serialiser<T> specialisation.
    ///
    /// This accepts any reference to accommodate proxy objects.
    template <typename T>
    requires requires (T&& t, Reader& r) {
        Serialiser<std::remove_reference_t<T>, E>::Deserialise(r, std::forward<T>(t));
    } auto operator>>(T&& t) -> Reader& {
        Serialiser<std::remove_reference_t<T>, E>::Deserialise(*this, std::forward<T>(t));
        return *this;
    }

    /// Deserialise a byte string.
    template <typename T>
    requires (sizeof(T) == sizeof(std::byte))
    auto operator>>(std::basic_string<T>& s) -> Reader& {
        auto sz = read<u64>();
        if (sz > s.max_size()) [[unlikely]] {
            result = Error("Input size {} exceeds maximum size {} of std::basic_string<>", sz, s.max_size());
            return *this;
        }

        s.resize_and_overwrite(
            sz,
            [&](T* ptr, usz count) { return Copy(ptr, count * sizeof(T)); }
        );
        return *this;
    }

    /// Deserialise a string.
    template <typename T>
    auto operator>>(std::basic_string<T>& s) -> Reader& {
        return ReadRange(s);
    }

    /// Deserialise an array.
    template <typename T, usz n>
    auto operator>>(std::array<T, n>& a) -> Reader& {
        for (auto& elem : a) *this >> elem;
        return *this;
    }

    /// Deserialise a vector.
    template <typename T>
    auto operator>>(std::vector<T>& s) -> Reader& {
        return ReadRange(s);
    }

    // Deserialise an optional.
    template <typename T>
    auto operator>>(std::optional<T>& o) -> Reader& {
        if (not read<bool>()) {
            o = std::nullopt;
            return *this;
        }

        o = read<T>();
        return *this;
    }

    // TODO: Maps, variants.

    /// Check if we could read the entire thing.
    [[nodiscard]] explicit operator bool() { return result.has_value(); }

    /// Mark that serialisation has failed for whatever reason.
    void fail(std::string_view err) {
        if (result) result = Error("{}", err);
    }

    /// Check how many bytes are left in the buffer.
    [[nodiscard]] auto size() const -> usz { return data.size(); }

private:
    usz Copy(void* ptr, usz count);

    template <typename T>
    auto ReadRange(T& range) -> Reader& {
        auto sz = read<u64>();
        if (sz > range.max_size()) [[unlikely]] {
            result = Error("Input size {} exceeds maximum size {} of range", sz, range.max_size());
            return *this;
        }

        range.resize(sz);
        for (auto& elem : range) *this >> elem;
        return *this;
    }
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

    /// Serialise an integer.
    template <std::integral T>
    auto operator<<(T t) -> Writer& {
        if constexpr (E != std::endian::little) t = std::byteswap(t);
        Append(&t, sizeof(T));
        return *this;
    }

    /// Serialise a floating-point number.
    auto operator<<(float t) -> Writer&;
    auto operator<<(double t) -> Writer&;

    /// Serialise an enum.
    template <typename T>
    requires std::is_enum_v<T>
    auto operator<<(T t) -> Writer& {
        *this << std::to_underlying(t);
        return *this;
    }

    /// Serialise an object that provides a 'serialise()' member.
    template <typename T>
    requires requires (const T& t, Writer& w) { t.serialise(w); }
    auto operator<<(const T& t) -> Writer& {
        t.serialise(*this);
        return *this;
    }

    /// Serialise an object for which there is a Serialiser<T> specialisation.
    template <typename T>
    requires requires (const T& t, Writer& w) {
        Serialiser<std::remove_reference_t<T>, E>::Serialise(w, t);
    } auto operator<<(const T& t) -> Writer& {
        Serialiser<std::remove_reference_t<T>, E>::Serialise(*this, t);
        return *this;
    }

    /// Serialise a byte string.
    template <typename T>
    requires (sizeof(T) == sizeof(std::byte))
    auto operator<<(const std::basic_string<T>& s) -> Writer& {
        *this << u64(s.size());
        Append(s.data(), s.size() * sizeof(T));
        return *this;
    }

    /// Serialise a string.
    template <typename T>
    auto operator<<(const std::basic_string<T>& s) -> Writer& { return AppendRange(s); }

    /// Serialise an array.
    template <typename T, u64 n>
    auto operator<<(const std::array<T, n>& a) -> Writer& {
        for (const auto& elem : a) *this << elem;
        return *this;
    }

    /// Serialise a vector.
    template <typename T>
    auto operator<<(const std::vector<T>& v) -> Writer& { return AppendRange(v); }

    /// Serialise an optional.
    template <typename T>
    auto operator<<(const std::optional<T>& o) -> Writer& {
        *this << o.has_value();
        if (o) *this << *o;
        return *this;
    }

private:
    void Append(const void* ptr, u64 count);

    template <typename T>
    auto AppendRange(const T& range) -> Writer& {
        *this << u64(range.size());
        for (const auto& elem : range) *this << elem;
        return *this;
    }
};

extern template class base::ser::Reader<std::endian::little>;
extern template class base::ser::Reader<std::endian::big>;

extern template class base::ser::Writer<std::endian::little>;
extern template class base::ser::Writer<std::endian::big>;

template <typename T, std::endian SerialisedEndianness>
auto base::ser::Deserialise(InputSpan data) -> Result<T> {
    Result<T> ret{T{}};
    Reader<SerialisedEndianness> r{data};
    r >> ret.value();
    if (not r) ret = std::unexpected(std::move(r.result).error());
    return ret;
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


#endif // LIBBASE_SERIALISATION_HH
