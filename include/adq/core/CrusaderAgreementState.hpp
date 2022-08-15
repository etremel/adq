#pragma once

#include "adq/core/InternalTypes.hpp"
#include "adq/util/PointerUtil.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace adq {

/**
 * State machine for the Crusader Agreement (2-phase Byzantine Agreement) phase
 * of the BFT query protocol. Keeps track of intermediate state of the
 * protocol and handles transitions based on received messages.
 */
template <typename RecordType>
class CrusaderAgreementState {
private:
    const int node_id;
    const int num_nodes;
    const int log2n;
    const int query_num;
    bool phase_1_finished;
    CryptoLibrary& crypto_library;
    // I want my map keys to be ValueContributions, but I have to store them by
    // pointer because this map doesn't own them. This mess is the result.
    std::unordered_map<
        std::shared_ptr<messaging::ValueContribution<RecordType>>,
        messaging::SignedValue<RecordType>,
        util::ptr_hash<messaging::ValueContribution<RecordType>>,
        util::ptr_equal<messaging::ValueContribution<RecordType>>>
        signed_proxy_values;

public:
    CrusaderAgreementState(const int node_id, const int num_nodes, const int query_num, CryptoLibrary& crypto_library)
        : node_id(node_id),
          num_nodes(num_nodes),
          log2n((int)std::ceil(std::log2(num_nodes))),
          query_num(query_num),
          phase_1_finished(false),
          crypto_library(crypto_library) {}

    bool is_phase1_finished() { return phase_1_finished; }
    /**
     * Completes phase 1 of agreement, determining which values to accept,
     * signing the accepted values, and preparing "accept messages" to send
     * to each other node in the agreement group.
     * @param current_round The current round in the peer-to-peer overlay
     *        that messages will be sent over.
     * @return A list of accept messages (by shared_ptr) to send to other nodes
     *         in this node's agreement group
     */
    std::vector<std::shared_ptr<messaging::OverlayMessage<RecordType>>> finish_phase_1(int current_round);
    /**
     * Completes phase 2 of agreement, determining which values to accept.
     * @return The set of accepted values (by shared_ptr).
     */
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> finish_phase_2();
    /**
     * Handles a message received during either phase of Crusader Agreement.
     * Determines which phase's logic to use based on the type of the message
     * (specifically, whether message.body is a {@code SignedValue} or an
     * {@code AgreementValue}).
     * @param message A message received during Crusader Agreement.
     */
    void handle_message(const messaging::OverlayMessage<RecordType>& message);

private:
    /**
     * Processes a message for phase 1 of Crusader Agreement: add the signature
     * on this value to the set of received signatures for the same value.
     * @param signed_value A signed value
     */
    void handle_phase_1_message(const messaging::SignedValue<RecordType>& signed_value);
    /**
     * Processes a message for phase 2 of Crusader Agreement: ensure the
     * received value has enough signatures, and add them to the set of
     * signatures for that value if so.
     * @param agreement_value  A message containing a signed set of signatures
     * for a value.
     */
    void handle_phase_2_message(messaging::AgreementValue<RecordType>& agreement_value);
};

} /* namespace adq */

#include "detail/CrusaderAgreementState_impl.hpp"
