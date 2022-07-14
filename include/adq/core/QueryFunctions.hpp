#pragma once

#include <functional>
#include <cstdint>
#include <vector>

namespace adq {

/**
 * Type of a function that selects a single record from a data source based on some
 * parameters. The parameters are serialized as bytes, so it's up to the application
 * to deserialize them.
 *
 * @tparam RecordType The type of a data point or "record" that can be read from a data source
 */
template<typename RecordType>
using SelectFunction = std::function<RecordType(const uint8_t* const)>;

/**
 * Type of a function that determines whether a record should be included in a query,
 * based on some serialized parameters. The application must deserialize the parameters
 * from the byte buffer in order to use them.
 *
 * @tparam RecordType The type of a data point or "record" that can be read from a data source
 */
template<typename RecordType>
using FilterFunction = std::function<bool(const RecordType&, const uint8_t* const)>;

/**
 * Type of a function that combines multiple records (data points) read from a data
 * source into a single record, with (optionally) some additional arguments that can
 * determine how the records should be aggregated. The additional parameters are
 * serialized as bytes, so it's up to the application to deserialize them.
 *
 * @tparam RecordType The type of a data point or "record" that can be read from a data source
 */
template<typename RecordType>
using AggregateFunction = std::function<RecordType(const std::vector<RecordType>&, const uint8_t* const)>;

using Opcode = uint32_t;

}