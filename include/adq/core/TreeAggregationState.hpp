#pragma once

#include "NetworkManager.hpp"
#include "adq/core/DataSource.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/util/PointerUtil.hpp"

#include <memory>

namespace adq {

/**
 * Helper class representing the state machine for the Aggregation phase of any
 * subclass of ProtocolState.
 */
template <typename RecordType>
class TreeAggregationState {
private:
    const int node_id;
    const int num_groups;
    const int num_meters;
    NetworkManager<RecordType>& network;
    const std::shared_ptr<messaging::QueryRequest<RecordType>> current_query;
    bool initialized;
    int children_received_from;
    int children_needed;
    std::shared_ptr<messaging::AggregationMessage<RecordType>> aggregation_intermediate;

public:
    TreeAggregationState(const int node_id, const int num_groups, const int num_meters,
                         NetworkManager<RecordType>& network_client,
                         const std::shared_ptr<messaging::QueryRequest<RecordType>>& query_request)
        : node_id(node_id),
          num_groups(num_groups),
          num_meters(num_meters),
          network(network_client),
          current_query(query_request),
          initialized(false),
          children_received_from(0),
          children_needed(2),
          aggregation_intermediate(std::make_shared<messaging::AggregationMessage<RecordType>>(
              node_id, current_query->query_number,
              std::make_shared<messaging::AggregationMessageValue<RecordType>>(), 0)) {}
    /** Performs initial setup on the tree aggregation state once the aggregation phase starts. */
    void initialize(const std::set<int>& failed_meter_ids);
    bool is_initialized() const { return initialized; }
    bool done_receiving_from_children() const;
    /**
     * Combines the data in an incoming AggregationMessage with the current
     * aggregated value, using the DataSource's aggregation function.
     */
    void handle_message(const messaging::AggregationMessage<RecordType>& message,
                        DataSource<RecordType>& data_source);
    /**
     * Computes this client's contribution to the aggregation phase, by combining
     * the records in accepted_proxy_values using the DataSource's aggregation
     * function, and sends a new AggregationMessage over the network.
     *
     * @param accepted_proxy_values The set of values this client will contribute
     * to the aggregation phase.
     * @param data_source The DataSource provided by the client, which will be
     * used for its AggregateFunction.
     */
    void compute_and_send_aggregate(const util::unordered_ptr_set<messaging::ValueContribution<RecordType>>& accepted_proxy_values,
                                    DataSource<RecordType>& data_source);
};

}  // namespace adq

#include "detail/TreeAggregationState_impl.hpp"