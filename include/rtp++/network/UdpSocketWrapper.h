#pragma once
#include <deque>
#include <utility>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <cpputil/Buffer.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/NetworkPacket.h>

namespace rtp_plus_plus
{

/**
 * @brief A wrapper for a UDP socket that can be used to send messages to multiple endpoints
 *
 * For this reason the endpoint needs to be passed in as a parameter.
 */
class UdpSocketWrapper : private boost::noncopyable,
                         public boost::enable_shared_from_this<UdpSocketWrapper>
{
  typedef std::pair<Buffer, EndPoint> NetworkPackage_t;
public:
  /**
   * \typedef for smart pointer to UdpSocketWrapper
   */
  typedef boost::shared_ptr<UdpSocketWrapper> ptr;
  /**
   * \typedef for timeout callback 
   */
  typedef boost::function<void(UdpSocketWrapper::ptr)> TimeoutCb_t;
  /**
   * \typedef for receive callback
   */
  typedef boost::function<void(const boost::system::error_code&, UdpSocketWrapper::ptr, NetworkPacket, const EndPoint&)> ReceiveCb_t;
  /**
   * \typedef for send callback
   */
  typedef boost::function<void(const boost::system::error_code&, UdpSocketWrapper::ptr, Buffer, const EndPoint&)> SendCb_t;
  /**
   * @brief Named constructor for UdpSocketWrapper.
   *
   * This version throws an exception when it fails to bind to the specified port.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   * @return a smart pointer to the created UdpSocketWrapper object
   */
  static UdpSocketWrapper::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort);
  /**
   * @brief Named constructor for UdpSocketWrapper.
   *
   * This version throws an exception when it fails to bind to the specified port.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   * @param[in] pSocket The previously constructed socket. This class takes ownership of the socket.
   * @return a smart pointer to the created UdpSocketWrapper object
   */
  static UdpSocketWrapper::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, std::unique_ptr<boost::asio::ip::udp::socket> pSocket);
  /**
   * @brief Named constructor for UdpSocketWrapper
   *
   * This version sets the error code when it fails to bind to the specified port.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   * @param[out] ec In the case where the socket binding fails, the error_code is set.
   * @return a smart pointer to the created UdpSocketWrapper object
   */
  static UdpSocketWrapper::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, boost::system::error_code& ec);
  /**
   * @brief Constructor for UdpSocketWrapper.
   *
   * This version throws an exception when it fails to bind to the specified port.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   */
  UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort);
  /**
   * @brief Constructor for UdpSocketWrapper
   *
   * This version sets the error code when it fails to bind to the specified port.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   * @param[out] ec In the case where the socket binding fails, the error_code is set.
   */
  UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, boost::system::error_code& ec);
  /**
   * @brief Constructor for UdpSocketWrapper
   *
   * This version takes a previously constructed socket as a parameter.
   * @param[in] ioService A reference to the IO service to be used for IO operations
   * @param[in] sBindIp The IP address that the UDP socket should be bound to.
   * @param[in] uiBindPort The port that the UDP socket should be bound to.
   * @param[in] pSocket The previously constructed socket. This class takes ownership of the socket.
   */
  UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, std::unique_ptr<boost::asio::ip::udp::socket> pSocket);
  /**
   * @brief Destructor
   */
  ~UdpSocketWrapper();
  /**
   * @brief sends the network packet to the specified endpoint
   *
   * The send callback m_fnOnSend will be invoked once the asynchronous operation has completed.
   * @param networkPacket The packet to be sent to the endpoint
   * @param endpoint The endpoint the packet should be sent to.
   */
  void send(Buffer networkPacket, const EndPoint& endpoint);
  /**
   * @brief Closes the underlying UDP socket.
   *
   * This results in outstanding operations such as recv() being cancelled.
   */
  void close();
  /**
   * @brief Starts an asynchronous recv() operation.
   *
   * The m_fnOnRecv callback will be invoked on completion of the asynchronous operation.
   */
  void recv();
  /**
   * @brief configures the receive callback.
   */
  void onRecv(ReceiveCb_t val) { m_fnOnRecv = val; }
  /**
   * @brief configures the send callback.
   */
  void onSendComplete(SendCb_t val) { m_fnOnSend = val; }
  /**
   * @brief configures the timeout callback.
   */
  void onTimeout(TimeoutCb_t val) { m_fnOnTimeout = val; }

private:

  /** 
   * @brief initialises the underlying UDP socket
   * @return The error_code containing the result of the initialisation
   */
  boost::system::error_code initialise();
  /**
   * @brief Callback invoked when receive times out
   */
  void timeoutHandler(const boost::system::error_code& ec);
  /**
   * @brief Callback to be invoked on asynchronous read operation
   */
  void readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received);
  /**
   * @brief Callback to be invoked on asynchronous write operation
   */
  void writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);

private:

  //! asio service for io operations
  boost::asio::io_service& m_rIoService;
  //! timer for the purpose of notifying the calling class that no data has arrived
  boost::asio::deadline_timer m_timer;
  //! IP address that the udp socket should be bound to
  std::string m_sIpAddress;
  //! Port that the udp socket should be bound to
  uint16_t m_uiPort;
  //! Local IP that the socket should be bound to
  boost::asio::ip::address m_address;
  //! Local end point that the socket should be bound to
  boost::asio::ip::udp::endpoint m_endpoint;
  //! Udp socket to be used for sending and receiving data
  std::unique_ptr<boost::asio::ip::udp::socket> m_pSocket;
  //! End-point that the last packet was received from
  boost::asio::ip::udp::endpoint m_lastSenderEndpoint;
  //! Receive data callback
  ReceiveCb_t m_fnOnRecv;
  //! Send data callback
  SendCb_t m_fnOnSend;
  //! Timeout data callback
  TimeoutCb_t m_fnOnTimeout;
  //!  flag whether timeouts should trigger the callback
  bool m_bTimeOut;
  //! Timeout in milliseconds
  unsigned m_uiTimeoutMs;
  //! Receiver constants
  enum
  {
    max_length = 1500,
    udp_receiver_buffer_size_kb = 300000
  };
  //! Buffer for receiving incoming network packets
  char m_data[max_length];
  //! threading lock for m_vDeliveryQueue
  boost::mutex m_lock;
  //! Deque to store packets while component is busy
  std::deque< NetworkPackage_t > m_vDeliveryQueue;
private:
};

} // rtp_plus_plus
