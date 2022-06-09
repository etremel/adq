#pragma once

#include <functional>
#include <cstdint>
#include <vector>

namespace adq {

//The function's arguments will still be serialized as bytes; it's up to the application to deserialize them
template<typename RecordType>
using SelectFunction = std::function<RecordType(const uint8_t* const)>;

template<typename RecordType>
using FilterFunction = std::function<bool(const RecordType&, const uint8_t* const)>;

template<typename RecordType>
using AggregateFunction = std::function<RecordType(const std::vector<RecordType>&, const uint8_t* const)>;

using Opcode = uint32_t;

}