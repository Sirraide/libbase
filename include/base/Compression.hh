#ifndef LIBBASE_COMPRESSION_HH
#define LIBBASE_COMPRESSION_HH

#include <base/Serialisation.hh>
#include <base/Size.hh>

namespace base {
constexpr int MaxCompressionLevel = std::numeric_limits<int>::max();

namespace detail {
[[nodiscard]] auto CompressImpl(MutableByteSpan output, ByteSpan input, std::optional<int> compression_level) -> usz;
[[nodiscard]] auto DecompressImpl(MutableByteSpan output, ByteSpan input) -> Result<usz>;
[[nodiscard]] auto GetCompressedSize(ByteSpan input) -> usz;
[[nodiscard]] auto GetDecompressedSize(ByteSpan input) -> Result<usz>;
}

/// Compresses a sequence of bytes into a container.
///
/// \param into Container the compressed data will be appended to.
/// \param input The input data to compress.
/// \param compression_level Higher means better compression; pass \c std::nullopt
///    to use the default level, or \c base::MaxCompressionLevel to compress as much
///    as possible.
template <typename Buffer>
void CompressInto(
    Buffer& into,
    ByteSpan input,
    std::optional<int> compression_level = {}
) {
    static_assert(BytePointer<decltype(into.data())>);

    // Take care not to overwrite data that is already in the buffer.
    auto starting_size = into.size();

    // Allocate space for the compressed data.
    auto max_compressed_size = detail::GetCompressedSize(input);
    into.resize(starting_size + max_compressed_size);
    auto output_start = static_cast<std::byte*>(static_cast<void*>(into.data())) + starting_size;

    // Perform the compression.
    auto actual_size = detail::CompressImpl({output_start, max_compressed_size}, input, compression_level);

    // Truncate the buffer to how many bytes we actually used.
    into.resize(starting_size + actual_size);
}

/// Compress a sequence of bytes and return a new buffer.
///
/// \see CompressInto()
template <typename Buffer = std::vector<std::byte>>
[[nodiscard]] auto Compress(
    ByteSpan input,
    std::optional<int> compression_level = {}
) -> Buffer {
    Buffer b;
    CompressInto(b, input, compression_level);
    return b;
}

/// Decompress a sequence of bytes.
///
/// \param into Container the decompressed data will be appended to.
/// \param input The input data to decompress.
template <typename Buffer>
auto DecompressInto(
    Buffer& into,
    ByteSpan input
) -> Result<> {
    static_assert(BytePointer<decltype(into.data())>);

    // Take care not to overwrite data that is already in the buffer.
    auto starting_size = into.size();

    // Allocate space for the decompressed data.
    auto max_decompressed_size = Try(detail::GetDecompressedSize(input));
    into.resize(starting_size + max_decompressed_size);
    auto output_start = static_cast<std::byte*>(static_cast<void*>(into.data())) + starting_size;

    // Perform the decompression.
    auto actual_size = Try(detail::DecompressImpl({output_start, max_decompressed_size}, input));

    // Truncate the buffer to how many bytes we actually used.
    into.resize(starting_size + actual_size);
    return {};
}

/// Decompress a sequence of bytes and return a new buffer.
///
/// \see DecompressInto()
template <typename Buffer = std::vector<std::byte>>
[[nodiscard]] auto Decompress(ByteSpan input) -> Result<Buffer> {
    Buffer b;
    Try(DecompressInto(b, input));
    return b;
}
}

#endif // LIBBASE_COMPRESSION_HH
