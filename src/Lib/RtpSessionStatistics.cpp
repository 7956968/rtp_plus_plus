#include "CorePch.h"
#include <rtp++/RtpSessionStatistics.h>

#define MIN_MEASUREMENT_INTERVAL_MS 1000

namespace rtp_plus_plus
{

IntervalStatistic::IntervalStatistic()
  :IncomingRtpRateKbps(0),
  OutgoingRtpRateKbps(0),
  IncomingRtcpRateKbps(0),
  OutgoingRtcpRateKbps(0),
  DurationMillisecs(0)
{

}

IntervalStatistic::IntervalStatistic( double dIncomingRtpRateKbps, double dOutgoingRtpRateKbps, double dIncomingRtcpRateKbps, double dOutgoingRtcpRateKbps, uint32_t uiDurationMillisecs)
  :IncomingRtpRateKbps(dIncomingRtpRateKbps),
  OutgoingRtpRateKbps(dOutgoingRtpRateKbps),
  IncomingRtcpRateKbps(dIncomingRtcpRateKbps),
  OutgoingRtcpRateKbps(dOutgoingRtcpRateKbps),
  DurationMillisecs(uiDurationMillisecs)
{

}

PacketCounter::PacketCounter()
    :TotalPackets(0),
    TotalOctets(0),
    PacketsInInterval(0),
    OctetsInInterval(0)
{

}

void PacketCounter::addPacket(uint32_t uiPacketSize)
{
  ++TotalPackets;
  TotalOctets += uiPacketSize;
  ++PacketsInInterval;
  OctetsInInterval += uiPacketSize;
}

void PacketCounter::newInterval()
{
  PacketsInInterval = 0;
  OctetsInInterval = 0;
}

std::ostream& operator<< (std::ostream& ostr, const PacketCounter& packetCounter)
{
  ostr << "Total packets: " << packetCounter.TotalPackets
       << " (" << packetCounter.TotalOctets << "B)"
       << " Interval packets: " << packetCounter.PacketsInInterval
       << " (" << packetCounter.OctetsInInterval << "B)";
  return ostr;
}

ChannelStatistics::ChannelStatistics()
  :m_uiRtpTsLastRtpPacketSent(0)
{

}

ChannelStatistics::~ChannelStatistics()
{
#ifdef ENABLE_STATISTICS
  if (!m_vRtpIntervals.empty() || !m_vRtcpIntervals.empty())
    VLOG(2) << "Mean interval RTP: " << getMeanRtpInterval() << "ms RTCP:" << getMeanRtcpInterval() << "ms";
#endif
}

uint32_t ChannelStatistics::getTotalRtpPacketsSent() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsSent.begin(), m_mRtpPacketsSent.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.TotalPackets;
  });

  return uiTotal;
}

uint32_t ChannelStatistics::getTotalOctetsSent() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsSent.begin(), m_mRtpPacketsSent.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.TotalOctets;
  });
  return uiTotal;
}

uint32_t ChannelStatistics::getTotalRtpPacketsSent(uint8_t uiPayloadType) const
{
  auto it = m_mRtpPacketsSent.find(uiPayloadType);
  if (it != m_mRtpPacketsSent.end())
    return it->second.TotalPackets;
  return 0;
}

uint32_t ChannelStatistics::getTotalOctetsSent(uint8_t uiPayloadType) const
{
  auto it = m_mRtpPacketsSent.find(uiPayloadType);
  if (it != m_mRtpPacketsSent.end())
    return it->second.TotalOctets;
  return 0;
}

uint32_t ChannelStatistics::getRtpPacketsSentDuringLastInterval() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsSent.begin(), m_mRtpPacketsSent.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.PacketsInInterval;
  });

  return uiTotal;
}

uint32_t ChannelStatistics::getOctetsSentDuringLastInterval() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsSent.begin(), m_mRtpPacketsSent.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.OctetsInInterval;
  });
  return uiTotal;
}

uint32_t ChannelStatistics::getRtpPacketsSentDuringLastInterval(uint8_t uiPayloadType) const
{
  auto it = m_mRtpPacketsSent.find(uiPayloadType);
  if (it != m_mRtpPacketsSent.end())
    return it->second.PacketsInInterval;
  return 0;
}

uint32_t ChannelStatistics::getOctetsSentDuringLastInterval(uint8_t uiPayloadType) const
{
  auto it = m_mRtpPacketsSent.find(uiPayloadType);
  if (it != m_mRtpPacketsSent.end())
    return it->second.OctetsInInterval;
  return 0;
}

