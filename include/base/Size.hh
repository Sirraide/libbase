#ifndef LIBBASE_SIZE_HH
#define LIBBASE_SIZE_HH

#include <base/Macros.hh>
#include <base/Types.hh>
#include <climits>
#include <ostream>

namespace base {
static_assert(CHAR_BIT == 8, "Platforms where CHAR_BIT != 8 are not supported!");

class Size;

/// A byte-sized pointer.
template <typename T>
concept BytePointer = // clang-format off
    std::is_pointer_v<T> and
    utils::is_same<std::remove_cv_t<std::remove_pointer_t<T>>,
        void,
        char,
        signed char,
        unsigned char,
        std::byte
    >;
// clang-format on

/// Used to represent the alignment of a type in bytes.
class Align {
    u8 log_value;

public:
    /// Create an alignment of 1 byte.
    [[nodiscard]] constexpr Align() : log_value{0} {}

    /// Create a new alignment; the alignment must be a power of two.
    [[nodiscard]] constexpr explicit Align(std::unsigned_integral auto value) {
        if (not std::has_single_bit(value)) [[unlikely]] utils::ThrowOrAbort("Alignment must be a power of two");
        log_value = u8(std::countr_zero(value));
    }

    /// Create a new alignment; the alignment must be positive and a power of two.
    [[nodiscard]] constexpr explicit Align(std::signed_integral auto value) : Align(u64(value)) {
        if (value < 0) [[unlikely]] utils::ThrowOrAbort("Alignment must be positive");
    }

    /// Align a pointer to this alignment.
    template <BytePointer Pointer>
    [[nodiscard]] auto align(Pointer ptr) -> Pointer;

    /// Get the alignment of a type.
    template <typename Ty>
    [[nodiscard]] static consteval auto Of() -> Align {
        return Align(alignof(Ty));
    }

    /// Align a number to a multiple of another.
    ///
    /// Prefer to use Size and Align over invoking this directly; this is
    /// mainly here to implement those classes.
    [[nodiscard]] static constexpr u64 To(u64 value, u64 alignment) {
        DebugAssert(std::has_single_bit(alignment), "Alignment must be a power of 2");
        return (value + alignment - 1u) & ~(alignment - 1u);
    }

    /// Get the logarithmic representation of this alignment.
    [[nodiscard]] constexpr auto log_repr() const -> u8 { return log_value; }

    /// Get the alignment value.
    [[nodiscard]] constexpr auto value() const -> Size;

    /// Compare alignments.
    [[nodiscard]] constexpr auto operator<=>(const Align& rhs) const = default;
};

/// Used to represent the size of a type.
///
/// This is just a wrapper around an integer, but it requires us
/// to be explicit as to whether we want bits or bytes, which is
/// useful for avoiding mistakes.
///
/// Helpers to align the size to a given alignment are also provided.
class Size {
public:
    static constexpr u64 BitsPerByte = 8;

private:
    u64 raw;

    constexpr explicit Size(u64 raw) : raw{raw} {}

public:
    /// Create a size that stores a zero value.
    [[nodiscard]] constexpr Size() : raw{0} {}

    /// Create a size from a number of bits.
    [[nodiscard]] static constexpr Size Bits(std::unsigned_integral auto bits) {
        return Size{bits};
    }

    /// Create a size from a number of bits. The number must not be negative.
    [[nodiscard]] static constexpr Size Bits(std::signed_integral auto bits) {
        if (bits < 0) [[unlikely]] utils::ThrowOrAbort("Size must not be negative");
        return Size{u64(bits)};
    }

    /// Create a size from a number of bytes.
    [[nodiscard]] static constexpr Size Bytes(std::unsigned_integral auto bytes) {
        return Size{bytes * BitsPerByte};
    }

    /// Create a size from a number of bytes. The number must not be negative.
    [[nodiscard]] static constexpr Size Bytes(std::signed_integral auto bytes) {
        if (bytes < 0) [[unlikely]] utils::ThrowOrAbort("Size must not be negative");
        return Size{u64(bytes) * BitsPerByte};
    }

