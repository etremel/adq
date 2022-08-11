#include "../ValueContribution.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstring>
#include <memory>
#include <vector>

namespace adq {

namespace messaging {

template <typename RecordType>
const constexpr MessageBodyType ValueContribution<RecordType>::type;

template <typename RecordType>
std::size_t ValueContribution<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(value_tuple, buffer + bytes_written);
    std::memcpy(buffer + bytes_written, signature.data(), signature.size() * sizeof(SignatureArray::value_type));
    bytes_written += signature.size() * sizeof(SignatureArray::value_type);
    return bytes_written;
}

template <typename RecordType>
void ValueContribution<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& consumer_function) const {
    mutils::post_object(consumer_function, type);
    mutils::post_object(consumer_function, value_tuple);
    consumer_function((const uint8_t*)signature.data(), signature.size() * sizeof(SignatureArray::value_type));
}

template <typename RecordType>
std::size_t ValueContribution<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) + mutils::bytes_size(value_tuple) +
           (signature.size() * sizeof(SignatureArray::value_type));
}

template <typename RecordType>
std::unique_ptr<ValueContribution<RecordType>> ValueContribution<RecordType>::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);

    auto value_tuple = mutils::from_bytes<ValueTuple<RecordType>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*value_tuple);

    SignatureArray signature;
    std::memcpy(signature.data(), buffer + bytes_read, signature.size() * sizeof(SignatureArray::value_type));
    bytes_read += signature.size() * sizeof(SignatureArray::value_type);

    // This will unnecessarily copy the vectors into the new ValueTuple, but
    // it's the only thing we can do because from_bytes returns unique_ptr
    return std::make_unique<ValueContribution<RecordType>>(*value_tuple, signature);
}
}  // namespace messaging

}  // namespace adq