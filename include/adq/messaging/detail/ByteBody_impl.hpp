#pragma once

#include "../ByteBody.hpp"

#include "adq/messaging/MessageBody.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <iomanip>
#include <iterator>
#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

template <typename RecordType>
const constexpr MessageBodyType ByteBody<RecordType>::type;

template <typename RecordType>
ByteBody<RecordType>& ByteBody<RecordType>::operator=(const ByteBody<RecordType>& other) {
    bytes = other.bytes;
    return *this;
}
template <typename RecordType>
ByteBody<RecordType>& ByteBody<RecordType>::operator=(ByteBody<RecordType>&& other) {
    bytes = std::move(other.bytes);
    return *this;
}
template <typename RecordType>
bool ByteBody<RecordType>::operator==(const MessageBody<RecordType>& rhs) const {
    if(auto* rhs_cast = dynamic_cast<const ByteBody<RecordType>*>(&rhs)) {
        return this->bytes == rhs_cast->bytes;
    } else {
        return false;
    }
}

template <typename RecordType>
std::size_t ByteBody<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) + mutils::bytes_size(bytes);
}
template <typename RecordType>
std::size_t ByteBody<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    return bytes_written + mutils::to_bytes(bytes, buffer + bytes_written);
}
template <typename RecordType>
void ByteBody<RecordType>::post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const {
    mutils::post_object(f, type);
    mutils::post_object(f, bytes);
}
template <typename RecordType>
std::unique_ptr<ByteBody<RecordType>> ByteBody<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    // Skip past the MessageBodyType, then deserialize the vector
    return std::make_unique<ByteBody<RecordType>>(
        *mutils::from_bytes<std::vector<uint8_t>>(m, buffer + sizeof(type)));
}

}  // namespace messaging
}  // namespace adq