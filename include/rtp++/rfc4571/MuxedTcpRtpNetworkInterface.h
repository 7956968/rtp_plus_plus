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

namespace rtp_plus_plus
{
namespace rfc4571
{

/**
* This class is the RTP interface between the library and the socket wrapper
* RTP/RTCP goes in via method calls and comes out via the callback mechanism
*/
class MuxedTcpRtpNetworkInterface : public RtpNetworkInterface
{
public:

  MuxedTcpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& localRtpEp, const EndPoint& remoteRtpEp, bool bIsActiveConnection );
  virtual ~MuxedTcpRtpNetworkInterface();

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

//  void stopConnection(const std::string& sEndPoint, TcpRtpConnection::ptr pConnection);
protected:

  /// helper method
  void initAccept();
  /// Initiate an asynchronous accept operation.
  void startAccept();
  /// Handle completion of an asynchronous accept operation.
  void handleAccept(const boost::system::error_code& e);


  void handleMuxedRtpRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleSentRtpRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep);

  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

  bool sendDataOverTcp(Buffer buffer, const EndPoint& ep, bool bIsRtp);

protected:

  /// io service
  boost::asio::io_service& m_rIoService;

  EndPoint m_localRtpEp;
  EndPoint m_remoteRtpEp;
  bool m_bIsActiveConnection;

  /// shutdown flag
  bool m_bConnected;
  bool m_bShuttingDown;

  /// TCP connections
  boost::asio::ip::tcp::acceptor m_acceptor;
  TcpRtpConnection::ptr m_pConnection;
  boost::mutex m_connectionLock;

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
}
