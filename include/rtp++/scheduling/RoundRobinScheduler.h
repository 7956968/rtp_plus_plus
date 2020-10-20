#pragma once
#include <rtp++/scheduling/RtpScheduler.h>

namespace rtp_plus_plus
{

/**
 * @brief The RoundRobinScheduler class
 */
class RoundRobinScheduler : public RtpScheduler
{
public:
  /**
   * @brief RoundRobinScheduler
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @param bPathWiseSplitAllowed
   */
  RoundRobinScheduler(RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows, bool bPathWiseSplitAllowed = true)
    :RtpScheduler(pRtpSession),
      m_uiLastFlowId(0),
      m_uiNumberOfFlows(uiNumberOfFlows),
      m_bPathWiseSplitAllowed(bPathWiseSplitAllowed)
  {
    VLOG(2) << "RoundRobinScheduler #flows: " << uiNumberOfFlows << " pathwise split allowed: " << bPathWiseSplitAllowed;
  }
  /**
   * @brief ~RoundRobinScheduler destructor
   */
  ~RoundRobinScheduler()
  {

  }
  /**
   * @brief scheduleRtpPackets schedules the sending of packets in round robin fashion.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    assert(!vRtpPackets.empty());
    if (m_bPathWiseSplitAllowed)
    {
      for (const RtpPacket& rtpPacket: vRtpPackets)
      {
        VLOG(12) << "Selecting flow: " << m_uiLastFlowId;
        m_pRtpSession->sendRtpPacket(rtpPacket, m_uiLastFlowId);
        m_uiLastFlowId = (m_uiLastFlowId + 1) % m_uiNumberOfFlows;
      }
    }
    else
    {
      VLOG(12) << "Selecting flow: " << m_uiLastFlowId;
      for (const RtpPacket& rtpPacket: vRtpPackets)
      {
        m_pRtpSession->sendRtpPacket(rtpPacket, m_uiLastFlowId);
      }
      m_uiLastFlowId = (m_uiLastFlowId + 1) % m_uiNumberOfFlows;
    }
  }

protected:
  uint16_t m_uiLastFlowId;
  uint16_t m_uiNumberOfFlows;
  bool m_bPathWiseSplitAllowed;
};

} // rtp_plus_plus
