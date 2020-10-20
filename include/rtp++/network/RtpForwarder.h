#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/Utility.h>
#include <rtp++/network/AddressDescriptor.h>
//#include <rtp++/network/MuxedUdpRtpNetworkInterface.h>
//#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/UdpForwarder.h>
#include <rtp++/network/UdpRtpNetworkInterface.h>

#define FIVE_PERCENT 5

namespace rtp_plus_plus
{

/**
 * @brief NetworkProperties_t Consist of loss probability, owd and jitter
 */
typedef std::tuple<uint32_t, uint32_t, uint32_t> NetworkProperties_t;

class RtpForwarder :  public boost::enable_shared_from_this<RtpForwarder>,
                      private boost::noncopyable
{
  public:
  static boost::shared_ptr<RtpForwarder> create(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                                       const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                                       std::vector<NetworkProperties_t> networkProps, bool bStrict = false);

  static boost::shared_ptr<RtpForwarder> create(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                                       const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                                       uint32_t uiLossProbability = FIVE_PERCENT, const std::vector<double>& vDelays = std::vector<double>(), bool bStrict = false);

  RtpForwarder(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
               const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
               std::vector<NetworkProperties_t> networkProps, bool bStrict = false);

  RtpForwarder(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
             const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
             uint32_t uiLossProbability = FIVE_PERCENT,
             const std::vector<double>& vDelays = std::vector<double>(),
             bool bStrict = false);

  ~RtpForwarder();

protected:
  void setupRtpForwarders(const InterfaceDescriptions_t& localInterfaces,
                          const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2);

  void setupUdpForwarders(const InterfaceDescriptions_t& localInterfaces,
                          const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2);

  /// Handlers
  void sendRtpPacket(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex );

  void sendRtcpPacket(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex );

  void onIncomingRtp( const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex );

  void onIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex );

  void onOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex);

  void onOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex );

private:
  /// io service reference
  boost::asio::io_service& m_rIoService;

  /// RTP forwarder
  std::vector<std::unique_ptr<RtpNetworkInterface> > m_vRtpInterfaces;
  /// UDP forwarder
  std::vector<UdpForwarder::ptr> m_vUdpForwarders;

  /// Network properties
  std::vector<NetworkProperties_t> m_vNetworkProperties;

  /// packet loss probability when using delay vector
  uint32_t m_uiPacketLossProbability;

//  /// OWD in ms
//  uint32_t m_uiOwdMs;
//  /// Jitter in ms
//  uint32_t m_uiJitterMs;
  /// delay vector
  std::vector<double> m_vDelays;

  /// If in strict mode, each packet is parsed with the RTP implementation and repacketized
  /// Otherwise packets are reflected at the UDP layer
  bool m_bStrict;

  // Map from address to port to forwarding address descriptor
  std::map<std::string, std::map<uint16_t, EndPoint> > m_map;
  // Map from address to port to forwarding address descriptor
  std::map<std::string, std::map<uint16_t, NetworkProperties_t> > m_networkMap;
};

}
