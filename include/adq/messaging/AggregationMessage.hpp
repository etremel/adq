#pragma once

#include "Message.hpp"
#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "MessageType.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/util/Hash.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>

namespace adq {
namespace messaging {

/** Decorates std::vector with the MessageBody type so it can be the payload of a Message. */
template <typename RecordType>
class AggregationMessageValue : public MessageBody {
private:
    std::vector<RecordType> data;

public:
    static const constexpr MessageBodyType type = MessageBodyType::AGGREGATION_VALUE;
    template <typename... A>
    AggregationMessageValue(A&&... args) : data(std::forward<A>(args)...) {}
    AggregationMessageValue(const AggregationMessageValue&) = default;
    AggregationMessageValue(AggregationMessageValue&&) = default;
    virtual ~AggregationMessageValue() = default;
    operator std::vector<RecordType>() const { return data; }
    operator std::vector<RecordType>&() { return data; }
    // Boilerplate copy-and-pasting of the entire interface of std::vector follows
    decltype(data)::size_type size() const noexcept { return data.size(); }
    template <typename... A>
    void resize(A&&... args) { return data.resize(std::forward<A>(args)...); }
    template <typename... A>
    void emplace_back(A&&... args) { return data.emplace_back(std::forward<A>(args)...); }
    decltype(data)::iterator begin() { return data.begin(); }
    decltype(data)::const_iterator begin() const { return data.begin(); }
    decltype(data)::iterator end() { return data.end(); }
    decltype(data)::const_iterator end() const { return data.end(); }
    decltype(data)::const_iterator cbegin() const { return data.cbegin(); }
    decltype(data)::const_iterator cend() const { return data.cend(); }
    bool empty() const noexcept { return data.empty(); }
    void clear() noexcept { data.clear(); }
    decltype(data)::reference at(decltype(data)::size_type i) { return data.at(i); }
    decltype(data)::const_reference at(decltype(data)::size_type i) const { return data.at(i); }
    decltype(data)::reference operator[](decltype(data)::size_type i) { return data[i]; }
    decltype(data)::const_reference operator[](decltype(data)::size_type i) const { return data[i]; }
    template <typename A>
    AggregationMessageValue<RecordType>& operator=(A&& other) {
        data.operator=(std::forward<A>(other));
        return *this;
    }
    // An argument of type AggregationMessageValue won't forward to std::vector::operator=
    AggregationMessageValue<RecordType>& operator=(const AggregationMessageValue<RecordType>& other) {
        data = other.data;
        return *this;
    }
    // This is the only method that differs from std::vector
    inline bool operator==(const MessageBody& _rhs) const {
        if(auto* rhs = dynamic_cast<const AggregationMessageValue<RecordType>*>(&_rhs))
            return this->data == rhs->data;
        else
            return false;
    }
    // Forward the serialization methods to the already-implemented ones for std::vector
    std::size_t bytes_size() const {
        return mutils::bytes_size(type) + mutils::bytes_size(data);
    }
    std::size_t to_bytes(uint8_t* buffer) const {
        std::size_t bytes_written = mutils::to_bytes(type, buffer);
        return bytes_written + mutils::to_bytes(data, buffer + bytes_written);
    }
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const {
        mutils::post_object(f, type);
        mutils::post_object(f, data);
    }
    static std::unique_ptr<AggregationMessageValue<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
        /*"Skip past the MessageBodyType, then take the deserialized vector
         * and wrap it in a new AggregationMessageValue"*/
        return std::make_unique<AggregationMessageValue<RecordType>>(
            *mutils::from_bytes<std::vector<RecordType>>(m, buffer + sizeof(type)));
    }
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AggregationMessageValue<RecordType>& v);

/**
 * The messages sent in the Aggregate phase of all versions of the protocol.
 * They carry the query result (or its intermediate value) and the count of
 * data points that contributed to the result.
 */
template <typename RecordType>
class AggregationMessage : public Message {
private:
    int num_contributors;

public:
    static const constexpr MessageType type = MessageType::AGGREGATION;
    using body_type = AggregationMessageValue<RecordType>;
    int query_num;
    AggregationMessage() : Message(0, nullptr), num_contributors(0), query_num(0) {}
    AggregationMessage(const int sender_id, const int query_num, std::shared_ptr<AggregationMessageValue> value)
        : Message(sender_id, value), num_contributors(1), query_num(query_num) {}
    virtual ~AggregationMessage() = default;
    std::shared_ptr<body_type> get_body() { return std::static_pointer_cast<body_type>(body); };
    const std::shared_ptr<body_type> get_body() const { return std::static_pointer_cast<body_type>(body); };
    void add_value(const RecordType& value, int num_contributors);
    void add_values(const std::vector<RecordType>& values, const int num_contributors);
    int get_num_contributors() const { return num_contributors; }

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>&) const;
    static std::unique_ptr<AggregationMessage> from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer);

    friend bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs);
    friend struct std::hash<AggregationMessage>;

private:
    // All-member constructor used only be deserialization
    AggregationMessage(const int sender_id, const int query_num,
                       std::shared_ptr<AggregationMessageValue<RecordType>> value, const int num_contributors) : Message(sender_id, value), num_contributors(num_contributors), query_num(query_num) {}
};

template <typename RecordType>
bool operator==(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs);

template <typename RecordType>
bool operator!=(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs);

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AggregationMessage<RecordType>& m);

} /* namespace messaging */
} /* namespace adq */

namespace std {

template <typename RecordType>
struct hash<adq::messaging::AggregationMessageValue<RecordType>> {
    size_t operator()(const adq::messaging::AggregationMessageValue<RecordType>& input) const {
        using adq::util::hash_combine;

        std::size_t seed = input.size();
        for(const auto& i : input) {
            hash_combine(seed, i);
        }
        return seed;
    }
};

template <typename RecordType>
struct hash<adq::messaging::AggregationMessage<RecordType>> {
    size_t operator()(const adq::messaging::AggregationMessage<RecordType>& input) const {
        using adq::util::hash_combine;

        size_t result = 1;
        hash_combine(result, input.num_contributors);
        hash_combine(result, input.query_num);
        hash_combine(result, *std::static_pointer_cast<adq::messaging::AggregationMessageValue<RecordType>>(input.body));
        return result;
    }
};

}  // namespace std

#include "detail/AggregationMessage_impl.hpp"
