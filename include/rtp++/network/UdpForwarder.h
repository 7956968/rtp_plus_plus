#pragma once

#include <deque>
#include <map>
#include <unordered_map>
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
  * A socket can be used to send messages to multiple endpoints
  * For this reason the endpoint needs to be passed in as a parameter
  */

class UdpForwarder : private boost::noncopyable,
                     public boost::enable_shared_from_this<UdpForwarder>
{
  typedef std::pair<Buffer, EndPoint> NetworkPackage_t;
public:

  typedef boost::shared_ptr<UdpForwarder> ptr;
  typedef boost::function<void (UdpForwarder::ptr)> TimeoutCb_t;

  static UdpForwarder::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, uint32_t uiPacketLossProbability = 0, uint32_t uiOwd = 0, uint32_t uiJitter = 0);
  static UdpForwarder::ptr create(boost::asio::io_service& ioService, const EndPoint& endpoint, uint32_t uiPacketLossProbability = 0, uint32_t uiOwd = 0, uint32_t uiJitter = 0);
  static UdpForwarder::ptr create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, uint32_t uiPacketLossProbability = 0, const std::vector<double>& vDelays = std::vector<double>());
  static UdpForwarder::ptr create(boost::asio::io_service& ioService, const EndPoint& endpoint, uint32_t uiPacketLossProbability = 0, const std::vector<double>& vDelays = std::vector<double>());

  UdpForwarder(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, uint32_t uiPacketLossProbability, uint32_t uiOwd, uint32_t uiJitter);
  UdpForwarder(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, uint32_t uiPacketLossProbability, const std::vector<double>& vDelays);

  void addForwardingPair(const EndPoint& host1, const EndPoint& host2)
  {
    VLOG(2) << "Adding forwarding pairs " << host1 << " to " << host2;
    m_map[host1.getAddress()][host1.getPort()] = host2;
    m_map[host2.getAddress()][host2.getPort()] = host1;

    for (auto& it: m_map)
    {
      for (auto& it2: it.second)
      {
        VLOG(2) << "FWD MAP INFO IP: " << it.first << ":" << it2.first << " " << it2.second;
      }
    }
  }


  void send(Buffer networkPacket, const EndPoint& endpoint);
  void sendPacketDelayed(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, Buffer networkPacket, const EndPoint& endpoint);
  void close();
  void recv();

  void onTimeout(TimeoutCb_t val) { m_fnOnTimeout = val; }

private:

  void initialise();

  void timeoutHandler(const boost::system::error_code& ec);
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
  boost::asio::ip::udp::endpoint m_endpoint;
  /// Udp socket to be used for sending and receiving data
  boost::asio::ip::udp::socket m_socket;
  /// End-point that the last packet was received from
  boost::asio::ip::udp::endpoint m_lastSenderEndpoint;
  /// Timeout data callback
  TimeoutCb_t m_fnOnTimeout;
  /// flag whether timeouts should trigger the callback
  bool m_bTimeOut;
  /// Timeout in milliseconds
  unsigned m_uiTimeoutMs;

  enum
  {
    max_length = 1500,
    udp_receiver_buffer_size_kb = 300000
  };
  char m_data[max_length];

  /// Sender members
  /// threading lock for queue
  boost::mutex m_lock;
  std::deque< NetworkPackage_t > m_vDeliveryQueue;

  // Map from address to port to forwarding address descriptor
  std::map<std::string, std::map<uint16_t, EndPoint> > m_map;

  /// packet loss probability
  uint32_t m_uiPacketLossProbability;

  /// Delays can be owd/jitter or based on the supplied vector
  bool m_bUseJitter;
  /// OWD in ms
  uint32_t m_uiOwdMs;
  /// Jitter in ms
  uint32_t m_uiJitterMs;
  /// Delay vector-based
  std::vector<double> m_vDelays;
  /// Delay vector index
  uint32_t m_uiDelayIndex;

  /// Timers for delayed forwarding
  std::unordered_map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mTimers;
};

}
