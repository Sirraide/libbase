#ifndef LIBBASE_SPAN_HH
#define LIBBASE_SPAN_HH

#include <span>
#include <base/Types.hh>
#include <base/Utils.hh>

namespace base {
template <typename CharType>
class basic_str;

namespace detail {
template <typename ValueType, bool AllowByte>
class SpanImpl;
}

/// Span of bytes that can be constructed from various other
/// representations of ‘a blob of data’.
struct ByteSpan;

/// Mutable span of bytes.
///
/// \see ByteSpan
struct MutableByteSpan;

/// Mutable dynamic span with some additional functions (closer to llvm::ArrayRef).
template <typename ValueType>
requires (not std::is_const_v<ValueType>)
using MutableSpan = detail::SpanImpl<ValueType, false>;

/// Dynamic span with some additional functions (closer to llvm::ArrayRef).
template <typename ValueType>
requires (not std::is_const_v<ValueType>)
using Span = detail::SpanImpl<const ValueType, false>;
}

/// ====================================================================
///  Implementation
/// ====================================================================
namespace base::detail {
template <typename ValueType, bool AllowByte>
class SpanImpl : public std::span<ValueType> {
public:
    using Base = std::span<ValueType>;
    using Base::Base;
    using Base::operator=;
    static_assert(not std::is_same_v<ValueType, std::byte> or AllowByte, "Use (Mutable)ByteSpan instead");

    SpanImpl(ValueType& v) : Base{std::addressof(v), 1} {}
    SpanImpl(ValueType&& v) : Base{std::addressof(v), 1} {}

    // We don’t implement '<=>' to accommodate classes that e.g. only support '==' but not '<'.
#define COMPARE(ret, op, impl)                                                            \
    [[nodiscard]] ret operator op(const SpanImpl& other) const                      \
    noexcept(noexcept(impl(this->begin(), this->end(), other.begin(), other.end()))) \
    requires requires (const ValueType v) {                                          \
        { v op v };                                                                  \
    } {                                                                              \
        return impl(this->begin(), this->end(), other.begin(), other.end());         \
    }

    COMPARE(bool, ==, std::equal);
    COMPARE(bool, <, std::lexicographical_compare);
    COMPARE(auto, <=>, std::lexicographical_compare_three_way);

#undef COMPARE
};
}

struct base::ByteSpan : detail::SpanImpl<const std::byte, true> {
    using SpanImpl::SpanImpl;
    using SpanImpl::operator=;

    ByteSpan(const char* data, usz size)
        : SpanImpl(reinterpret_cast<const std::byte*>(data), size) {}

    ByteSpan(const u8* data, usz size)
        : SpanImpl(reinterpret_cast<const std::byte*>(data), size) {}

    /* implicit */ ByteSpan(const utils::SizedByteRange auto& data) : SpanImpl(
        static_cast<const std::byte*>(static_cast<const void*>(data.data())),
        data.size()
    ) {}

    template <usz n>
    /* implicit */ ByteSpan(const char (&str)[n]) : SpanImpl(
        reinterpret_cast<const std::byte*>(str),
        n != 0 and str[n - 1] == '\0' ? n - 1 : n
    ) {}

    [[nodiscard]] auto operator<=>(const ByteSpan& other) const {
        return std::lexicographical_compare_three_way(
            begin(),
            end(),
            other.begin(),
            other.end()
        );
    }

    [[nodiscard]] auto str() const -> basic_str<char>;
};

struct base::MutableByteSpan : detail::SpanImpl<std::byte, true> {
    using SpanImpl::SpanImpl;
    using SpanImpl::operator=;

    MutableByteSpan(char* data, usz size)
        : SpanImpl(reinterpret_cast<std::byte*>(data), size) {}

    MutableByteSpan(u8* data, usz size)
        : SpanImpl(reinterpret_cast<std::byte*>(data), size) {}

    /* implicit */ MutableByteSpan(utils::SizedByteRange auto& data) : SpanImpl(
        static_cast<std::byte*>(static_cast<void*>(data.data())),
        data.size()
    ) {}

    [[nodiscard]] auto operator<=>(const MutableByteSpan& other) const {
        return std::lexicographical_compare_three_way(
            begin(),
            end(),
            other.begin(),
            other.end()
        );
    }

    [[nodiscard]] auto str() const -> basic_str<char>;
};

#endif // LIBBASE_SPAN_HH
