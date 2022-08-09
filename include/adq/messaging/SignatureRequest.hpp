#pragma once

#include "Message.hpp"
#include "MessageType.hpp"
#include "ByteBody.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace adq {
namespace messaging {

class SignatureRequest : public Message {
public:
    static const constexpr MessageType type = MessageType::SIGNATURE_REQUEST;
    using body_type = ByteBody;
    SignatureRequest(const int sender_id, std::shared_ptr<ByteBody> encrypted_value)
        : Message(sender_id, std::move(encrypted_value)) {}
    virtual ~SignatureRequest() = default;

    // Trivial extension of Message, just call superclass's serialization methods
    std::size_t bytes_size() const {
        return mutils::bytes_size(type) + Message::bytes_size();
    }
    std::size_t to_bytes(uint8_t* buffer) const {
        std::size_t bytes_written = mutils::to_bytes(type, buffer);
        bytes_written += Message::to_bytes(buffer + bytes_written);
        return bytes_written;
    }
    void post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
        mutils::post_object(function, type);
        Message::post_object(function);
    }
    static std::unique_ptr<SignatureRequest> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
        std::size_t bytes_read = 0;
        MessageType message_type;
        std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
        bytes_read += sizeof(MessageType);

        int sender_id;
        std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
        bytes_read += sizeof(sender_id);
        std::shared_ptr<body_type> body_shared = mutils::from_bytes<body_type>(m, buffer + bytes_read);
        return std::make_unique<SignatureRequest>(sender_id, body_shared);
    }
};

}  // namespace messaging
}  // namespace adq
