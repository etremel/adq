#include "adq/messaging/ByteBody.hpp"

#include "adq/messaging/MessageBody.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <iomanip>
#include <iterator>
#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

const constexpr MessageBodyType ByteBody::type;

ByteBody& ByteBody::operator=(const ByteBody& other) {
    bytes = other.bytes;
    return *this;
}
ByteBody& ByteBody::operator=(ByteBody&& other) {
    bytes = std::move(other.bytes);
    return *this;
}
bool ByteBody::operator==(const MessageBody& rhs) const {
    if(auto* rhs_cast = dynamic_cast<const ByteBody*>(&rhs)) {
        return this->bytes == rhs_cast->bytes;
    } else {
        return false;
    }
}

std::size_t ByteBody::bytes_size() const {
    return mutils::bytes_size(type) + mutils::bytes_size(bytes);
}
std::size_t ByteBody::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    return bytes_written + mutils::to_bytes(bytes, buffer + bytes_written);
}
void ByteBody::post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const {
    mutils::post_object(f, type);
    mutils::post_object(f, bytes);
}
std::unique_ptr<ByteBody> ByteBody::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    // Skip past the MessageBodyType, then deserialize the vector
    return std::make_unique<ByteBody>(
        *mutils::from_bytes<std::vector<uint8_t>>(m, buffer + sizeof(type)));
}

std::ostream& operator<<(std::ostream& stream, const ByteBody& v) {
    if(v.size() > 0) {
        std::ios normal_stream_state(nullptr);
        normal_stream_state.copyfmt(stream);
        stream << '[' << std::hex << std::setfill('0') << std::setw(2) << std::right;
        std::copy(v.bytes.begin(), v.bytes.end(), std::ostream_iterator<uint8_t>(stream, ", "));
        stream << "\b\b]";
        stream.copyfmt(normal_stream_state);
    }
    return stream;
}

}  // namespace messaging
}  // namespace adq