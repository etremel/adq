#pragma once

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/util/Hash.hpp"

#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

/**
 * This is the payload of an OverlayTransportMessage, which may contain as its body
 * another OverlayMessage if the message is an encrypted onion. Each time an
 * OverlayMessage is relayed to another node, it is "wrapped" in a new
 * OverlayTransportMessage.
 *
 * @tparam RecordType The data type being collected by queries in this instantiation
 * of the query system; needed because some types of message will contain instances
 * of this data type.
 */
template <typename RecordType>
class OverlayMessage : public MessageBody<RecordType> {
public:
    static const constexpr MessageBodyType type = MessageBodyType::OVERLAY;
    int query_num;
    int destination;
    bool is_encrypted;
    /**  True if this message should be sent out on every round, regardless of destination */
    bool flood;
    /**
     * A "continued" or "enclosed" body for this message body, which will be
     * either a ValueContribution (the final payload) or another OverlayMessage
     * (if this is an onion-encrypted message).
     */
    std::shared_ptr<MessageBody<RecordType>> enclosed_body;
    OverlayMessage(const int query_num, const int dest_id, std::shared_ptr<MessageBody<RecordType>> body, const bool flood = false)
        : query_num(query_num),
          destination(dest_id),
          is_encrypted(false),
          flood(flood),
          enclosed_body(std::move(body)) {}
    virtual ~OverlayMessage() = default;

    bool operator==(const MessageBody<RecordType>& _rhs) const override;

    /**
     * Computes the number of bytes it would take to serialize this message.
     * @return The size of this message when serialized, in bytes.
     */
    std::size_t bytes_size() const;

    /**
     * Copies an OverlayMessage into the byte buffer that {@code buffer}
     * points to, blindly assuming that the buffer is large enough to contain the
     * message. The caller must ensure that the buffer is at least as long as
     * bytes_size(message) before calling this.
     * @param buffer The byte buffer into which it should be serialized.
     * @return The number of bytes written; should be equal to bytes_size()
     */
    std::size_t to_bytes(uint8_t* buffer) const;

    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& consumer_function) const;

    /**
     * Creates a new OverlayMessage by deserializing the contents of
     * {@code buffer}, blindly assuming that the buffer is large enough to contain
     * the OverlayMessage and its enclosed body (if present).
     * @param buffer A byte buffer containing the results of an earlier call to
     * OverlayMessage::to_bytes(uint8_t*).
     * @return A new OverlayMessage reconstructed from the serialized bytes.
     */
    static std::unique_ptr<OverlayMessage<RecordType>> from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer);

protected:
    /** Default constructor, used only when reconstructing serialized messages */
    OverlayMessage() : query_num(0), destination(0), is_encrypted(false), flood(false), enclosed_body(nullptr) {}
    /**
     * Helper method for implementing to_bytes; serializes the superclass
     * fields (from OverlayMessage) into the given buffer. Subclasses can
     * call this to get OverlayMessage's implementation of to_bytes
     * *without* getting a MessageBodyType inserted into the buffer.
     * @param buffer The byte buffer into which OverlayMessage fields should
     * be serialized.
     * @return The number of bytes written by this method.
     */
    std::size_t to_bytes_common(uint8_t* buffer) const;

    /**
     * Helper method for implementing from_bytes; deserializes the superclass
     * (OverlayMessage) fields from buffer into the corresponding fields of
     * the provided partially-constructed OverlayMessage object. Assumes
     * that the pointer to the buffer is already offset to the correct
     * position at which to start reading OverlayMessage fields. Subclasses
     * can call this to get OverlayMessgae's implementation of from_bytes
     * without the overhead of creating an extra copy of the message.
     * @param partial_overlay_message An OverlayMessage whose fields should
     * be updated to contain the values in the buffer
     * @param buffer The byte buffer containing serialized OverlayMessage fields
     * @return The number of bytes read from the buffer during deserialization.
     */
    static std::size_t from_bytes_common(OverlayMessage<RecordType>& partial_overlay_message, const uint8_t* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const OverlayMessage<RecordType>& message);

/**
 * Constructs an OverlayMessage that will carry a payload (body) along a specific
 * path. It will be successively encrypted with the public keys of each node in
 * the path, so that it must be decrypted by each node in order before it can be
 * read.
 *
 * @param path The sequence of node IDs that the OverlayMessage will pass through
 * @param payload The message body that should be contained in the innermost layer
 * of encryption
 * @param query_num The query number (epoch) in which this message is being sent
 * @param crypto_library The cryptography library that should be used to encrypt
 * the message
 * @return A new encrypted OverlayMessage
 */
template <typename RecordType>
std::shared_ptr<OverlayMessage<RecordType>> build_encrypted_onion(const std::list<int>& path,
                                                                  std::shared_ptr<MessageBody<RecordType>> payload,
                                                                  const int query_num,
                                                                  CryptoLibrary& crypto_library);

} /* namespace messaging */
}  // namespace adq

namespace std {

template <typename RecordType>
struct hash<adq::messaging::OverlayMessage<RecordType>> {
    size_t operator()(const adq::messaging::OverlayMessage<RecordType>& input) const {
        using adq::util::hash_combine;

        size_t result = 1;
        hash_combine(result, input.query_num);
        hash_combine(result, input.destination);
        hash_combine(result, input.is_encrypted);
        hash_combine(result, input.flood);
        hash_combine(result, input.enclosed_body);
        return result;
    }
};

}  // namespace std

#include "detail/OverlayMessage_impl.hpp"