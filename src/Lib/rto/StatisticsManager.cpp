#include "CorePch.h"
#include <rtp++/rto/StatisticsManager.h>

#define SLIDING_WINDOW_SIZE 500

namespace rtp_plus_plus
{
namespace rto
{

StatisticsManager::StatisticsManager()
  :m_id(-1),
    m_uiTotalOriginalPacketsReceived(0),
    m_uiTotalRtxPacketsReceived(0),
    m_qRecentlyReceived(SLIDING_WINDOW_SIZE)
{

}

void StatisticsManager::printStatistics()
{
  // only log if we received any packets to avoid clogging output
  if (m_uiTotalOriginalPacketsReceived != 0)
  {
#if 0
    m_rtxInfo.finalise();
    uint32_t uiTotalPacketsWithoutRtx = m_uiTotalOriginalPacketsReceived + m_rtxInfo.getLostAfterLateCount();
    // print stats
    VLOG(2) << "Statistics [" << m_id << "] Total original packets received: " << m_uiTotalOriginalPacketsReceived
            << " Late packets: " << m_rtxInfo.getFalsePositiveCount() << "(" << m_rtxInfo.getFalsePositiveCount()*100/(double)m_uiTotalOriginalPacketsReceived << "%)"
            << " Cancelled RTX: " << m_rtxInfo.getCancelledRtxCount() << "(" << m_rtxInfo.getCancelledRtxCount()*100/(double)m_rtxInfo.getFalsePositiveCount() << "%)"
            << " RTX packets: " << m_rtxInfo.getReceivedRtxCount() << "(" << m_rtxInfo.getReceivedRtxCount()*100.0/m_uiTotalOriginalPacketsReceived << "%)"
            << " Redundant RTX: " << m_rtxInfo.getRedundantRtxCount() << "(" << m_rtxInfo.getRedundantRtxCount()*100/(double)m_rtxInfo.getReceivedRtxCount() << "%)"
            << " Late RTX: " << m_rtxInfo.getLateRtxCount() << "(" << m_rtxInfo.getLateRtxCount()*100/(double)m_uiTotalRtxPacketsReceived << "%)"
            << " Lost: " << m_rtxInfo.getLostAfterLateCount() << "(" << m_rtxInfo.getLostAfterLateCount()*100/(double)uiTotalPacketsWithoutRtx << "%)"
            << " Lost after RTX: " << m_rtxInfo.getLostAfterRtxCount() << "(" << m_rtxInfo.getLostAfterRtxCount()*100/(double)uiTotalPacketsWithoutRtx << "%)"
            ;
#else

    // TODO
#if 0
    uint32_t uiTotalPacketsWithoutRtx = m_uiTotalOriginalPacketsReceived + m_rtxInfo.getLostAfterLateCount();
    // print stats
    VLOG(2) << "Statistics [" << m_id << "] Total original packets received: " << m_uiTotalOriginalPacketsReceived
            << " Late packets: " << m_rtxInfo.getFalsePositiveCount() << "(" << m_rtxInfo.getFalsePositiveCount()*100/(double)m_uiTotalOriginalPacketsReceived << "%)"
            << " Cancelled RTX: " << m_rtxInfo.getCancelledRtxCount() << "(" << m_rtxInfo.getCancelledRtxCount()*100/(double)m_rtxInfo.getFalsePositiveCount() << "%)"
            << " RTX packets: " << m_rtxInfo.getReceivedRtxCount() << "(" << m_rtxInfo.getReceivedRtxCount()*100.0/m_uiTotalOriginalPacketsReceived << "%)"
            << " Redundant RTX: " << m_rtxInfo.getRedundantRtxCount() << "(" << m_rtxInfo.getRedundantRtxCount()*100/(double)m_rtxInfo.getReceivedRtxCount() << "%)"
            << " Late RTX: " << m_rtxInfo.getLateRtxCount() << "(" << m_rtxInfo.getLateRtxCount()*100/(double)m_uiTotalRtxPacketsReceived << "%)"
            << " Lost: " << m_rtxInfo.getLostAfterLateCount() << "(" << m_rtxInfo.getLostAfterLateCount()*100/(double)uiTotalPacketsWithoutRtx << "%)"
            << " Lost after RTX: " << m_rtxInfo.getLostAfterRtxCount() << "(" << m_rtxInfo.getLostAfterRtxCount()*100/(double)uiTotalPacketsWithoutRtx << "%)"
            ;
#endif
#endif
    // TODO: see how we can re-arrange this info according to what is actually useful
    // Output statistics from m_mPacketTransmissionInfo
    VLOG(10) << "Verbose packet transmission info";
    for (const auto& pair: m_mPacketTransmissionInfo )
    {
      if (!pair.second.getEstimated().is_not_a_date_time() &&
          !pair.second.getRtxArrival().is_not_a_date_time())
      {
        // still log error for scenarios where the packet arrives after the FB request
        VLOG(10) << pair.first << " "
                 << "Error: " << pair.second.getErrorUs()
                 << "us FB Delay: " << pair.second.getRtxRequestDelayUs()
                 << "us ~RTT: " << pair.second.getRetransmissionTimeUs()
                 << "us Total RTX: " << pair.second.getTotalRetransmissionTimeUs();
      }
      else if (!pair.second.getRtxRequested().is_not_a_date_time())
      {
        // still log error for scenarios where the packet arrives after the FB request
        VLOG(10) << pair.first << " "
                 << "Error: " << pair.second.getErrorUs()
                 << "us FB Delay: " << pair.second.getRtxRequestDelayUs();
      }
      else if (!pair.second.getEstimated().is_not_a_date_time())
      {
        VLOG(15) << pair.first << " "
                 << "Error: " << pair.second.getErrorUs();
      }
    }
  }
}

bool StatisticsManager::isPacketAssumedLost(const uint32_t uiSN) const
{
  auto it = m_mPacketTransmissionInfo.find(uiSN);
  if (it != m_mPacketTransmissionInfo.end())
  {
    return it->second.isAssumedLost();
  }
  return false;
}

bool StatisticsManager::isFalsePositive(const uint32_t uiSN) const
{
  auto it = m_mPacketTransmissionInfo.find(uiSN);
  if (it != m_mPacketTransmissionInfo.end())
  {
    return it->second.isFalsePositive();
  }
  return false;
}

bool StatisticsManager::hasPacketBeenReceivedRecently(const uint32_t uiSN) const
{
  boost::mutex::scoped_lock l(m_lock);
  return std::find(m_qRecentlyReceived.begin(), m_qRecentlyReceived.end(), uiSN) != m_qRecentlyReceived.end();
}

void StatisticsManager::estimatePacketArrivalTime(const boost::posix_time::ptime &tEstimation, const uint32_t uiSN)
{
  PacketTransmissionInfo entry(tEstimation);
  m_mPacketTransmissionInfo.insert(std::make_pair(uiSN, entry));
}

void StatisticsManager::receivePacket(const boost::posix_time::ptime &tArrival, const uint32_t uiSN)
{
  ++m_uiTotalOriginalPacketsReceived;
  {
    boost::mutex::scoped_lock l(m_lock);
    m_qRecentlyReceived.push_back(uiSN);
  }
  m_mPacketTransmissionInfo[uiSN].setArrival(tArrival);

  // if packet was previously assumed lost i.e. the timer timed out -> false positive
  // alternatively if the arrival time is bigger than the estimation time -> false positive
  // the second condition handles cases where the timeout hasn't fired e.g due to CPU slicing
  // "inaccuracy" when paths have little jitter
  if ((isPacketAssumedLost(uiSN)) ||
      (tArrival > m_mPacketTransmissionInfo[uiSN].getEstimated())
     )
  {
    VLOG(5) << "False positive on " << uiSN;
    m_mPacketTransmissionInfo[uiSN].setFalsePositive(tArrival);
  }
}

void StatisticsManager::receiveRtxPacket(const boost::posix_time::ptime &tArrival, const uint32_t uiSN, bool /*bLate*/, bool /*bDuplicate*/)
{
  ++m_uiTotalRtxPacketsReceived;
#if 0
  VLOG(5) << "RTX received for packet " << uiSN << " late: " << bLate << " dup: " << bDuplicate;
#else
  VLOG(5) << "RTX received for packet " << uiSN;
#endif
  // stats update for RTX
  m_mPacketTransmissionInfo[uiSN].setRtxArrival(tArrival);

#if 0
  if (bLate)
  {
    m_rtxInfo.rtxLate(uiSN);
  }
  else
  {
    if (!bDuplicate)
    {
      m_rtxInfo.rtxReceived(uiSN);
    }
    else
    {
      VLOG(2) << "Received duplicate " << uiSN << ". Was this a false positive?";
    }
  }
#endif
}

void StatisticsManager::assumePacketLost(const boost::posix_time::ptime &tLost, const uint32_t uiSN)
{
  m_mPacketTransmissionInfo[uiSN].setAssumedLost(tLost);
}

void StatisticsManager::retransmissionRequested(const boost::posix_time::ptime &tRequested, const uint32_t uiSN)
{
  m_mPacketTransmissionInfo[uiSN].setRtxRequestedAt(tRequested);
}

void StatisticsManager::falsePositiveRemovedBeforeRetransmission(uint32_t uiSN)
{
//   m_rtxInfo.removedBeforeRtx(uiSN);
}

void StatisticsManager::setRtxCancelled(uint32_t uiSN)
{
  m_mPacketTransmissionInfo[uiSN].setRtxCancelled();
}

} // rto
} // rtp_plus_plus
