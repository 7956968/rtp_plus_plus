#pragma once
#include <rtp++/scheduling/RtpScheduler.h>

namespace rtp_plus_plus
{

/**
 * @brief The DistributedScheduler class
 */
class DistributedScheduler : public RtpScheduler
{
public:
  /**
   * @brief create Named constructor
   * @param sPacketSendPattern must start with "dst-"
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @return
   */
  static std::unique_ptr<RtpScheduler> create(const std::string& sPattern, RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows)
  {
    if (boost::algorithm::starts_with(sPattern, "dst-"))
    {
      // see if there's a correct format string
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sPattern.substr(4), "-", true);
      if (!vTokens.empty())
      {
        // for distributed sending: each pair consists of a lower and upper bound
        std::vector<std::pair<uint32_t, uint32_t> > vPacketDistPattern;

        for (std::string& sPair: vTokens)
        {
          std::vector<uint32_t> vPair = StringTokenizer::tokenizeV2<uint32_t>(sPair, ":", true);
          assert(vPair.size() == 2);
          std::pair<uint32_t, uint32_t> bounds = std::make_pair(vPair[0], vPair[1]);
          vPacketDistPattern.push_back(bounds);
          VLOG(2) << "MPRTP distribution char: " << vPair[0] << ":" << vPair[1];
        }
        if (!vPacketDistPattern.empty())
        {
          return std::unique_ptr<RtpScheduler>(new DistributedScheduler(pRtpSession, uiNumberOfFlows, vPacketDistPattern));
        }
      }
    }
    return std::unique_ptr<RtpScheduler>();
  }
  /**
   * @brief DistributedScheduler
   * @param pRtpSession
   * @param uiNumberOfFlows
   * @param vPacketDistPattern
   */
  DistributedScheduler(RtpSession::ptr pRtpSession, uint16_t uiNumberOfFlows, std::vector<std::pair<uint32_t, uint32_t>> vPacketDistPattern)
    :RtpScheduler(pRtpSession),
      m_uiNumberOfFlows(uiNumberOfFlows),
      m_vPacketDistPattern(vPacketDistPattern),
      m_uiInterfaceIndex(0),
      m_uiPacketsSentOnCurrentIndex(0),
      m_uiCurrentPatternIndex(0),
      m_uiPacketDistCurrentNumberToSend(0)
  {
    m_uiPacketDistCurrentNumberToSend = m_vPacketDistPattern[0].first +
        (rand()%(m_vPacketDistPattern[0].second - m_vPacketDistPattern[0].first + 1));

    VLOG(12) << "MPRTP distributed scheduling pattern: sending " << m_uiPacketDistCurrentNumberToSend << " packets";
  }
  /**
   * @brief ~DistributedScheduler destructor
   */
  ~DistributedScheduler()
  {

  }
  /**
   * @brief scheduleRtpPackets schedules the sending of packets in random fashion.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
  {
    assert(!vRtpPackets.empty());

    for (const RtpPacket& rtpPacket: vRtpPackets)
    {
      if (m_uiPacketsSentOnCurrentIndex == m_uiPacketDistCurrentNumberToSend)
      {
        m_uiCurrentPatternIndex = (m_uiCurrentPatternIndex + 1) % m_vPacketDistPattern.size();
        m_uiPacketsSentOnCurrentIndex = 0;
        m_uiPacketDistCurrentNumberToSend = m_vPacketDistPattern[m_uiCurrentPatternIndex].first +
            (rand()%(m_vPacketDistPattern[m_uiCurrentPatternIndex].second - m_vPacketDistPattern[m_uiCurrentPatternIndex].first + 1));

        VLOG(12) << "MPRTP distributed scheduling pattern sending: " << m_uiPacketDistCurrentNumberToSend << " packets";

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
  uint16_t m_uiNumberOfFlows;
  std::vector<std::pair<uint32_t, uint32_t> > m_vPacketDistPattern;
  uint32_t m_uiInterfaceIndex;
  uint32_t m_uiPacketsSentOnCurrentIndex;
  uint32_t m_uiCurrentPatternIndex;
  uint32_t m_uiPacketDistCurrentNumberToSend;
};

} // rtp_plus_plus
