#pragma once

#include "CryptoLibrary.hpp"
#include "InternalTypes.hpp"
#include "MessageConsumer.hpp"
#include "NetworkManager.hpp"
#include "adq/messaging/AggregationMessage.hpp"
#include "adq/util/PointerUtil.hpp"
#include "adq/util/TimerManager.hpp"

#include <spdlog/spdlog.h>

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace adq {

template <typename RecordType>
class QueryServer : public MessageConsumer<RecordType> {
public:
    using QueryCallback = std::function<void(const int, std::shared_ptr<messaging::AggregationMessageValue<RecordType>>)>;

private:
    std::shared_ptr<spdlog::logger> logger;
    const int num_meters;
    NetworkManager<RecordType> network;
    CryptoLibrary crypto_library;
    std::unique_ptr<util::TimerManager> timer_library;
    /** Number of milliseconds to wait for a query timeout interval */
    const int query_timeout_time;
    /** Handle referring to the timer that was set to time-out the current query*/
    int query_timeout_timer;
    int query_num;
    bool query_finished;
    std::map<int, QueryCallback> query_callbacks;
    util::unordered_ptr_multiset<messaging::AggregationMessage<RecordType>> curr_query_results;
    /** All results of queries the utility has issued, indexed by query number. */
    std::vector<std::shared_ptr<messaging::AggregationMessageValue<RecordType>>> all_query_results;
    std::set<int> curr_query_meters_signed;
    //"A priority queue of pointers to QueryRequests, ordered by QueryNumGreater"
    using query_priority_queue = std::priority_queue<
        std::shared_ptr<messaging::QueryRequest<RecordType>>,
        std::vector<std::shared_ptr<messaging::QueryRequest<RecordType>>>,
        util::ptr_comparator<messaging::QueryRequest<RecordType>, messaging::QueryNumGreater<RecordType>>>;
    query_priority_queue pending_batch_queries;

    static int compute_timeout_time(const int num_meters);

    void end_query();

public:
    QueryServer(int num_clients,
                uint16_t service_port,
                const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map,
                const std::string& private_key_filename,
                const std::map<int, std::string>& public_key_files_by_id);

    virtual ~QueryServer();

    /** Handles receiving an AggregationMessage from a meter, which should contain a query result. */
    virtual void handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) override;

    /** Handles receiving a SignatureRequest from a meter, by signing the requested value. */
    virtual void handle_message(std::shared_ptr<messaging::SignatureRequest<RecordType>> message) override;

    // Handlers for other messages that a server should not normally receive. These will print a warning and drop the message.

    virtual void handle_message(std::shared_ptr<messaging::OverlayTransportMessage<RecordType>> message) override;
    virtual void handle_message(std::shared_ptr<messaging::PingMessage<RecordType>> message) override;
    virtual void handle_message(std::shared_ptr<messaging::QueryRequest<RecordType>> message) override;
    virtual void handle_message(std::shared_ptr<messaging::SignatureResponse<RecordType>> message) override;

    /**
     * Starts a query by broadcasting a message from the utility to all the meters in the network.
     * Do not call this while an existing query is still in progress, or the existing query's
     * results will be lost.
     */
    void start_query(std::shared_ptr<messaging::QueryRequest<RecordType>> query);

    /**
     * Starts a batch of queries that should be executed in sequence as quickly as possible.
     * This starts the first query in the batch immediately (defined as the one with
     * the lowest query number), but the next one will not start running until the
     * first one completes.
     * @param queries A batch of queries. The order of this vector will be ignored
     * and the queries will be run in order of query number.
     */
    void start_queries(const std::list<std::shared_ptr<messaging::QueryRequest<RecordType>>>& queries);

    /**
     * Registers a callback function that should be run each time a query completes.
     * This allows other components running at the utility to be notified when a
     * query they sent using this UtilityClient (e.g. through start_query) has
     * completed.
     * @param callback A function that will be called every time a query completes.
     * Its arguments will be the query number and the result of that query (an
     * AggregationMessageValue containing the final aggregate).
     * @return A numeric ID that can be used to refer to this callback later.
     */
    int register_query_callback(const QueryCallback& callback);

    /** Deregisters a callback function previously registered, using its ID. */
    bool deregister_query_callback(const int callback_id);

    /** Gets the stored result of a query that has completed. */
    std::shared_ptr<messaging::AggregationMessageValue<RecordType>> get_query_result(const int query_num) { return all_query_results.at(query_num); }

    /** Starts an infinite loop that listens for incoming messages (i.e.
     * query responses) and reacts to them. This function call never
     * returns, so it should be run in a separate thread from any start_query
     * calls. */
    void listen_loop();

    /** Shuts down the message-listening loop to allow the client to exit cleanly.
     * Obviously, this must be called from a separate thread from listen_loop().  */
    void shut_down();

    /** The maximum time (ms) the utility is willing to wait on a network round-trip */
    static constexpr int NETWORK_ROUNDTRIP_TIMEOUT = 100;
};
}  // namespace adq

#include "detail/QueryServer_impl.hpp"