#pragma once
#include <list>
#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/IRtpJitterBuffer.h>

namespace rtp_plus_plus
{

/**
 * @brief The RtpJitterBufferV2 class is used to compensate for the maximum allowable latency.
 * This class is designed for a single RTP session
 */
class RtpJitterBufferV2 : public IRtpJitterBuffer
{
public:
  typedef std::unique_ptr<RtpJitterBufferV2> ptr;

  static ptr create(uint32_t uiPlayoutBufferLatency = IRtpJitterBuffer::MAX_LATENCY_MS);
  RtpJitterBufferV2(uint32_t uiPlayoutBufferLatency = IRtpJitterBuffer::MAX_LATENCY_MS);
  ~RtpJitterBufferV2();

  virtual const RtpPacketGroup getNextPlayoutBufferNode();

protected:
  virtual boost::posix_time::ptime calculatePlayoutTime(const RtpPacket& packet,
                                                        const boost::posix_time::ptime& tPresentation,
                                                        bool bRtcpSynchronised);

  virtual bool doAddRtpPacket(const RtpPacket& packet,
                              const boost::posix_time::ptime& tPresentation,
                              bool bRtcpSynchronised,
                              const boost::posix_time::ptime& tPlayout,
                              uint32_t& uiLateMs, bool &bDuplicate);

private:
  /**
   * @brief insertAccordingToPts: maintains the order in the RtpPacketGroup list
   * according to presentation time.
   * @param packet
   * @param tPresentation
   * @param tPlayout
   */
  void insertAccordingToPts(const RtpPacket& packet,
                            const boost::posix_time::ptime& tPresentation,
                            bool bRtcpSynchronised, 
                            const boost::posix_time::ptime& tPlayout);

  // lock for multi-threaded access
  mutable boost::mutex m_listLock;

  std::list<RtpPacketGroup> m_playoutBufferList;

  // arrival time of first packet
  boost::posix_time::ptime m_tFirstPacket;
  // presentation time of first packet
  boost::posix_time::ptime m_tFirstPts;
  // presentation time of first RTCP synced packet
  boost::posix_time::ptime m_tFirstSyncedPts;
  // first RTP TS
  uint32_t m_uiFirstRtpTs;
  // RTP TS offset between first RTP packet, and first RTCP synced RTP packet
  uint32_t m_uiRtpDiffMs;

  // For stats
  // 1st SN
  uint32_t m_uiFirstSN;
  // Last SN
  uint32_t m_uiLastSN;
  uint32_t m_uiTotalPackets;
  // discard characteristics
  uint32_t m_uiTotalLatePackets;
  // duplicates
  uint32_t m_uiTotalDuplicates;

  /// Flag for RTCP sync
  bool m_bRtcpSync;

  /**
   * @brief m_recentHistory: store recently processed RtpPacketGroups
   * This allows us to mark late packets as duplicates, etc.
   */
  boost::circular_buffer<RtpPacketGroup> m_recentHistory;
};

}
