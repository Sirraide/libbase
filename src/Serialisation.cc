#include <base/Serialisation.hh>
#include <cstring>

using namespace base::ser;

template <std::endian E>
auto Reader<E>::read_bytes_into(void* ptr, usz count) -> Result<> {
    if (size() < count) [[unlikely]] {
        return Error(
            "Not enough data to read {} bytes ({} bytes left)",
            count,
            size()
        );
    }

    std::memcpy(ptr, data.data(), count);
    data = data.subspan(count);
    return {};
}

template <std::endian E>
auto Reader<E>::read_bytes(usz count) -> Result<ByteSpan> {
    if (size() < count) [[unlikely]] {
        return Error(
            "Not enough data to read {} bytes ({} bytes left)",
            count,
            size()
        );
    }

    auto span = ByteSpan{data.data(), count};
    data = data.subspan(count);
    return span;
}

template <std::endian E>
auto Writer<E>::allocate(u64 bytes) -> MutableByteSpan {
    auto old_size = data.size();
    data.resize(old_size + bytes);
    return {data.data() + old_size, bytes};
}

template <std::endian E>
void Writer<E>::append_bytes(const void* ptr, usz count) {
    auto* p = static_cast<const std::byte*>(ptr);
    data.insert(data.end(), p, p + count);
}

namespace base::ser {
template class Reader<std::endian::little>;
template class Reader<std::endian::big>;

template class Writer<std::endian::little>;
template class Writer<std::endian::big>;
}
