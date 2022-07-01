#pragma once

#include "NetworkManager.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/ValueContribution.hpp"
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
    const std::shared_ptr<messaging::QueryRequest> current_query;
    bool initialized;
    int children_received_from;
    int children_needed;
    std::shared_ptr<messaging::AggregationMessage<RecordType>> aggregation_intermediate;

public:
    TreeAggregationState(const int node_id, const int num_groups, const int num_meters,
                         NetworkManager<RecordType>& network_client, const std::shared_ptr<messaging::QueryRequest>& query_request)
        : node_id(node_id),
          num_groups(num_groups),
          num_meters(num_meters),
          network(network_client),
          current_query(query_request),
          initialized(false),
          children_received_from(0),
          children_needed(2) {}
    /** Performs initial setup on the tree aggregation state, such as initializing
     * the local intermediate aggregate value to an appropriate zero.*/
    void initialize(const int data_array_length, const std::set<int>& failed_meter_ids);
    bool is_initialized() const { return initialized; }
    bool done_receiving_from_children() const;
    void handle_message(const messaging::AggregationMessage<RecordType>& message);
    void compute_and_send_aggregate(const util::unordered_ptr_set<messaging::ValueContribution<RecordType>>& accepted_proxy_values);
};

}  // namespace adq

#include "detail/TreeAggregationState_impl.hpp"