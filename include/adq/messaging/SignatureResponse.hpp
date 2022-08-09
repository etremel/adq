/*
 * SignatureResponse.h
 *
 *  Created on: May 23, 2016
 *      Author: edward
 */

#pragma once


#include "Message.hpp"
#include "MessageType.hpp"
#include "ByteBody.hpp"

#include <memory>
#include <cstring>
#include <cstdint>
#include <ostream>

namespace adq {
namespace messaging {

/**
 * Trivial subclass of Message that specializes its body to be a ByteBody.
 */
class SignatureResponse : public Message {
public:
    static const constexpr MessageType type = MessageType::SIGNATURE_RESPONSE;
    using body_type = ByteBody;
    SignatureResponse(const int sender_id, std::shared_ptr<ByteBody> encrypted_response)
        : Message(sender_id, std::move(encrypted_response)){};
    virtual ~SignatureResponse() = default;

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
    static std::unique_ptr<SignatureResponse> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
        std::size_t bytes_read = 0;
        MessageType message_type;
        std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
        bytes_read += sizeof(MessageType);

        int sender_id;
        std::memcpy(&sender_id, buffer + bytes_read, sizeof(int));
        bytes_read += sizeof(int);
        std::shared_ptr<body_type> body_shared = mutils::from_bytes<body_type>(m, buffer + bytes_read);
        return std::make_unique<SignatureResponse>(sender_id, body_shared);
    }
};

inline std::ostream& operator<<(std::ostream& out, const SignatureResponse& message) {
    return out << "SignatureResponse with body: " << *(std::static_pointer_cast<SignatureResponse::body_type>(message.body));
}

} /* namespace messaging */
}  // namespace adq
