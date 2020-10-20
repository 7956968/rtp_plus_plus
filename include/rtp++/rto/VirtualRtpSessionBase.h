#pragma once

namespace rtp_plus_plus
{
namespace rto
{

// Callback for RTP and RTCP packet to be sent over virtual network
typedef boost::function<void (Buffer rtpBuffer, const EndPoint& from, const EndPoint& to)> SendCallback_t;

/**
  * Base class for virtual RTP sessions. Provides callbacks for sending media.
  * Subclasses must implement receive functionality
  */
class VirtualRtpSessionBase
{
public:
  typedef boost::shared_ptr<VirtualRtpSessionBase> ptr;

  ~VirtualRtpSessionBase()
  {

  }

  void setRtpCallback(SendCallback_t rtpCallback) { m_rtpCallback = rtpCallback; }
  void setRtcpCallback(SendCallback_t rtcpCallback) { m_rtcpCallback = rtcpCallback; }

  /**
    * method that should handle receiving virtual RTP packets
    */
  virtual void receiveRtpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to) = 0;
  /**
    * method that should handle receiving virtual RTCP packets
    */
  virtual void receiveRtcpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to) = 0;

protected:
  void onSendRtp(Buffer rtpBuffer, const EndPoint& from, const EndPoint& to)
  {
    VLOG(20) << "Sending RTP from " << from.getAddress() << ":" << from.getPort()
             << " to " << to.getAddress() << ":" << to.getPort();

    if (m_rtpCallback) m_rtpCallback(rtpBuffer, from, to);
  }

  void onSendRtcp(Buffer rtcpBuffer, const EndPoint& from, const EndPoint& to)
  {
    VLOG(20) << "Sending RTCP from " << from.getAddress() << ":" << from.getPort()
             << " to " << to.getAddress() << ":" << to.getPort();

    if (m_rtcpCallback) m_rtcpCallback(rtcpBuffer, from, to);
  }

protected:
  // Callbacks
  SendCallback_t m_rtpCallback;
  SendCallback_t m_rtcpCallback;
};

} // rto
} // rtp_plus_plus