    /// Get the size of a type.
    ///
    /// Equivalent to Size::Bytes(sizeof(Ty)).
    template <typename Ty>
    [[nodiscard]] static consteval Size Of() { return Bytes(sizeof(Ty)); }

    /// Return this size aligned to a given alignment.
    [[nodiscard]] Size align(Align align) const {
        return Bytes(Align::To(bytes(), align.value().bytes()));
    }

    /// Return this size rounded up to the nearest byte.
    [[nodiscard]] constexpr Size as_bytes() const { return Bytes(bytes()); }

    /// Get the value of this size in bits.
    [[nodiscard]] constexpr auto bits() const -> u64 { return raw; }

    /// Get the value of this size in bytes.
    [[nodiscard]] constexpr auto bytes() const -> u64 {
        return Align::To(raw, BitsPerByte) / BitsPerByte;
    }

    /// Compare two sizes.
    [[nodiscard]] constexpr auto operator<=>(const Size& lhs) const = default;

    /// Mathematical operators; not all possible combinations are provided, only the
    /// ones that make sense semantically (e.g. multiplying two sizes doesn’t make sense
    /// so no `*` or `*=` operator is provided for that).
    ///
    /// Note: `/=` especially isn’t provided because dividing a size by a size yields
    /// an integer, not a size.
    constexpr Size operator+=(Size rhs) { return Size{raw += rhs.raw}; }
    constexpr Size operator-=(Size rhs) { return Size{raw -= rhs.raw}; }
    constexpr Size operator*=(u64 rhs) { return Size{raw *= rhs}; }

private:
    [[nodiscard]] friend constexpr Size operator*(Size lhs, u64 rhs) { return Size{lhs.raw * rhs}; }
    [[nodiscard]] friend constexpr Size operator*(u64 lhs, Size rhs) { return Size{lhs * rhs.raw}; }
    [[nodiscard]] friend constexpr auto operator/(Size lhs, Size rhs) -> u64 { return lhs.raw / rhs.raw; }
    [[nodiscard]] friend constexpr Size operator+(Size lhs, Size rhs) { return Size{lhs.raw + rhs.raw}; }

    template <BytePointer Pointer>
	[[nodiscard]] friend auto operator+(Pointer ptr, Size sz) -> Pointer {
        return reinterpret_cast<Pointer>(reinterpret_cast<uptr>(ptr) + sz.bytes());
    }

    template <BytePointer Pointer>
    friend auto operator+=(Pointer& ptr, Size sz) -> Pointer& {
        return ptr = ptr + sz;
    }

    template <BytePointer Pointer>
    [[nodiscard]] friend auto operator-(Pointer ptr, Size sz) -> Pointer {
        return reinterpret_cast<Pointer>(reinterpret_cast<uptr>(ptr) - sz.bytes());
    }

    template <BytePointer Pointer>
    friend auto operator-=(Pointer& ptr, Size sz) -> Pointer& {
        return ptr = ptr - sz;
    }

    /// This needs to check for underflow.
    [[nodiscard]] friend constexpr Size operator-(Size lhs, Size rhs) {
        if (lhs.raw < rhs.raw) [[unlikely]] utils::ThrowOrAbort("Size underflow");
        return Size{lhs.raw - rhs.raw};
    }

    /// For libassert.
    friend auto operator<<(std::ostream& os, Size sz) -> std::ostream& {
        return os << sz.bits();
    }
};

template <BytePointer Pointer>
auto Align::align(Pointer ptr) -> Pointer {
    auto s = Size::Bytes(reinterpret_cast<uptr>(ptr));
    s = s.align(*this);
    return reinterpret_cast<Pointer>(s.bytes());
}

constexpr auto Align::value() const -> Size {
    return Size::Bytes(u64(1) << u64(log_value));
}
}

#endif //LIBBASE_SIZE_HH
