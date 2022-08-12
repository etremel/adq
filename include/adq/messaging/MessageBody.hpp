#pragma once

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <memory>

namespace adq {

namespace messaging {

/**
 * Superclass for all types that can be the body of a message. Ensures that all
 * message bodies are subtypes of ByteRepresentable, and provides an "interface"
 * version of from_bytes (not actually virtual) that manually implements dynamic
 * dispatch to the correct subclass's version of from_bytes.
 *
 * @tparam RecordType The data type being collected by queries in this instantiation
 * of the query system; needed because some types of message body will contain
 * instances of this data type.
 */
template <typename RecordType>
class MessageBody : public mutils::ByteRepresentable {
public:
    virtual ~MessageBody() = default;
    virtual bool operator==(const MessageBody<RecordType>&) const = 0;

    static std::unique_ptr<MessageBody<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

template<typename RecordType>
bool operator!=(const MessageBody<RecordType>& a, const MessageBody<RecordType>& b) {
    return !(a == b);
}

} /* namespace messaging */
} /* namespace adq */

#include "detail/MessageBody_impl.hpp"