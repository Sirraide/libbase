#include <base/Serialisation.hh>
#include <cstring>

using namespace base::ser;

template <std::endian E>
auto Reader<E>::copy(void* ptr, usz count) -> usz {
    if (size() < count or not result) [[unlikely]] {
        result = Error("Not enough data to read {} bytes ({} bytes left)", count, size());
        return 0;
    }

    std::memcpy(ptr, data.data(), count);
    data = data.subspan(count);
    return count;
}

template <std::endian E>
auto Reader<E>::read_bytes(usz count) -> std::span<const std::byte> {
    if (size() < count or not result) [[unlikely]] {
        result = Error("Not enough data to read {} bytes ({} bytes left)", count, size());
        return {};
    }

    auto span = std::span{data.data(), count};
    data = data.subspan(count);
    return span;
}

template <std::endian E>
auto Writer<E>::allocate(u64 bytes) -> std::span<std::byte> {
    auto old_size = data.size();
    data.resize(old_size + bytes);
    return {data.data() + old_size, bytes};
}

template <std::endian E>
void Writer<E>::append(const void* ptr, usz count) {
    auto* p = static_cast<const std::byte*>(ptr);
    data.insert(data.end(), p, p + count);
}

template <std::endian E>
auto Reader<E>::operator>>(float& t) -> Reader& {
    t = std::bit_cast<float>(read<u32>());
    return *this;
}

template <std::endian E>
auto Reader<E>::operator>>(double& t) -> Reader& {
    t = std::bit_cast<double>(read<u64>());
    return *this;
}

template <std::endian E>
auto Writer<E>::operator<<(float t) -> Writer& {
    *this << std::bit_cast<u32>(t);
    return *this;
}

template <std::endian E>
auto Writer<E>::operator<<(double t) -> Writer& {
    *this << std::bit_cast<u64>(t);
    return *this;
}

namespace base::ser {
template class Reader<std::endian::little>;
template class Reader<std::endian::big>;

template class Writer<std::endian::little>;
template class Writer<std::endian::big>;
}
