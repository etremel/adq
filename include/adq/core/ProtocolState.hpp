#pragma once

#include "CrusaderAgreementState.hpp"
#include "CryptoLibrary.hpp"
#include "TreeAggregationState.hpp"
#include "adq/core/DataSource.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/util/Overlay.hpp"
#include "adq/util/PointerUtil.hpp"
#include "adq/util/TimerManager.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <list>
#include <memory>
#include <set>
#include <vector>

namespace adq {

enum class ProtocolPhase { IDLE,
                           SETUP,
                           SHUFFLE,
                           AGREEMENT,
                           AGGREGATE };

template <typename RecordType>
class ProtocolState {
private:
    std::shared_ptr<spdlog::logger> logger;
    ProtocolPhase protocol_phase;

    /** The ID of the meter that this ProtocolState tracks state for */
    int meter_id;
    /** The effective number of meters in the network, including virtual meters */
    int num_meters;
    /** Log (base 2) of num_meters */
    int log2n;
    /** The number of aggregation groups, which is based on the number of meters */
    const int num_aggregation_groups;
    /** The current overlay round */
    int overlay_round;
    /** True if the current overlay round is the last one in a query */
    bool is_last_round;
    /** The set of meters (by ID) that have definitely failed this round.
     * Meters are added to this set when this meter fails to establish a TCP
     * connection to them, and we don't bother waiting for a message from a
     * meter that has failed. */
    std::set<int> failed_meter_ids;

    /** Handle for the timer registered to timeout the round. */
    util::timer_id_t round_timeout_timer;
    /**
     * True if this client has received a ping response from its predecessor in
     * the overlay graph in the current round. Reset to false at the end of each
     * round.
     */
    bool ping_response_from_predecessor;

    template <typename T>
    using ptr_list = std::list<std::shared_ptr<T>>;

    ptr_list<messaging::OverlayTransportMessage<RecordType>> future_overlay_messages;
    ptr_list<messaging::AggregationMessage<RecordType>> future_aggregation_messages;
    ptr_list<messaging::OverlayMessage<RecordType>> waiting_messages;
    ptr_list<messaging::OverlayMessage<RecordType>> outgoing_messages;

    std::shared_ptr<messaging::ValueTuple<RecordType>> my_contribution;
    /**
     * This automatically rejects duplicate proxy contributions; it must
     * be an unordered_set because there's no sensible way to "sort" contributions.
     * Since the set of proxies is part of ValueContribution's value equality
     * (by way of ValueTuple), two meters are allowed to contribute the same
     * measurement (they should have distinct proxy sets).
     */
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> proxy_values;
    std::unique_ptr<TreeAggregationState<RecordType>> aggregation_phase_state;

    std::unique_ptr<util::TimerManager> timers;
    /** A reference to the NetworkManager stored in the QueryClient. */
    NetworkManager<RecordType>& network;
    /**
     * A reference to the DataSource stored in the QueryClient. This is only
     * used to access the DataSource's AggregateFunction, when handling
     * aggregation messages.
     */
    DataSource<RecordType>& data_source;
    CryptoLibrary crypto;

    /* --- Specific to BFT agreement --- */
    std::unique_ptr<CrusaderAgreementState<RecordType>> agreement_phase_state;
    int agreement_start_round;
    /** The subset of proxy_values accepted after running Crusader Agreement. */
    util::unordered_ptr_set<messaging::ValueContribution<RecordType>> accepted_proxy_values;
    void handle_agreement_phase_message(const messaging::OverlayMessage<RecordType>& message);
    void handle_shuffle_phase_message(const messaging::OverlayMessage<RecordType>& message);

public:
    void handle_signature_response(messaging::SignatureResponse<RecordType>& message);
    /* ------ */

private:
    /**
     * Helper method that generates an encrypted multicast of a ValueContribution
     * to the proxies it specifies, assuming the overlay is starting in round 0,
     * then ends the overlay round.
     * @param contribution The ValueContribution to multicast.
     */
    void encrypted_multicast_to_proxies(std::shared_ptr<messaging::ValueContribution<RecordType>> contribution);
    /**
     * Sends all messages from waiting_messages and outgoing_messages that need
     * to be sent in the current overlay round.
     */
    void send_overlay_message_batch();
    /**
     * Transitions the protocol from the agreement phase to the aggregate phase.
     */
    void start_aggregate_phase();
    void send_aggregate_if_done();
    /**
     * Ends the current round in the peer-to-peer overlay and starts the next round.
     * Any cached future messages for the next round will be immediately "received"
     * and handled.
     */
    void end_overlay_round();
    /**
     * "Common logic" for ending an overlay round that is not specific to the BFT
     * protocol. This used to be in the "superclass" and should probably be
     * incorporated into end_overlay_round().
     */
    void common_end_overlay_round();
    /**
     * Logic for handling a timeout waiting for a message in the current round.
     * If the predecessor node has responded to a ping recently, we ping it
     * again and keep waiting. If not, we give up and move to the next round.
     */
    void handle_round_timeout();

public:
    /**
     * Constructs the ProtocolState, making it ready to start handling query
     * messages. The NetworkManager must be constructed first, since
     * ProtocolState needs a reference to it.
     *
     * @param num_clients The total number of clients in the network
     * @param local_client_id This client's ID
     * @param network_manager A reference to the NetworkManager that will be used to send messages
     * @param data_source A reference to the client's DataSource, which will be used to
     * compute aggregates when handling AggregationMessages.
     * @param private_key_filename The file containing the private key for this client.
     * Used to construct the CryptoLibrary.
     * @param public_key_files_by_id A map from client IDs to files containing the
     * public keys of those clients. Used to construct the CryptoLibrary.
     */
    ProtocolState(int num_clients, int local_client_id,
                  NetworkManager<RecordType>& network_manager,
                  DataSource<RecordType>& data_source,
                  const std::string& private_key_filename,
                  const std::map<int, std::string>& public_key_files_by_id);

