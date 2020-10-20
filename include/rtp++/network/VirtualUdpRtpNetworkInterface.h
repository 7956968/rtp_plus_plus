#pragma once
#include <deque>
#include <vector>
#include <boost/function.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/RtpNetworkInterface.h>

/// @def DEBUG_RTP Logs first RTP and RTCP packet
#define DEBUG_RTP

namespace rtp_plus_plus
{

typedef boost::function<void (Buffer rtpBuffer, const EndPoint& from, const EndPoint& to)> SendCallback_t;

/**
* This class simulates the sending of RTP and RTCP over a UDP socket
*/
class VirtualUdpRtpNetworkInterface : public RtpNetworkInterface
{
public:

  VirtualUdpRtpNetworkInterface( const EndPoint& rtpEp, const EndPoint& rtcpEp );

  virtual ~VirtualUdpRtpNetworkInterface();

  void setRtpSendCallback(SendCallback_t rtpCallback) { m_rtpCallback = rtpCallback; }
  void setRtcpSendCallback(SendCallback_t rtcpCallback) { m_rtcpCallback = rtcpCallback; }

  void receiveRtpPacket(NetworkPacket networkPacket, const EndPoint& from);
  void receiveRtcpPacket(NetworkPacket networkPacket, const EndPoint& from);

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

  void handleSentRtpPacket(Buffer buffer, const EndPoint &ep);
  void handleSentRtcpPacket(Buffer buffer, const EndPoint &ep);

  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

private:

  EndPoint m_rtpEp;
  EndPoint m_rtcpEp;

  SendCallback_t m_rtpCallback;
  SendCallback_t m_rtcpCallback;
};

}
