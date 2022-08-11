#pragma once

#include "adq/core/QueryFunctions.hpp"

#include <map>

namespace adq {
/**
 * Represents the application-specific source of data that a query will collect
 * from a client. Each client contains a DataSource that it will use to handle
 * query requests. The "interface" that clients will use to process queries is
 * the maps from numeric opcodes to select, filter, and aggregate functions.
 * The application should subclass DataSource and fill in these maps with function
 * pointers to its own application-specific logic.
 *
 * @tparam RecordType The type of data contained in an individual record that a
 * query could retrieve. RecordType must be default-constructable and implement
 * the ByteRepresentable interface.
 */
template <typename RecordType>
class DataSource {
public:
    std::map<Opcode, SelectFunction<RecordType>> select_functions;
    std::map<Opcode, FilterFunction<RecordType>> filter_functions;
    std::map<Opcode, AggregateFunction<RecordType>> aggregate_functions;
};
}  // namespace adq