    /**
     * Starts the query protocol to respond to a specific query request with the
     * provided data. Stores the query request message for reference, for the
     * duration of the query.
     *
     * @param query_request A shared_ptr to the query request message, which
     * ProtocolState will now own.
     * @param contributed_data The data to contribute for this query, supplied
     * by the client.
     */
    void start_query(std::shared_ptr<messaging::QueryRequest<RecordType>> query_request, const RecordType& contributed_data);
    /**
     * Processes an overlay message that has been received for the current round.
     * This includes resetting the message timeout for this round, decrypting the
     * message's body if necessary, and adding it to the outgoing waiting_messages
     * list if it needs to be forwarded.
     *
     * @param message An overlay message that should be handled by this client
     * in the current round
     */
    void handle_overlay_message(messaging::OverlayTransportMessage<RecordType>& message);
    /**
     * Processes an aggregation message, assuming the protocol is currently in
     * the aggregation phase.
     *
     * @param message The aggregation message
     */
    void handle_aggregation_message(messaging::AggregationMessage<RecordType>& message);
    /**
     * Processes a ping message from another client, for the purpose of
     * detecting failures. Either responds if it is a request, or locally
     * records the fact that the predecessor responded to a ping recently.
     *
     * @param message The ping message
     */
    void handle_ping_message(const messaging::PingMessage<RecordType>& message);
    /**
     * Stores an overlay message for a future round in an internal cache, so it can be
     * automatically handled when the round advances.
     *
     * @param message A shared_ptr to the overlay message, which ProtocolState will now own
     */
    void buffer_future_message(std::shared_ptr<messaging::OverlayTransportMessage<RecordType>> message);
    /**
     * Stores an aggregation message for a future aggregation step in an internal cache,
     * so it can be automatically handled when that stage of aggregation is reached.
     *
     * @param message A shared_ptr to the aggregation message, which ProtocolState will now own
     */
    void buffer_future_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message);

    int get_num_aggregation_groups() const { return num_aggregation_groups; }
    int get_current_query_num() const { return my_contribution ? my_contribution->query_num : -1; }
    int get_current_overlay_round() const { return overlay_round; }

    bool is_in_overlay_phase() const { return protocol_phase == ProtocolPhase::SHUFFLE || protocol_phase == ProtocolPhase::AGREEMENT; }
    bool is_in_aggregate_phase() const { return protocol_phase == ProtocolPhase::AGGREGATE; }

    /** The maximum time (ms) any meter should wait on receiving a message in an overlay round */
    static constexpr int OVERLAY_ROUND_TIMEOUT = 100;
    /**
     * The number of failures tolerated by the currently running instance of the system.
     * This is set only once, at startup, once the number of meters in the system is known.
     * It should be initialized, by calling init_failures_tolerated(), before creating
     * any instances of ProtocolState.
     */
    static int FAILURES_TOLERATED;

    static void init_failures_tolerated(const int num_meters) {
        FAILURES_TOLERATED = (int)std::ceil(std::log2(num_meters));
    }
};

// Useless boilerplate to complete the declaration of the static member FAILURES_TOLERATED
template <typename RecordType>
int ProtocolState<RecordType>::FAILURES_TOLERATED;

}  // namespace adq

#include "detail/ProtocolState_impl.hpp"