uint32_t ChannelStatistics::getTotalRtpPacketsReceived() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsReceived.begin(), m_mRtpPacketsReceived.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.TotalPackets;
  });
  return uiTotal;
}

uint32_t ChannelStatistics::getTotalOctetsReceived() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsReceived.begin(), m_mRtpPacketsReceived.end(), [&uiTotal](const  std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.TotalOctets;
  });
  return uiTotal;
}

uint32_t ChannelStatistics::getRtpPacketsReceivedDuringLastInterval() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsReceived.begin(), m_mRtpPacketsReceived.end(), [&uiTotal](const  std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.PacketsInInterval;
  });
  return uiTotal;
}

uint32_t ChannelStatistics::getOctetsReceivedDuringLastInterval() const
{
  uint32_t uiTotal = 0;
  std::for_each(m_mRtpPacketsReceived.begin(), m_mRtpPacketsReceived.end(), [&uiTotal](const std::pair<uint8_t, PacketCounter>& pair)
  {
    uiTotal += pair.second.OctetsInInterval;
  });
  return uiTotal;
}

#ifdef ENABLE_STATISTICS
double ChannelStatistics::getMeanRtpInterval() const
{
  if (m_vRtpIntervals.empty()) return 0.0;
  double sum = std::accumulate(std::begin(m_vRtpIntervals), std::end(m_vRtpIntervals), 0.0);
  return sum / m_vRtpIntervals.size();
}

double ChannelStatistics::getMeanRtcpInterval() const
{
  if (m_vRtcpIntervals.empty()) return 0.0;
  double sum = std::accumulate(std::begin(m_vRtcpIntervals), std::end(m_vRtcpIntervals), 0.0);
  return sum / m_vRtcpIntervals.size();
}
#endif

void ChannelStatistics::newInterval()
{
  for (auto& counter : m_mRtpPacketsSent)
  {
    counter.second.newInterval();
  }
  for (auto& counter : m_mRtpPacketsReceived)
  {
    counter.second.newInterval();
  }

  m_rtcpPacketsSent.newInterval();
  m_rtcpPacketsReceived.newInterval();
}

void ChannelStatistics::onSendRtpPacket( const uint8_t uiPayload, const uint32_t uiPacketSize, uint32_t uiRtpTimestamp )
{
  // store RTP TS and local system time mapping
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#ifdef ENABLE_STATISTICS
  if (!m_tLastRtpPacketSent.is_not_a_date_time())
  {
    uint64_t uiDiff = ( tNow - m_tLastRtpPacketSent ).total_milliseconds();
    m_vRtpIntervals.push_back(uiDiff);
  }
#endif
  m_tLastRtpPacketSent = tNow;
  m_uiRtpTsLastRtpPacketSent = uiRtpTimestamp;

  m_mRtpPacketsSent[uiPayload].addPacket(uiPacketSize);
}

void ChannelStatistics::onReceiveRtpPacket( const uint8_t uiPayload, const uint32_t uiPacketSize )
{
  m_mRtpPacketsReceived[uiPayload].addPacket(uiPacketSize);
}

void ChannelStatistics::onSendRtcpPacket( const uint32_t uiPacketSize )
{
  // store RTP TS and local system time mapping
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#ifdef ENABLE_STATISTICS
  if (!m_tLastRtcpPacketSent.is_not_a_date_time())
  {
    uint64_t uiDiff = ( tNow - m_tLastRtcpPacketSent ).total_milliseconds();
    m_vRtcpIntervals.push_back(uiDiff);
  }
#endif
  m_tLastRtcpPacketSent = tNow;

  m_rtcpPacketsSent.addPacket(uiPacketSize);
}

void ChannelStatistics::onReceiveRtcpPacket( const uint32_t uiPacketSize )
{
  m_rtcpPacketsReceived.addPacket(uiPacketSize);
}

