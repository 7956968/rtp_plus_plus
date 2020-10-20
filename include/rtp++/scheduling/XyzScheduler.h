#pragma once
#include <rtp++/scheduling/RtpScheduler.h>

namespace rtp_plus_plus
{

/**
 * @brief The XyzScheduler class
 */
class XyzScheduler : public RtpScheduler
{
public:
  /**
   * @brief create Named constructor
   * @param sPacketSendPattern A string pattern of the form x-y-z where x packets will
   * be sent on the first path, y on the second, and so forth
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @return a pointer to the created object if the send pattern is valid, a null ptr otherwise
   */
  static std::unique_ptr<RtpScheduler> create(const std::string& sPacketSendPattern, RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows)
  {
    std::vector<uint32_t> vTokens = StringTokenizer::tokenizeV2<uint32_t>(sPacketSendPattern, "-", true);
    if (!vTokens.empty())
    {
      return std::unique_ptr<RtpScheduler>(new XyzScheduler(pRtpSession, uiNumberOfFlows, vTokens));
    }
    return std::unique_ptr<RtpScheduler>();
  }
  /**
   * @brief XyzScheduler
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @param vPacketSendPattern
   */
  XyzScheduler(RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows, std::vector<uint32_t> vPacketSendPattern)
    :RtpScheduler(pRtpSession),
      m_uiNumberOfFlows(uiNumberOfFlows),
      m_vPacketSendPattern(vPacketSendPattern),
      m_uiInterfaceIndex(0),
      m_uiCurrentPatternIndex(0),
      m_uiPacketsSentOnCurrentIndex(0)
  {
    VLOG(2) << "MPRTP scheduling pattern: " << ::toString(m_vPacketSendPattern);
  }
  /**
   * @brief ~XyzScheduler destructor
   */
  ~XyzScheduler()
  {

  }
  /**
   * @brief scheduleRtpPackets schedules the sending of packets in round robin fashion.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    assert(!vRtpPackets.empty());

    for (const RtpPacket& rtpPacket: vRtpPackets)
    {
      if (m_uiPacketsSentOnCurrentIndex == m_vPacketSendPattern[m_uiCurrentPatternIndex])
      {
        m_uiCurrentPatternIndex = (m_uiCurrentPatternIndex + 1) % m_vPacketSendPattern.size();
        m_uiPacketsSentOnCurrentIndex = 1;
        m_uiInterfaceIndex = (m_uiInterfaceIndex + 1) % m_uiNumberOfFlows;
      }
      else
      {
        ++m_uiPacketsSentOnCurrentIndex;
      }

      m_pRtpSession->sendRtpPacket(rtpPacket, m_uiInterfaceIndex);
    }
  }

protected:
  uint16_t m_uiLastFlowId;
  uint16_t m_uiNumberOfFlows;
  std::vector<uint32_t> m_vPacketSendPattern;
  uint32_t m_uiInterfaceIndex;
  uint32_t m_uiCurrentPatternIndex;
  uint32_t m_uiPacketsSentOnCurrentIndex;
};

} // rtp_plus_plus
