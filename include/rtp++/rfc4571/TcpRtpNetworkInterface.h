#pragma once
#include <deque>
#include <set>
#include <vector>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/TcpRtpConnection.h>

/// @def DEBUG_RTP Logs first RTP and RTCP packet
#define DEBUG_RTP

// -DDEBUG_RTP_TS
// #define DEBUG_RTP_TS

/// -DRTP_DEBUG: writes RTP and RTCP packets to file
//#define RTP_DEBUG

#ifdef RTP_DEBUG
  #include <rtp++/RtpDump.h>
#endif

namespace rtp_plus_plus {
namespace rfc4571 {

/**
 * This class is the RTP interface between the library and the socket wrapper
 * RTP/RTCP goes in via method calls and comes out via the callback mechanism
 */
class TcpRtpNetworkInterface : public RtpNetworkInterface
{
public:

  TcpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp );
  virtual ~TcpRtpNetworkInterface();
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

  void stopConnection(const std::string& sEndPoint, TcpRtpConnection::ptr pConnection);
protected:

  void handleRtpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleSentRtpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep);
  void handleSentRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep);

  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

  virtual TcpRtpConnection::ptr getConnection(const EndPoint& ep, bool bRtp);
protected:
  /// io service
  boost::asio::io_service& m_rIoService;

  EndPoint m_rtpEp;
  EndPoint m_rtcpEp;

  /// shutdown flag
  bool m_bShuttingDown;

  /// TCP connections
  std::map<std::string, TcpRtpConnection::ptr> m_connections;
  boost::mutex m_connectionLock;
};

} // rfc4571
} // rtp_plus_plus
