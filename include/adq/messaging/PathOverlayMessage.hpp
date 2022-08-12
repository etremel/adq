#pragma once

#include "OverlayMessage.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

/**
 * Represents a non-onion-encrypted OverlayMessage that must traverse a path
 * through the overlay. The destination field should contain the ID of the next
 * destination on the path, and the remaining_path field should contain a list
 * of the IDs to forward to after destination. This is another possible body of
 * an OverlayTransportMessage.
 *
 * @tparam RecordType The data type being collected by queries in this instantiation
 * of the query system; needed because some types of message will contain instances
 * of this data type.
 */
template <typename RecordType>
class PathOverlayMessage : public OverlayMessage<RecordType> {
public:
    static const constexpr MessageBodyType type = MessageBodyType::PATH_OVERLAY;
    std::list<int> remaining_path;
    PathOverlayMessage(const int query_num, const std::list<int>& path, std::shared_ptr<MessageBody<RecordType>> body)
        : OverlayMessage<RecordType>(query_num, path.front(), std::move(body)),
          remaining_path(++path.begin(), path.end()) {}
    virtual ~PathOverlayMessage() = default;

    // Serialization support
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& consumer_function) const;
    std::size_t bytes_size() const;
    static std::unique_ptr<PathOverlayMessage<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);

protected:
    /** Default constructor, used only by deserialization.*/
    PathOverlayMessage() : OverlayMessage<RecordType>() {}
    // Tell C++ I want to use inheritance
    using OverlayMessage<RecordType>::to_bytes_common;
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const PathOverlayMessage<RecordType>& message);

}  // namespace messaging
}  // namespace adq

#include "detail/PathOverlayMessage_impl.hpp"