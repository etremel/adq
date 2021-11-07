#pragma once

namespace adq {
/**
 * Represents a source of data that a query will collect from a client.
 * Each client contains a DataSource that it reads data from in response
 * to a query. The type of data contained in a record depends on the application.
 */
template<typename RecordType>
class DataSource {
};
}  // namespace adq

#include "detail/DataSource_impl.hpp"