std::ostream& operator<< (std::ostream& ostr, const ChannelStatistics& channelStats)
{
  ostr << "Rtp (out): " << channelStats.m_tLastRtpPacketSent;
  for (auto& pair: channelStats.m_mRtpPacketsSent)
  {
    ostr << " RTP PT: " << (int)pair.first << ": " << pair.second << std::endl;
  }
  ostr << "Rtp (in): " << std::endl;
  for (auto& pair: channelStats.m_mRtpPacketsReceived)
  {
    ostr << "RTP PT: " << (int)pair.first << ": " << pair.second << std::endl;
  }
  ostr << "Rtcp (out): " << channelStats.m_tLastRtcpPacketSent << " "
       << channelStats.m_rtcpPacketsSent << std::endl
       << "Rtcp (in): " << channelStats.m_rtcpPacketsReceived << std::endl;
  return ostr;
}

RtpSessionStatistics::RtpSessionStatistics()
  :m_uiCurrentDurationMs(0),
    m_uiRtpRecv(0),
    m_uiRtpSent(0),
    m_uiRtcpRecv(0),
    m_uiRtcpSent(0),
    m_bSummariseStats(false),
    m_bFinalised(false),
    m_dIncomingRtpRateKbps(0.0),
    m_dIncomingRtcpRateKbps(0.0),
    m_dOutgoingRtpRateKbps(0.0),
    m_dOutgoingRtcpRateKbps(0.0)
{

}

RtpSessionStatistics::~RtpSessionStatistics()
{
  if (m_bSummariseStats)
  {
    LOG(INFO) << "RTCP Statistics: ";
    std::for_each(m_vStats.begin(), m_vStats.end(), [](IntervalStatistic stat)
    {
      double dRatio = (stat.IncomingRtcpRateKbps + stat.OutgoingRtcpRateKbps)/(static_cast<double>(stat.IncomingRtpRateKbps + stat.OutgoingRtpRateKbps));
      LOG(INFO) << "Duration ms: " << stat.DurationMillisecs
        << " RTP: Incoming: " << stat.IncomingRtpRateKbps
        << "kbps Outgoing: " << stat.OutgoingRtpRateKbps
        << "kbps RTCP Incoming: " << stat.IncomingRtcpRateKbps
        << "kbps Outgoing: " << stat.OutgoingRtcpRateKbps
        << "kbps Ratio: " << dRatio;
    });
  }
}

void RtpSessionStatistics::enableStatisticsSummary()
{
  m_bSummariseStats = true;
}

uint32_t RtpSessionStatistics::getTotalRtpPacketsSent(const uint32_t uiId) const
{
  auto it = m_mEndpointStatistic.find(uiId);
  if ( it != m_mEndpointStatistic.end())
  {
      return it->second.getTotalRtpPacketsSent();
  }
  return 0;
}

uint32_t RtpSessionStatistics::getTotalOctetsSent(const uint32_t uiId) const
{
  auto it = m_mEndpointStatistic.find(uiId);
  if ( it != m_mEndpointStatistic.end() )
  {
      return it->second.getTotalOctetsSent();
  }
  return 0;
}

