#pragma once
#include <rtp++/scheduling/RtpScheduler.h>

namespace rtp_plus_plus
{

/**
 * @brief The RoundRobinScheduler class
 */
class RandomScheduler : public RtpScheduler
{
public:
  /**
   * @brief RandomScheduler
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @param bPathWiseSplitAllowed
   */
  RandomScheduler(RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows, bool bPathWiseSplitAllowed = true)
    :RtpScheduler(pRtpSession),
      m_uiNumberOfFlows(uiNumberOfFlows),
      m_bPathWiseSplitAllowed(bPathWiseSplitAllowed)
  {

  }
  /**
   * @brief ~RandomScheduler destructor
   */
  ~RandomScheduler()
  {

  }
  /**
   * @brief scheduleRtpPackets schedules the sending of packets in random fashion.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    assert(!vRtpPackets.empty());
    int index = rand() % m_uiNumberOfFlows;

    if (m_bPathWiseSplitAllowed)
    {
      for (const RtpPacket& rtpPacket: vRtpPackets)
      {
        uint16_t uiIndex = rand() % m_uiNumberOfFlows;
        m_pRtpSession->sendRtpPacket(rtpPacket, uiIndex);
      }
    }
    else
    {
      uint16_t uiIndex = rand() % m_uiNumberOfFlows;
      for (const RtpPacket& rtpPacket: vRtpPackets)
      {
        m_pRtpSession->sendRtpPacket(rtpPacket, uiIndex);
      }
    }
  }

protected:
  uint16_t m_uiNumberOfFlows;
  bool m_bPathWiseSplitAllowed;
};

} // rtp_plus_plus
