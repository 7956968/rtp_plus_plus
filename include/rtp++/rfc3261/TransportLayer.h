#pragma once
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/network/UdpSocketWrapper.h>
#include <rtp++/rfc3261/Protocols.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/SipClientTcpConnection.h>
#include <rtp++/rfc3261/SipServerTcpConnection.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

#define DEFAULT_MTU 1500

/**
 * @brief Transport layer for SIP Client and Server transactions.
 *
 * The transport layer is responsible for the actual transmission of
 * requests and responses over network transports.  This includes
 * determination of the connection to use for a request or response in
 * the case of connection-oriented transports.
 *
 * The transport layer is responsible for managing persistent
 * connections for transport protocols like TCP and SCTP, or TLS over
 * those, including ones opened to the transport layer.  This includes
 * connections opened by the client or server transports, so that
 * connections are shared between client and server transport functions.
 * These connections are indexed by the tuple formed from the address,
 * port, and transport protocol at the far end of the connection.  When
 * a connection is opened by the transport layer, this index is set to
 * the destination IP, port and transport.  When the connection is
 * accepted by the transport layer, this index is set to the source IP
 * address, port number, and transport.  Note that, because the source
 * port is often ephemeral, but it cannot be known whether it is
 * ephemeral or selected through procedures in [4], connections accepted
 * by the transport layer will frequently not be reused.  The result is
 * that two proxies in a "peering" relationship using a connection-
 * oriented transport frequently will have two connections in use, one
 * for transactions initiated in each direction.
 *
 * It is RECOMMENDED that connections be kept open for some
 * implementation-defined duration after the last message was sent or
 * received over that connection.  This duration SHOULD at least equal
 * the longest amount of time the element would need in order to bring a
 * transaction from instantiation to the terminated state.  This is to
 * make it likely that transactions are completed over the same
 * connection on which they are initiated (for example, request,
 * response, and in the case of INVITE, ACK for non-2xx responses).
 * This usually means at least 64*T1 (see Section 17.1.1.1 for a
 * definition of T1).  However, it could be larger in an element that
 * has a TU using a large value for timer C (bullet 11 of Section 16.6),
 * for example.
 *
 * All SIP elements MUST implement UDP and TCP.  SIP elements MAY
 * implement other protocols.
 *
 * Making TCP mandatory for the UA is a substantial change from RFC
 * 2543.  It has arisen out of the need to handle larger messages,
 * which MUST use TCP, as discussed below.  Thus, even if an element
 * never sends large messages, it may receive one and needs to be
 * able to handle them.
 */
