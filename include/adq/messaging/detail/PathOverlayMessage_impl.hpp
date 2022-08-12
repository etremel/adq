#pragma once

#include "../PathOverlayMessage.hpp"

#include "adq/messaging/MessageBodyType.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/util/OStreams.hpp"

#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

namespace adq {
namespace messaging {

template <typename RecordType>
const constexpr MessageBodyType PathOverlayMessage<RecordType>::type;

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const PathOverlayMessage<RecordType>& message) {
    std::stringstream super_streamout;
    super_streamout << static_cast<OverlayMessage<RecordType>>(message);
    std::string output_string = super_streamout.str();
    std::stringstream path_string_builder;
    path_string_builder << "|RemainingPath=" << message.remaining_path;
    output_string.insert(output_string.find("|Body="), path_string_builder.str());
    return out << output_string;
}

template <typename RecordType>
std::size_t PathOverlayMessage<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(remaining_path, buffer + bytes_written);
    bytes_written += to_bytes_common(buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void PathOverlayMessage<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    uint8_t buffer[bytes_size()];
    to_bytes(buffer);
    function(buffer, bytes_size());
}

template <typename RecordType>
std::size_t PathOverlayMessage<RecordType>::bytes_size() const {
    // The superclass bytes_size already includes the size of a MessageBodyType
    return OverlayMessage<RecordType>::bytes_size() + mutils::bytes_size(remaining_path);
}

template <typename RecordType>
std::unique_ptr<PathOverlayMessage<RecordType>> PathOverlayMessage<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);
    assert(type == MessageBodyType::PATH_OVERLAY);

    auto constructed_message = std::unique_ptr<PathOverlayMessage<RecordType>>(new PathOverlayMessage<RecordType>());
    auto deserialized_list = mutils::from_bytes<std::list<int>>(nullptr, buffer + bytes_read);
    constructed_message->remaining_path = *deserialized_list;
    bytes_read += mutils::bytes_size(constructed_message->remaining_path);
    bytes_read += OverlayMessage<RecordType>::from_bytes_common(*constructed_message, buffer + bytes_read);
    return constructed_message;
}

}  // namespace messaging
}  // namespace adq
