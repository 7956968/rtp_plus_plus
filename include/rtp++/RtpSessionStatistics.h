#pragma once
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cpputil/RunningAverageQueue.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/EndPoint.h>

namespace rtp_plus_plus
{

struct IntervalStatistic
{
  IntervalStatistic();
  IntervalStatistic( double dIncomingRtpRateKbps, double dOutgoingRtpRateKbps, double dIncomingRtcpRateKbps, double dOutgoingRtcpRateKbps, uint32_t uiDurationMillisecs);

  double IncomingRtpRateKbps;
  double OutgoingRtpRateKbps;
  double IncomingRtcpRateKbps;
  double OutgoingRtcpRateKbps;
  double DurationMillisecs;
};

// -DENABLE_STATISTICS: dangerous wrt memory consumption, replace with running queue at some point
// #define ENABLE_STATISTICS

/**
  * Helper class that stores interval and total packet and octet counts
  */

struct PacketCounter
{
  PacketCounter();
  
  void addPacket(uint32_t uiPacketSize);
  void newInterval();

  uint32_t TotalPackets;
  uint32_t TotalOctets;
  uint32_t PacketsInInterval;
  uint32_t OctetsInInterval;
};

std::ostream& operator<< (std::ostream& ostr, const PacketCounter& packetCounter);

/**
  * This class stores all info related to a single channel
  */
struct ChannelStatistics
{    
    ChannelStatistics();
    ~ChannelStatistics();

    boost::posix_time::ptime getLastRtpSendTime() const { return m_tLastRtpPacketSent; }
    uint32_t getLastRtpSendRtpTime() const { return m_uiRtpTsLastRtpPacketSent; }

    uint32_t getTotalRtpPacketsSent() const;
    uint32_t getTotalOctetsSent() const;
    uint32_t getTotalRtpPacketsSent(uint8_t uiPayloadType) const;
    uint32_t getTotalOctetsSent(uint8_t uiPayloadType) const;

    uint32_t getRtpPacketsSentDuringLastInterval() const;
    uint32_t getOctetsSentDuringLastInterval() const;
    uint32_t getRtpPacketsSentDuringLastInterval(uint8_t uiPayloadType) const;
    uint32_t getOctetsSentDuringLastInterval(uint8_t uiPayloadType) const;

    uint32_t getRtpPacketsReceivedDuringLastInterval() const;
    uint32_t getOctetsReceivedDuringLastInterval() const;

    uint32_t getTotalRtpPacketsReceived() const;
    uint32_t getTotalOctetsReceived() const;

#ifdef ENABLE_STATISTICS
    double getMeanRtpInterval() const;
    double getMeanRtcpInterval() const;
#endif

    void newInterval();

    void onSendRtpPacket( const uint8_t uiPayload, const uint32_t uiPacketSize, uint32_t uiRtpTimestamp );
    void onReceiveRtpPacket( const uint8_t uiPayload, const uint32_t uiPacketSize );
    void onSendRtcpPacket( const uint32_t uiPacketSize );
    void onReceiveRtcpPacket( const uint32_t uiPacketSize );

    // mapping of media time to system time for RTCP SR
    // when an RTP packet is sent we want to store the corresponding system time
    // so that we can work out the RTP timestamp when we send the SR
    boost::posix_time::ptime m_tLastRtpPacketSent;
    boost::posix_time::ptime m_tLastRtcpPacketSent;

    // vector to store intervals
#ifdef ENABLE_STATISTICS
    std::vector<uint64_t> m_vRtpIntervals;
    std::vector<uint64_t> m_vRtcpIntervals;
#endif

    uint32_t m_uiRtpTsLastRtpPacketSent;

    //< RTP BW Statistics per payload format
    std::map<uint8_t, PacketCounter> m_mRtpPacketsSent;
    std::map<uint8_t, PacketCounter> m_mRtpPacketsReceived;
    //< RTCP BW Stats
    PacketCounter m_rtcpPacketsSent;
    PacketCounter m_rtcpPacketsReceived;
};

std::ostream& operator<< (std::ostream& ostr, const ChannelStatistics& channelStats);

/**
 * @brief This class is used for RTCP SR generation. It stores information regarding total number of packets sent
 * The latest modifications allow this class to handle multiple paths as long as each endpoint is related
 * to a *unique* flow id and this id has been set on the endpoint.
 */
class RtpSessionStatistics
{
public:

  RtpSessionStatistics();

  ~RtpSessionStatistics();

  const ChannelStatistics& getOverallStatistic() const { return m_overallStatistic; }
  const ChannelStatistics& getFlowSpecificStatistic(uint32_t uiId) const { return m_mEndpointStatistic[uiId]; }

  boost::posix_time::ptime getLastRtpSendTime() const { return m_overallStatistic.m_tLastRtpPacketSent; }
  uint32_t getLastRtpSendRtpTime() const { return m_overallStatistic.m_uiRtpTsLastRtpPacketSent; }

  uint32_t getTotalRtpPacketsSent() const { return m_overallStatistic.getTotalRtpPacketsSent(); }
  uint32_t getTotalOctetsSent() const { return m_overallStatistic.getTotalOctetsSent(); }
  uint32_t getTotalRtpPacketsSent(const uint32_t uiId) const;
  uint32_t getTotalOctetsSent(const uint32_t uiId) const;
  /**
   * @brief getLastIncomingRateEstimate
   * @return
   */
  uint32_t getLastIncomingRtpRateEstimate(boost::posix_time::ptime& tEstimate) const;
  /**
   * @brief getLastOutgoingRtpRateEstimate
   * @param tEstimate
   * @return
   */
  uint32_t getLastOutgoingRtpRateEstimate(boost::posix_time::ptime& tEstimate) const;
  void newInterval();
  void newInterval(const uint32_t uiId);

  /**
   * @brief finalise should be called at the end of the RTP session.
   */
  void finalise();

  void onSendRtpPacket( const RtpPacket& packet, const EndPoint& ep  );
  void onReceiveRtpPacket( const RtpPacket& packet, const EndPoint& ep  );
  void onSendRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep );
  void onReceiveRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep );

  void enableStatisticsSummary();
private:

  boost::posix_time::ptime m_tStart;
  boost::posix_time::ptime m_tEnd;
  boost::posix_time::ptime m_tLastEstimate;

  // the statistic to store the overall RTP flow data
  ChannelStatistics m_overallStatistic;
  // the statistics to store endpoint specific flow data needed for MPRTP
  // FIXME: HACK FOR NOW CONST CORRECTNESS
  mutable std::map<uint32_t, ChannelStatistics> m_mEndpointStatistic;

  std::vector<IntervalStatistic> m_vStats;

  /// for estimates of bitrates
  uint32_t m_uiCurrentDurationMs;
  uint32_t m_uiRtpRecv;
  uint32_t m_uiRtpSent;
  uint32_t m_uiRtcpRecv;
  uint32_t m_uiRtcpSent;
  bool m_bSummariseStats;
  bool m_bFinalised;

  double m_dIncomingRtpRateKbps;
  double m_dIncomingRtcpRateKbps;
  double m_dOutgoingRtpRateKbps;
  double m_dOutgoingRtcpRateKbps;
};

} // rtp_plus_plus
