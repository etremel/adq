#pragma once

#include "../OverlayMessage.hpp"

#include "adq/core/CryptoLibrary.hpp"
#include "adq/messaging/AgreementValue.hpp"
#include "adq/messaging/ByteBody.hpp"
#include "adq/messaging/MessageBody.hpp"
#include "adq/messaging/PathOverlayMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <ostream>

namespace adq {

namespace messaging {

template <typename RecordType>
const constexpr MessageBodyType OverlayMessage<RecordType>::type;

template <typename RecordType>
bool OverlayMessage<RecordType>::operator==(const MessageBody<RecordType>& _rhs) const {
    auto lhs = this;
    if(auto* rhs = dynamic_cast<const OverlayMessage<RecordType>*>(&_rhs))
        return lhs->query_num == rhs->query_num &&
               lhs->destination == rhs->destination &&
               lhs->is_encrypted == rhs->is_encrypted &&
               lhs->flood == rhs->flood &&
               (lhs->enclosed_body == nullptr ? rhs->enclosed_body == nullptr
                                              : (rhs->enclosed_body != nullptr &&
                                                 *lhs->enclosed_body == *rhs->enclosed_body));
    else
        return false;
}

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const OverlayMessage<RecordType>& message) {
    out << "{QueryNum=" << message.query_num << "|Destination=" << message.destination << "|Body=";
    // Force C++ to use dynamic dispatch on operator<< even though it doesn't want to
    if(message.enclosed_body == nullptr) {
        out << "null";
    } else if(auto av_body = std::dynamic_pointer_cast<AgreementValue<RecordType>>(message.enclosed_body)) {
        out << *av_body;
    } else if(auto pom_body = std::dynamic_pointer_cast<PathOverlayMessage<RecordType>>(message.enclosed_body)) {
        out << *pom_body;
    } else if(auto om_body = std::dynamic_pointer_cast<OverlayMessage<RecordType>>(message.enclosed_body)) {
        out << *om_body;
    } else if(auto sv_body = std::dynamic_pointer_cast<SignedValue<RecordType>>(message.enclosed_body)) {
        out << *sv_body;
    } else if(auto byte_body = std::dynamic_pointer_cast<ByteBody<RecordType>>(message.enclosed_body)) {
        out << *byte_body;
    } else if(auto vc_body = std::dynamic_pointer_cast<ValueContribution<RecordType>>(message.enclosed_body)) {
        out << *vc_body;
    } else {
        out << "UNKNOWN TYPE @ " << message.enclosed_body;
    }
    out << "}";
    return out;
}

template <typename RecordType>
std::size_t OverlayMessage<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) +
           mutils::bytes_size(query_num) +
           mutils::bytes_size(destination) +
           mutils::bytes_size(is_encrypted) +
           mutils::bytes_size(flood) +
           mutils::bytes_size(false)  // Represents the "remaining_body" variable
           + (enclosed_body == nullptr ? 0 : mutils::bytes_size(*enclosed_body));
}

template <typename RecordType>
void OverlayMessage<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& consumer_function) const {
    uint8_t buffer[bytes_size()];
    to_bytes(buffer);
    consumer_function(buffer, bytes_size());
}

template <typename RecordType>
std::size_t OverlayMessage<RecordType>::to_bytes_common(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    // For POD types, this is just a shortcut to memcpy(buffer+bytes_written, &var, sizeof(var))
    bytes_written += mutils::to_bytes(query_num, buffer + bytes_written);
    bytes_written += mutils::to_bytes(destination, buffer + bytes_written);
    bytes_written += mutils::to_bytes(is_encrypted, buffer + bytes_written);
    bytes_written += mutils::to_bytes(flood, buffer + bytes_written);

    bool remaining_body = (enclosed_body != nullptr);
    bytes_written += mutils::to_bytes(remaining_body, buffer + bytes_written);
    if(remaining_body) {
        bytes_written += mutils::to_bytes(*enclosed_body, buffer + bytes_written);
    }
    return bytes_written;
}

template <typename RecordType>
std::size_t OverlayMessage<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    bytes_written += to_bytes_common(buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
std::unique_ptr<OverlayMessage<RecordType>> OverlayMessage<RecordType>::from_bytes(mutils::DeserializationManager* p, uint8_t const* buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);

    // We can't use the private constructor with make_unique
    auto constructed_message = std::unique_ptr<OverlayMessage>(new OverlayMessage());
    bytes_read += from_bytes_common(*constructed_message, buffer + bytes_read);
    return constructed_message;
}

template <typename RecordType>
std::size_t OverlayMessage<RecordType>::from_bytes_common(OverlayMessage& partial_overlay_message, uint8_t const* buffer) {
    std::size_t bytes_read = 0;
    std::memcpy(&partial_overlay_message.query_num, buffer + bytes_read, sizeof(partial_overlay_message.query_num));
    bytes_read += sizeof(partial_overlay_message.query_num);
    std::memcpy(&partial_overlay_message.destination, buffer + bytes_read, sizeof(partial_overlay_message.destination));
    bytes_read += sizeof(partial_overlay_message.destination);
    std::memcpy(&partial_overlay_message.is_encrypted, buffer + bytes_read, sizeof(partial_overlay_message.is_encrypted));
    bytes_read += sizeof(partial_overlay_message.is_encrypted);
    std::memcpy(&partial_overlay_message.flood, buffer + bytes_read, sizeof(partial_overlay_message.flood));
    bytes_read += sizeof(partial_overlay_message.flood);

    bool remaining_body;
    std::memcpy(&remaining_body, buffer + bytes_read, sizeof(remaining_body));
    bytes_read += sizeof(remaining_body);
    if(remaining_body) {
        partial_overlay_message.enclosed_body = mutils::from_bytes<MessageBody<RecordType>>(nullptr, buffer + bytes_read);
        bytes_read += mutils::bytes_size(*partial_overlay_message.enclosed_body);
    }

    return bytes_read;
}

template <typename RecordType>
std::shared_ptr<OverlayMessage<RecordType>> build_encrypted_onion(const std::list<int>& path, std::shared_ptr<MessageBody<RecordType>> payload,
                                                                  const int query_num, CryptoLibrary& crypto_library) {
    // Start with the last layer of the onion, which actually contains the payload
    auto current_layer = std::make_shared<OverlayMessage<RecordType>>(query_num, path.back(), std::move(payload));
    crypto_library.rsa_encrypt(*current_layer, path.back());
    // Build the onion from the end of the list to the beginning
    for(auto path_iter = path.rbegin(); path_iter != path.rend(); ++path_iter) {
        if(path_iter == path.rbegin())
            continue;
        // Make the previous layer the payload of a new message, whose destination is the next hop
        auto next_layer = std::make_shared<OverlayMessage<RecordType>>(query_num, *path_iter, current_layer);
        crypto_library.rsa_encrypt(*next_layer, *path_iter);
        current_layer = next_layer;
    }
    return current_layer;
}

}  // namespace messaging

}  // namespace adq