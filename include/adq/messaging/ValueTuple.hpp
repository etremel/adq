#pragma once

#include "adq/core/InternalTypes.hpp"
#include "adq/mutils-serialization/SerializationMacros.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/util/Hash.hpp"
#include "adq/util/OStreams.hpp"

#include <ostream>
#include <vector>

namespace adq {

namespace messaging {

template <typename RecordType>
struct ValueTuple : public mutils::ByteRepresentable {
    int query_num;
    RecordType value;
    std::vector<int> proxies;
    // Member-by-member constructor should do the obvious thing
    ValueTuple(const int query_num, const RecordType& value, const std::vector<int>& proxies)
        : query_num(query_num), value(value), proxies(proxies) {}

    DEFAULT_SERIALIZATION_SUPPORT(ValueTuple, query_num, value, proxies);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& stream, const ValueTuple<RecordType>& tuple) {
    return stream << "(" << tuple.query_num << ", " << tuple.value << ", " << tuple.proxies << ")";
}

// Equality operator should do the obvious thing.
template <typename RecordType>
bool operator==(const ValueTuple<RecordType>& lhs, const ValueTuple<RecordType>& rhs) {
    return lhs.query_num == rhs.query_num && lhs.value == rhs.value && lhs.proxies == rhs.proxies;
}

}  // namespace messaging

}  // namespace adq

namespace std {

template <typename RecordType>
struct hash<adq::messaging::ValueTuple<RecordType>> {
    size_t operator()(const adq::messaging::ValueTuple<RecordType>& input) const {
        using adq::util::hash_combine;
        size_t result = 1;
        hash_combine(result, input.query_num);
        hash_combine(result, input.value);
        hash_combine(result, input.proxies);
        return result;
    }
};

}  // namespace std
