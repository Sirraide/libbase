#include <base/Compression.hh>
#include <zstd.h>

using namespace base;

auto detail::CompressImpl(
    MutableByteSpan output,
    ByteSpan input,
    std::optional<int> compression_level
) -> usz {
    auto level = std::clamp(
        compression_level.value_or(ZSTD_defaultCLevel()),
        ZSTD_minCLevel(),
        ZSTD_maxCLevel()
    );

    auto size = ZSTD_compress(
        output.data(),
        output.size_bytes(),
        input.data(),
        input.size_bytes(),
        level
    );

    Assert(
        not ZSTD_isError(size),
        "Compression failed: {}",
        ZSTD_getErrorString(ZSTD_getErrorCode(size))
    );

    return size;
}

auto detail::DecompressImpl(MutableByteSpan output, ByteSpan input) -> Result<usz> {
    auto size = ZSTD_decompress(
        output.data(),
        output.size_bytes(),
        input.data(),
        input.size_bytes()
    );

    if (ZSTD_isError(size)) return Error(
        "Decompression failed: {}",
        ZSTD_getErrorString(ZSTD_getErrorCode(size))
    );

    return size;
}

usz detail::GetCompressedSize(ByteSpan input) {
    return ZSTD_compressBound(input.size());
}

auto detail::GetDecompressedSize(ByteSpan input) -> Result<usz> {
    auto res = ZSTD_getFrameContentSize(input.data(), input.size());
    if (res == ZSTD_CONTENTSIZE_UNKNOWN) [[unlikely]]
        return Error("Content size unknown: {}", ZSTD_getErrorString(ZSTD_getErrorCode(res)));
    if (res == ZSTD_CONTENTSIZE_ERROR) [[unlikely]]
        return Error("Could not compute content size: {}", ZSTD_getErrorString(ZSTD_getErrorCode(res)));
    return res;
}

