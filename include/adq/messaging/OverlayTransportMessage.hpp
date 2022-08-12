#pragma once

#include <cstddef>
#include <memory>
#include <ostream>

#include "Message.hpp"
#include "MessageType.hpp"
#include "OverlayMessage.hpp"

namespace adq {
namespace messaging {

/**
 * The type of message that is sent between nodes during any round of the
 * peer-to-peer overlay. It contains the header fields that change every time a
 * message is sent, and "wraps" a body message that may be relayed unchanged
 * over several rounds of messaging.
 */
template <typename RecordType>
class OverlayTransportMessage : public Message<RecordType> {
public:
    static const constexpr MessageType type = MessageType::OVERLAY;
    /** An OverlayTransportMessage may contain an OverlayMessage or a
     *  subclass of OverlayMessage, but OverlayMessage will at least be
     *  a valid cast target. */
    using body_type = OverlayMessage<RecordType>;
    using Message<RecordType>::body;
    int sender_round;
    bool is_final_message;
    OverlayTransportMessage(const int sender_id, const int sender_round,
                            const bool is_final_message,
                            std::shared_ptr<OverlayMessage<RecordType>> wrapped_message)
        : Message<RecordType>(sender_id, wrapped_message),
          sender_round(sender_round),
          is_final_message(is_final_message){};
    virtual ~OverlayTransportMessage() = default;
    /** Returns a pointer to the message body cast to the correct type for this message */
    std::shared_ptr<body_type> get_body() { return std::static_pointer_cast<body_type>(body); };
    const std::shared_ptr<body_type> get_body() const { return std::static_pointer_cast<body_type>(body); };

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>&) const;
    static std::unique_ptr<OverlayTransportMessage<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const OverlayTransportMessage<RecordType>& message);

} /* namespace messaging */
}  // namespace adq

#include "detail/OverlayTransportMessage_impl.hpp"
