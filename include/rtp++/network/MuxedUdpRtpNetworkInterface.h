#pragma once
#include <deque>
#include <boost/asio/io_service.hpp>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/UdpSocketWrapper.h>
#include <rtp++/rfc3550/Rtcp.h>

namespace rtp_plus_plus
{

/**
 * This class is the RTP interface between the library and the socket wrapper
 * It handles muxing of RTP and RTCP over the same port
 */
class MuxedUdpRtpNetworkInterface : public RtpNetworkInterface
{
public:

  /**
   * @brief Constructor
   */
  MuxedUdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpRtcpEp, boost::system::error_code& ec);
  /**
   * @brief Constructor
   */
  MuxedUdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpRtcpEp, std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket);
  /**
   * @brief Destructor
   */
  virtual ~MuxedUdpRtpNetworkInterface();
  /**
   * This method should initalise the component and setup all required
   */
  virtual void reset();
  /**
   * If this method is called the component will begin the shutdown sequence
   * - This will generate an RTCP BYE
   * - No further sends or receives will occur once shutdown has ben called
   */
  virtual void shutdown();
  /**
   * This method should make the sockets ready for receiving data
   */
  virtual bool recv();

protected:

  /**
   * @brief initialises RTP/RTCP socket
   */
  void initialise(boost::system::error_code& ec);
  /**
   * @brief initialises previously constructed RTP and RTCP sockets
   */
  void initialiseExistingSocket(std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket);
  void handleMuxedRtpRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleSentRtpRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, Buffer buffer, const EndPoint& ep);
  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

private:
  /// io service
  boost::asio::io_service& m_rIoService;

  /// bool to keep track if UDP sockets have been bound
  bool m_bInitialised;
  /// shutdown flag
  bool m_bShuttingDown;

  // Local endpoint
  EndPoint m_rtpRtcpEp;
  /// RTP/RTCP Socket
  UdpSocketWrapper::ptr m_pMuxedRtpSocket;

  /// State management for callbacks
  enum PacketType
  {
    RTP_PACKET = 0,
    COMPOUND_RTCP_PACKET = 1
  };
  /// As outgoing packets are sent, we need to store the type since
  /// both RTP and RTCP packets may be sent over the same socket
  /// and the underlying socket wrapper can not differentiate between
  /// the two
  std::deque<PacketType> m_qTypes;
  boost::mutex m_lock;
};

}
