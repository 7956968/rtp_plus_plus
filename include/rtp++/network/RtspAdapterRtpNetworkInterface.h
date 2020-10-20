#pragma once
#include <rtp++/RtpPacket.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/rfc2326/RtspClientConnection.h>

namespace rtp_plus_plus
{

/**
 * @brief This class is the RTP interface between the library and the socket wrapper
 * RTP/RTCP goes in via method calls and comes out via the callback mechanism
 */
class RtspAdapterRtpNetworkInterface : public RtpNetworkInterface
{
public:

  RtspAdapterRtpNetworkInterface( rfc2326::RtspClientConnection::ptr pConnection, uint32_t uiRtpChannel, uint32_t uiRtcpChannel );
  virtual ~RtspAdapterRtpNetworkInterface();

  void handleIncomingRtp(const boost::system::error_code& ec, NetworkPacket, const EndPoint&, rfc2326::RtspClientConnection::ptr);
  void handleIncomingRtcp(const boost::system::error_code& ec, NetworkPacket, const EndPoint&, rfc2326::RtspClientConnection::ptr);
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
  /**
   * @brief doSendRtp
   * @param rtpBuffer
   * @param rtpEp
   * @return
   */
  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  /**
   * @brief doSendRtcp
   * @param rtcpBuffer
   * @param rtcpEp
   * @return
   */
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);
  /**
   * @brief handleSentRtcpPacket
   * @param ec
   * @param sMessage
   * @param pConnection
   */
  void handleSentRtcpPacket(const boost::system::error_code& ec, const std::string& sMessage, rfc2326::RtspClientConnection::ptr pConnection);

private:

  rfc2326::RtspClientConnection::ptr m_pConnection;
  uint32_t m_uiRtpChannel;
  uint32_t m_uiRtcpChannel;
};

}
