#ifdef LIBBASE_ENABLE_ZSTD

#include "TestCommon.hh"

#include <base/Compression.hh>
#include <base/FS.hh>

using namespace base;

TEST_CASE("Compression tests") {
    auto data = File::Read(__FILE__).value();
    auto compressed = Compress(data);
    CHECK(compressed.size() < data.size());
    auto decompressed = Decompress(compressed).value();
    CHECK(data.span() == ByteSpan(decompressed));

    compressed.clear();
    CompressInto(compressed, data);
    DecompressInto(decompressed, compressed).value();
    std::string s{data.span().str()};
    CHECK(s + s == ByteSpan(decompressed).str());
}

#endif // LIBBASE_ENABLE_ZSTD