void RtpSessionStatistics::newInterval()
{
  m_tEnd = boost::posix_time::microsec_clock::universal_time();
  if (!m_tStart.is_not_a_date_time())
  {
    // compute and store average
    boost::posix_time::time_duration tDuration = m_tEnd - m_tStart;
    uint32_t uiDurationMs = static_cast<uint32_t>(tDuration.total_milliseconds());
    m_uiCurrentDurationMs += uiDurationMs;
    m_uiRtpRecv += m_overallStatistic.getOctetsReceivedDuringLastInterval();
    m_uiRtpSent += m_overallStatistic.getOctetsSentDuringLastInterval();
    m_uiRtcpRecv += m_overallStatistic.m_rtcpPacketsReceived.OctetsInInterval;
    m_uiRtcpSent += m_overallStatistic.m_rtcpPacketsSent.OctetsInInterval;

    // check if the interval was sufficient for estimates: when using rfc4585 in scenarios
    // with high loss this method was firing so often (every couple of milliseconds making
    // the estimates totally unreliable.
    // Now we estimate the rate at most every two and a half seconds
    if (m_uiCurrentDurationMs >= MIN_MEASUREMENT_INTERVAL_MS || m_bFinalised)
    {
      m_tLastEstimate = m_tEnd;
      // convert from Bytes per milliseconds to kbps 8*1000/1000
      m_dIncomingRtpRateKbps =  (8 * m_uiRtpRecv)/static_cast<double>(m_uiCurrentDurationMs);
      m_dOutgoingRtpRateKbps =  (8 * m_uiRtpSent)/static_cast<double>(m_uiCurrentDurationMs);
      m_dIncomingRtcpRateKbps = (8 * m_uiRtcpRecv)/static_cast<double>(m_uiCurrentDurationMs);
      m_dOutgoingRtcpRateKbps = (8 * m_uiRtcpSent)/static_cast<double>(m_uiCurrentDurationMs);

      VLOG(5) << "Stats - duration ms: " << m_uiCurrentDurationMs
        << " RTP out: "  << m_dOutgoingRtpRateKbps << " kbps " << m_overallStatistic.getRtpPacketsSentDuringLastInterval()
        << "(" << m_overallStatistic.getOctetsSentDuringLastInterval()
        << "B) RTCP out: " << m_dOutgoingRtcpRateKbps << " kbps " << m_overallStatistic.m_rtcpPacketsSent.PacketsInInterval
        << "(" << m_overallStatistic.m_rtcpPacketsSent.OctetsInInterval << "B)"
        << " RTP in: " << m_dIncomingRtpRateKbps << " kbps " << m_overallStatistic.getRtpPacketsReceivedDuringLastInterval()
        << "(" << m_overallStatistic.getOctetsReceivedDuringLastInterval()
        << "B) RTCP in: " << m_dIncomingRtcpRateKbps << " kbps " << m_overallStatistic.m_rtcpPacketsReceived.PacketsInInterval
        << "(" << m_overallStatistic.m_rtcpPacketsReceived.OctetsInInterval << "B)";

      if (m_bSummariseStats)
      {
        m_vStats.push_back(
              IntervalStatistic(
                m_dIncomingRtpRateKbps,
                m_dOutgoingRtpRateKbps,
                m_dIncomingRtcpRateKbps,
                m_dOutgoingRtcpRateKbps,
                m_uiCurrentDurationMs
                )
              );
      }

      m_uiCurrentDurationMs = 0;
      m_uiRtpRecv = 0;
      m_uiRtpSent = 0;
      m_uiRtcpRecv = 0;
      m_uiRtcpSent = 0;
    }
  }

  m_overallStatistic.newInterval();

  m_tStart = m_tEnd;
}

void RtpSessionStatistics::newInterval(const uint32_t uiId)
{
  m_mEndpointStatistic[uiId].newInterval();
}

void RtpSessionStatistics::finalise()
{
  m_bFinalised = true;
  newInterval();
  for (auto pair : m_mEndpointStatistic)
  {
    pair.second.newInterval();
  }
}

uint32_t RtpSessionStatistics::getLastIncomingRtpRateEstimate(boost::posix_time::ptime& tEstimate) const
{
  tEstimate = m_tLastEstimate;
  return static_cast<uint32_t>(m_dIncomingRtpRateKbps);
}

uint32_t RtpSessionStatistics::getLastOutgoingRtpRateEstimate(boost::posix_time::ptime& tEstimate) const
{
  tEstimate = m_tLastEstimate;
  return static_cast<uint32_t>(m_dOutgoingRtpRateKbps);
}

void RtpSessionStatistics::onSendRtpPacket( const RtpPacket& packet, const EndPoint& ep  )
{
  m_overallStatistic.onSendRtpPacket(packet.getHeader().getPayloadType(), packet.getSize(), packet.getHeader().getRtpTimestamp());
  m_mEndpointStatistic[ep.getId()].onSendRtpPacket(packet.getHeader().getPayloadType(), packet.getSize(), packet.getHeader().getRtpTimestamp());
}

void RtpSessionStatistics::onReceiveRtpPacket( const RtpPacket& packet, const EndPoint& ep  )
{
  m_overallStatistic.onReceiveRtpPacket( packet.getHeader().getPayloadType(), packet.getSize());
  m_mEndpointStatistic[ep.getId()].onReceiveRtpPacket(packet.getHeader().getPayloadType(), packet.getSize());
}

void RtpSessionStatistics::onSendRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep )
{
  uint32_t uiSize = compoundRtcpPacketSize(compoundPacket);
  m_overallStatistic.onSendRtcpPacket(uiSize);
  m_mEndpointStatistic[ep.getId()].onSendRtcpPacket(uiSize);
}

void RtpSessionStatistics::onReceiveRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep )
{
  uint32_t uiSize = compoundRtcpPacketSize(compoundPacket);
  m_overallStatistic.onReceiveRtcpPacket(uiSize);
  m_mEndpointStatistic[ep.getId()].onReceiveRtcpPacket(uiSize);
}

} // rtp_plus_plus
