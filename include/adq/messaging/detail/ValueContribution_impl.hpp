#include "../ValueContribution.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstring>
#include <memory>
#include <vector>

namespace adq {

namespace messaging {

template<typename RecordType>
const constexpr MessageBodyType ValueContribution<RecordType>::type;

template<typename RecordType>
std::size_t ValueContribution<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    //Unpack each member of value and write it individually, since ValueTuple isn't ByteSerializable
    bytes_written += mutils::to_bytes(value.query_num, buffer + bytes_written);
    bytes_written += mutils::to_bytes(value.value, buffer + bytes_written);
    bytes_written += mutils::to_bytes(value.proxies, buffer + bytes_written);
    std::memcpy(buffer + bytes_written, signature.data(), signature.size() * sizeof(SignatureArray::value_type));
    bytes_written += signature.size() * sizeof(SignatureArray::value_type);
    return bytes_written;
}

template<typename RecordType>
void ValueContribution<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& consumer_function) const {
    mutils::post_object(consumer_function, type);
    mutils::post_object(consumer_function, value.query_num);
    mutils::post_object(consumer_function, value.value);
    mutils::post_object(consumer_function, value.proxies);
    consumer_function((const uint8_t*) signature.data(), signature.size() * sizeof(SignatureArray::value_type));
}

template<typename RecordType>
std::size_t ValueContribution<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) + mutils::bytes_size(value.query_num) +
            mutils::bytes_size(value.value) + mutils::bytes_size(value.proxies) +
            (signature.size() * sizeof(SignatureArray::value_type));
}
template<typename RecordType>
std::unique_ptr<ValueContribution<RecordType>> ValueContribution<RecordType>::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);

    int query_num;
    std::memcpy(&query_num, buffer + bytes_read, sizeof(query_num));
    bytes_read += sizeof(query_num);
    std::unique_ptr<RecordType> value_vector =
            mutils::from_bytes<RecordType>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*value_vector);
    std::unique_ptr<std::vector<int>> proxy_vector =
            mutils::from_bytes<std::vector<int>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*proxy_vector);

    SignatureArray signature;
    std::memcpy(signature.data(), buffer + bytes_read, signature.size() * sizeof(SignatureArray::value_type));
    bytes_read += signature.size() * sizeof(SignatureArray::value_type);

    //This will unnecessarily copy the vectors into the new ValueTuple, but
    //it's the only thing we can do because from_bytes returns unique_ptr
    return std::make_unique<ValueContribution<RecordType>>(ValueTuple<RecordType>{query_num, *value_vector, *proxy_vector}, signature);
}
}

}