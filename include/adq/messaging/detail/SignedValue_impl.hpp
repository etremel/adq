#include "../SignedValue.hpp"

namespace adq {

namespace messaging {

// Voodoo incantations
template<typename RecordType>
const constexpr MessageBodyType SignedValue<RecordType>::type;

// I don't have time to make this generic for all std::maps
template <typename RecordType>
std::size_t SignedValue<RecordType>::bytes_size(const std::map<int, SignatureArray>& sig_map) {
    return sizeof(int) + sig_map.size() * (sizeof(int) +
                                           (RSA_SIGNATURE_SIZE * sizeof(SignatureArray::value_type)));
}

template <typename RecordType>
std::size_t SignedValue<RecordType>::to_bytes(const std::map<int, SignatureArray>& sig_map, uint8_t* buffer) {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(sig_map.size(), buffer);
    for(const auto& entry : sig_map) {
        bytes_written += mutils::to_bytes(entry.first, buffer + bytes_written);
        std::memcpy(buffer + bytes_written, entry.second.data(), entry.second.size() * sizeof(SignatureArray::value_type));
        bytes_written += entry.second.size() * sizeof(SignatureArray::value_type);
    }
    return bytes_written;
}

template <typename RecordType>
std::unique_ptr<std::map<int, SignatureArray>> SignedValue<RecordType>::from_bytes_map(mutils::DeserializationManager* p, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    int size;
    std::memcpy((uint8_t*)&size, buffer, sizeof(size));
    const uint8_t* buf2 = buffer + sizeof(size);
    auto new_map = std::make_unique<std::map<int, SignatureArray>>();
    for(int i = 0; i < size; ++i) {
        int key;
        SignatureArray value;
        std::memcpy((uint8_t*)&key, buf2 + bytes_read, sizeof(key));
        bytes_read += sizeof(key);
        std::memcpy(value.data(), buf2 + bytes_read, value.size() * sizeof(SignatureArray::value_type));
        bytes_read += value.size() * sizeof(SignatureArray::value_type);
        new_map->emplace(key, value);
    }
    return new_map;
}

template <typename RecordType>
std::size_t SignedValue<RecordType>::bytes_size() const {
    // Don't add a sizeof(MessageBodyType) because ValueContribution already adds one
    return mutils::bytes_size(*value) + bytes_size(signatures);
}

template <typename RecordType>
std::size_t SignedValue<RecordType>::to_bytes(uint8_t* buffer) const {
    // Since *value is itself a MessageBody, this will also put a MessageBodyType in the buffer
    std::size_t bytes_written = mutils::to_bytes(*value, buffer);
    // Rewrite the first two bytes of the buffer to change the MessageBodyType
    mutils::to_bytes(type, buffer);
    // Now append the signatures
    bytes_written += to_bytes(signatures, buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void SignedValue<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    uint8_t buffer[bytes_size()];
    to_bytes(buffer);
    function(buffer, bytes_size());
}

template <typename RecordType>
std::unique_ptr<SignedValue<RecordType>> SignedValue<RecordType>::from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer) {
    // Read the ValueContribution, which will also read past the MessageBodyType, and put it in a shared_ptr
    std::shared_ptr<ValueContribution<RecordType>> contribution = mutils::from_bytes<ValueContribution<RecordType>>(p, buffer);
    std::size_t bytes_read = mutils::bytes_size(*contribution);
    std::unique_ptr<std::map<int, SignatureArray>> signatures = from_bytes_map(p, buffer + bytes_read);
    return std::make_unique<SignedValue<RecordType>>(contribution, *signatures);
}

}  // namespace messaging
}  // namespace adq