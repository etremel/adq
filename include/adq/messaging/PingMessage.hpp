/*
 * PingMessage.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <cstddef>
#include <memory>

#include "Message.hpp"
#include "MessageType.hpp"

namespace adq {
namespace messaging {

/**
 * A simple message that represents a ping, asking the recipient to respond if
 * it is still alive.
 *
 * @tparam RecordType The data type being collected in queries in this system.
 * This parameter is ignored by PingMessage and has no effect, but it's
 * required in order to inherit from Message<RecordType>
 */
template <typename RecordType>
class PingMessage : public Message<RecordType> {
public:
    static const constexpr MessageType type = MessageType::PING;
    using Message<RecordType>::sender_id;
    bool is_response;
    PingMessage(const int sender_id, const bool is_response = false)
        : Message<RecordType>(sender_id, nullptr),
          is_response(is_response) {}
    virtual ~PingMessage() = default;

    /**
     * Computes the number of bytes it would take to serialize this message.
     * @return The size of a serialized PingMessage in bytes.
     */
    std::size_t bytes_size() const;
    /**
     * Copies a PingMessage into the byte buffer that {@code buffer} points to,
     * blindly assuming that the buffer is large enough to contain the message.
     * The caller must ensure that the buffer is at least as long as
     * message.bytes_size() before calling this.
     * @param buffer The byte buffer into which this object should be serialized.
     */
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const;
    /**
     * Creates a new PingMessage by deserializing the contents of {@code buffer},
     * blindly assuming that the buffer is large enough to contain a PingMessage.
     * @param buffer A byte buffer containing the results of an earlier call to
     * to_bytes(uint8_t*).
     * @return A new PingMessage reconstructed from the serialized bytes.
     */
    static std::unique_ptr<PingMessage<RecordType>> from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const PingMessage<RecordType>& message);

}  // namespace messaging
}  // namespace adq

#include "detail/PingMessage_impl.hpp"
