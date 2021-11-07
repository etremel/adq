#pragma once

#include <asio.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <map>

#include <adq/core/InternalTypes.hpp>

namespace adq {

class NetworkManager {
private:
    std::shared_ptr<spdlog::logger> logger;
    /** The io_context that all the sockets will use */
    asio::io_context network_io_context;
    /** The QueryClient that owns this NetworkManager */
    QueryClient& query_client;
    /** Maps client IDs to address/port pairs. */
    std::map<int, asio::ip::tcp::endpoint> id_to_ip_map;
    /** Maps address/port pairs to client IDs */
    std::map<asio::ip::tcp::endpoint, int> ip_to_id_map;
    /**
     * Cache of open sockets to clients, lazily initialized: the socket
     * is created the first time a message is sent to or received from
     * that client. This may also contain a socket for the query server,
     * at entry -1.
     */
    std::map<int, asio::ip::tcp::socket> sockets_by_id;
    /**
     * Contains one receive buffer per client for reading the initial
     * "size of message" data for an incoming message. These will be
     * supplied to the asynchronous read requests.
     */
    std::map<int, std::array<uint8_t, sizeof(std::size_t)>> receive_buffers;

    /**
     * A "server socket" that listens for incoming connections from
     * other clients
     */
    asio::ip::tcp::acceptor connection_listener;


    /**
     * Handler function for ASIO accept events.
     */
    void handle_accept(const asio::error_code& error, asio::ip::tcp::socket incoming_socket);

    /**
     * Handler function for ASIO read events. One copy will be bound
     * to each socket, with the first parameter set to the corresponding
     * client's ID.
     */
    void handle_read(int sender_id, const asio::error_code& error, std::size_t bytes_read);

    /**
     * Starts an asynchronous accept request on the connection listener.
     */
    void do_accept();

    /**
     * Starts an asynchronous read on the socket for the specified client ID
     */
    void do_read(int sender_id);
    /**
     * Performs the application-level logic of reading a message and dispatching
     * it to the correct handler, assuming the entire message has already been
     * received into a buffer.
     */
    void receive_message(const std::vector<uint8_t>& message_bytes);

public:
    NetworkManager(QueryClient& owning_client, const asio::ip::tcp::endpoint& my_tcp_address, const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map);

    /**
     * Starts waiting for network events by giving control of the calling
     * thread to the ASIO io_context. Callers should expect this function
     * to block forever.
     */
    void run();

    /**
     * Sends a stream of overlay messages over the network to another meter,
     * identified by its ID. Messages will be sent in the order they appear
     * in the list.
     * @param messages The messages to send.
     * @param recipient_id The ID of the recipient.
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id);
    /**
     * Sends an AggregationMessage over the network to another meter (or the
     * utility), identified by its ID.
     * @param message The message to send
     * @param recipient_id The ID of the recipient
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id);
    /**
     * Sends a PingMessage over the network to another meter
     * @param message The message to send
     * @param recipient_id The ID of the recipient
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id);
    /** Sends a signature request message to the query server. */
    bool send(const std::shared_ptr<messaging::SignatureRequest>& message);
};
}  // namespace adq