class TransportLayer : public boost::noncopyable
{
  /// @typedef Transport descriptor consisting of address, port and protocol
  typedef std::tuple<std::string, uint16_t, TransactionProtocol> Transport_t;
  /// @typedef to map Transport_t to TCP server, client or UDP connection. First uint32_t = vector_id [0=server tcp, 1 = client tcp, 2 = udp], second uint32_t = index into vector
  typedef std::pair<uint32_t, uint32_t> ConnectionId_t;
public:
  /**
   * @brief Incoming SIP message handler
   * @param sipMessage Incoming SIP message
   */
  typedef boost::function<void(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)> SipMessageHandler_t;
  //typedef boost::function<void(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)> ResponseHandler_t;
  /**
   * @brief Constructor
   *
   * @param ioService IO service
   * @param messageHandler Callback to be invoked when SIP messages are received.
   * @param uiMtu Path MTU
   */
  TransportLayer(boost::asio::io_service& ioService, SipMessageHandler_t messageHandler, const std::string& sFQDN = "", uint16_t uiSipPort = DEFAULT_SIP_PORT, uint32_t uiMtu = DEFAULT_MTU);
  /**
   * @brief Destructor
   */
  ~TransportLayer();
  /**
   * @brief adds a local transport to the layer
   */
  boost::system::error_code addLocalTransport(const std::string& sInterface, uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief checks if a transport exists
   */
  bool hasTransport(const std::string& sInterface, uint16_t uiPort, TransactionProtocol eProtocol) const;
  /**
   * @brief The TransportLayer starts listening on the configured transports.
   *
   * If no transports have been added, the default transport "0.0.0.0", 5060, TCP is used.
   */
  boost::system::error_code start();
  /**
   * @brief The TransportLayer stops listening on the configured transports.
   *
   */
  boost::system::error_code stop();
  /**
   * @brief sends a SIP request message to the specified destination using the specified protocol
   * @TODO: do we need to distinguish between SIP requests and responses here or is the one method sufficient?
   */
  boost::system::error_code sendSipMessage(const SipMessage& sipMessage, const std::string& sDestination, 
                                        const uint16_t uiPort, TransactionProtocol eProtocol);

private:
  void startAccept(const Transport_t& transport, uint32_t index);
  void acceptHandler(const boost::system::error_code& error, SipServerTcpConnection::ptr pNewConnection, Transport_t& transport, uint32_t index);
  void tcpReadCompletionHandler(const boost::system::error_code&, const std::string&, SipServerTcpConnection::ptr);
  void tcpWriteCompletionHandler(const boost::system::error_code&, const std::string&, SipServerTcpConnection::ptr);
  void tcpCloseCompletionHandler(SipServerTcpConnection::ptr);

  void tcpClientReadCompletionHandler(const boost::system::error_code&, const std::string&, SipClientTcpConnection::ptr);
  void tcpClientWriteCompletionHandler(const boost::system::error_code&, const std::string&, SipClientTcpConnection::ptr);
  void tcpClientCloseCompletionHandler(SipClientTcpConnection::ptr);

  void handleIncomingUdpPacket(const boost::system::error_code& ec,
                                UdpSocketWrapper::ptr pSource,
                                NetworkPacket networkPacket,
                                const EndPoint& ep);

  void handleSentUdpPacket(const boost::system::error_code& ec,
                           UdpSocketWrapper::ptr pSource,
                           Buffer buffer,
                           const EndPoint& ep);

  /**
   * @brief Creates listening sockets
   */
  boost::system::error_code createLocalTransportSocket(const Transport_t& t);
  /**
   * @brief gets the connection id to be used to send a message to the specified destination
   * @param[in] sInterface The address of the destination
   * @param[in] uiPort The port of the destination
   * @param[in] eProtocol The protocol to be used for the message
   * @param[out] The connection id to be used to send the message if it could located/created
   * @return Returns an error_code if no connection could be located.
   */
  boost::system::error_code getOutgoingConnection(const std::string& sInterface, uint16_t uiPort, TransactionProtocol eProtocol, ConnectionId_t& connectionId);

private:
  /// io service
  boost::asio::io_service& m_ioService;
  /// message handler for incoming SIP messages
  SipMessageHandler_t m_messageHandler;
  /// local IP or FQDN
  std::string m_sFQDN;
  /// default SIP port
  uint16_t m_uiSipPort;
  /// max UDP message size
  uint32_t m_uiUdpMaxMessageSize;
  /// registered transports
  std::vector<Transport_t> m_vTransports;
  /// acceptors for TCP
  std::vector<boost::shared_ptr<boost::asio::ip::tcp::acceptor> > m_vAcceptors;
  /// server sockets for TCP
  std::vector<SipServerTcpConnection::ptr> m_vTcpConnections;
  /// client sockets for TCP
  std::vector<SipClientTcpConnection::ptr> m_vTcpClientConnections;
  /// sockets for UDP
  std::vector<UdpSocketWrapper::ptr> m_vUdpSockets;
  /// map from Transport_t to ConnectionId_t which will allow us to use the correct socket
  std::map<Transport_t, ConnectionId_t> m_mConnectionMap;
};

} // rfc3261
} // rtp_plus_plus
