#pragma once

#include <adq/core/InternalTypes.hpp>
#include <adq/util/Hash.hpp>
#include <adq/util/OStreams.hpp>
#include <ostream>
#include <vector>

namespace adq {

namespace messaging {

struct ValueTuple {
    int query_num;
    std::vector<FixedPoint_t> value;
    std::vector<int> proxies;
    //Member-by-member constructor should do the obvious thing
    ValueTuple(const int query_num, const std::vector<FixedPoint_t>& value, const std::vector<int>& proxies) : query_num(query_num), value(value), proxies(proxies) {}
};

inline std::ostream& operator<<(std::ostream& stream, const ValueTuple& tuple) {
    return stream << "(" << tuple.query_num << ", " << tuple.value << ", " << tuple.proxies << ")";
}

//Equality operator should do the obvious thing.
inline bool operator==(const ValueTuple& lhs, const ValueTuple& rhs) {
    return lhs.query_num == rhs.query_num && lhs.value == rhs.value && lhs.proxies == rhs.proxies;
}

}  // namespace messaging

}  // namespace adq

namespace std {

template <>
struct hash<adq::messaging::ValueTuple> {
    size_t operator()(const adq::messaging::ValueTuple& input) const {
        using adq::util::hash_combine;
        size_t result = 1;
        hash_combine(result, input.query_num);
        hash_combine(result, input.value);
        hash_combine(result, input.proxies);
        return result;
    }
};

}  // namespace std
