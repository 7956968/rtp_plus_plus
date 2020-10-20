#pragma once
#ifdef ENABLE_ASIO_DCCP
#include <deque>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include <cpputil/Buffer.h>

#include <rtp++/network/dccp/dccp.hpp>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/NetworkPacket.h>

namespace rtp_plus_plus
{

/**
  * A socket can be used to send messages to multiple endpoints
  * For this reason the endpoint needs to be passed in as a parameter
  */

class RtpNetworkInterface;

class DccpRtpConnection : private boost::noncopyable,
                         public boost::enable_shared_from_this<DccpRtpConnection>
{
  typedef std::pair<Buffer, EndPoint> NetworkPackage_t;
public:

  typedef boost::shared_ptr<DccpRtpConnection> ptr;
  typedef boost::function<void (DccpRtpConnection::ptr)> TimeoutCb_t;

  typedef boost::function<void (const boost::system::error_code&, DccpRtpConnection::ptr, NetworkPacket, const EndPoint&)> ReceiveCb_t;
  typedef boost::function<void (const boost::system::error_code&, DccpRtpConnection::ptr, Buffer, const EndPoint&)> SendCb_t;

  static DccpRtpConnection::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort);

  DccpRtpConnection(boost::asio::io_service& ioService);
  DccpRtpConnection(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort);
  ~DccpRtpConnection();

  boost::asio::ip::dccp::socket& socket() { return m_socket; }

  void start();
  void start(boost::asio::ip::dccp::resolver::iterator endpoint_iter);

  void stop();

  void connect(boost::asio::ip::dccp::resolver::iterator endpoint_iter);
  void send(Buffer networkPacket, const EndPoint& endpoint);
  void close();

  void onRecv(ReceiveCb_t val) { m_fnOnRecv = val; }
  void onSendComplete(SendCb_t val) { m_fnOnSend = val; }
  void onTimeout(TimeoutCb_t val) { m_fnOnTimeout = val; }

private:

  void initialise();
  void read();

  void timeoutHandler(const boost::system::error_code& ec);
  void handleConnect( const boost::system::error_code& error, boost::asio::ip::dccp::resolver::iterator endpointIterator );
  void readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received);
  void writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);

private:

  /// asio service for io operations
  boost::asio::io_service& m_rIoService;
  /// timer for the purpose of notifying the calling class that no data has arrived
  boost::asio::deadline_timer m_timer;

  /// IP address that the udp socket should be bound to
  std::string m_sIpAddress;
  /// Port that the udp socket should be bound to
  uint16_t m_uiPort;
  /// Local IP that the socket should be bound to
  boost::asio::ip::address m_address;
  /// Local end point that the socket should be bound to
  boost::asio::ip::dccp::endpoint m_endpoint;
  /// Udp socket to be used for sending and receiving data
  boost::asio::ip::dccp::socket m_socket;

  bool m_bConnectionInProgress;
  /// Buffer for incoming data.

  enum
  {
    max_length = 1500,
    udp_receiver_buffer_size_kb = 300000
  };
  char m_data[max_length];

  //boost::array<char, 8192> m_buffer;
  unsigned char m_sizeBuffer[2];
  /// Buffer for async reads
  boost::asio::streambuf m_streamBuffer;
  /// End-point that the last packet was received from
  boost::asio::ip::dccp::endpoint m_lastSenderEndpoint;
  /// Receive data callback
  ReceiveCb_t m_fnOnRecv;
  /// Send data callback
  SendCb_t m_fnOnSend;
  /// Timeout data callback
  TimeoutCb_t m_fnOnTimeout;

  /// Sender members
  /// threading lock for queue
  boost::mutex m_lock;
  std::deque< NetworkPackage_t > m_vDeliveryQueue;
};

}
#endif
