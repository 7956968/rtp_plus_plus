#pragma once
#include <vector>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpSession.h>

namespace rtp_plus_plus
{

/**
 * @brief The RtpScheduler class is the basic implementation of a single path RTP scheduler.
 * It immediately sends the RTP packets using the RTP session object resulting in bursty behaviour when
 * sending IDR frames.
 *
 * TODO: rename to CongestionControlModule
 */
class RtpScheduler
{
public:
  /**
   * @brief RtpScheduler Constructor
   * @param pRtpSession
   */
  RtpScheduler(RtpSession::ptr pRtpSession)
    :m_pRtpSession(pRtpSession)
  {

  }
  /**
   * @brief ~RtpScheduler
   */
  virtual ~RtpScheduler()
  {

  }
  /**
   * @brief setRtpSession Setter for RTP session
   * @param pRtpSession
   */
  void setRtpSession(RtpSession::ptr pRtpSession) { m_pRtpSession = pRtpSession; }
  /**
   * @brief scheduleRtpPackets immediately sends packets using the provded RTP session object.
   * @param vRtpPackets The RTP packets to be sent.
   *
   * This will result in bursty behaviour.
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    for (const RtpPacket& rtpPacket: vRtpPackets)
    {
      m_pRtpSession->sendRtpPacket(rtpPacket);
    }
  }
  /**
   * @brief scheduleRtxPacket
   * @param rtpPacket
   */
  virtual void scheduleRtxPacket(const RtpPacket& rtpPacket)
  {
    if (m_pRtpSession->isMpRtpSession())
    {
      double dRtt = 0.0;
      uint16_t uiSubflowId = m_pRtpSession->findSubflowWithSmallestRtt(dRtt);
      VLOG(6) << "Selecting flow " << uiSubflowId << " RTT: " << dRtt;
      m_pRtpSession->sendRtpPacket(rtpPacket, uiSubflowId);
    }
    else
    {
      m_pRtpSession->sendRtpPacket(rtpPacket);
    }
  }
  /**
   * @brief processFeedback part of sender interface
   * @param fb
   * @param ep
   */
  virtual void processFeedback(const rfc4585::RtcpFb& /*fb*/, const EndPoint& /*ep*/){}
  /**
   * @brief onIncomingRtp part of receiver interface
   * @param rtpPacket
   * @param ep
   * @param bSSRCValidated
   * @param bRtcpSynchronised
   * @param tPresentation
   */
  virtual void onIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                            bool bSSRCValidated, bool bRtcpSynchronised,
                                            const boost::posix_time::ptime& tPresentation){}
  /**
   * @brief onIncomingRtcp part of receiver interface
   * @param compoundRtcp
   * @param ep
   */
  virtual void onIncomingRtcp(const CompoundRtcpPacket& compoundRtcp,
                                             const EndPoint& ep){}
  /**
   * @brief retrieveFeedback part of receiver interface
   * @return
   */
  virtual std::vector<rfc4585::RtcpFb::ptr> retrieveFeedback() { return std::vector<rfc4585::RtcpFb::ptr>(); }
  /**
   * @brief Give subclasses a chance to shutdown cleanly
   */
  virtual void shutdown() {}

protected:

  RtpSession::ptr m_pRtpSession;
};

} // rtp_plus_plus
