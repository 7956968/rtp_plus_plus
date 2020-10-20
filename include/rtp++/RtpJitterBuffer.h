#pragma once
#include <cassert>
#include <limits>
#include <list>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <cpputil/RunningAverageQueue.h>
#include <rtp++/IRtpJitterBuffer.h>

/// @def DEBUG_RTP_LOSSES Logs received sequence numbers
// #define DEBUG_RTP_LOSSES

/// @def DEBUG_PLAYOUT_DEADLINE Debug calculated playout times
// #define DEBUG_PLAYOUT_DEADLINE

namespace rtp_plus_plus
{

/**
 * @class RtpJitterBuffer
 *
 * @brief Buffer for incoming rtp packets. The RtpJitterBuffer is responsible for reordering packets and ignoring duplicate
 *        packets. The class is based on the design in the Perkins RTP book.
 *
 * Samples that have missed the playout time are discarded from the buffer.
 */

/// A maximum 10 samples should be enough to average the jitter
#define JITTER_SAMPLES 10
///// 100 ms for video? 30 - 50 for audio?
//#define MAX_LATENCY_MS 100

// fwd
namespace test
{
class RtpJitterBufferTest;
}


class RtpJitterBuffer : public IRtpJitterBuffer
{
  friend class test::RtpJitterBufferTest;
public:
  typedef std::unique_ptr<RtpJitterBuffer> ptr;

  static ptr create(uint32_t uiPlayoutBufferLatency = IRtpJitterBuffer::MAX_LATENCY_MS);

  RtpJitterBuffer(uint32_t uiPlayoutBufferLatency = IRtpJitterBuffer::MAX_LATENCY_MS);

  virtual ~RtpJitterBuffer();

  /// Method to update the jitter: this is used to update the playout point accordingly
  void updateJitter(uint32_t uiJitter);

  virtual const RtpPacketGroup getNextPlayoutBufferNode();

protected:
  virtual bool doAddRtpPacket(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                              bool bRtcpSynchronised,
                              const boost::posix_time::ptime& tPlayout, uint32_t &uiLateMs, bool &bDuplicate);

  /**
   * @brief basic implementation that calculates the playout time based on the arrival time of the first RTP packet
   * This implementation assumes that timestamps are increasing
   */
  virtual boost::posix_time::ptime calculatePlayoutTime(const RtpPacket& packet,
                                                        const boost::posix_time::ptime& tPresentation,
                                                        bool bRtcpSynchronised);

  /**
   * @brief This insertion method takes wrap around of the RTP TS into account
   */
  void insertAccordingToRtpTs(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                              bool bRtcpSynchronised,
                              const boost::posix_time::ptime& tPlayout, bool bWrapAroundDetected);

protected:

  bool hasWrappedAround(uint32_t uiTs1, uint32_t uiTs2) const
  {
    return (uiTs2 < uiTs1 && ((uiTs1 - uiTs2) > UINT_MAX >> 1));
  }
  // lock for multi-threaded access
  mutable boost::mutex m_listLock;
  // Playout buffer according to Perkins
  std::list<RtpPacketGroup> m_playoutBufferList;

  RunningAverageQueue<uint32_t, double> m_jitter;

  // discard characteristics
  uint32_t m_uiTotalLatePackets;
  // duplicates
  uint32_t m_uiTotalDuplicates;
  // arrival time of first packet
  boost::posix_time::ptime m_tFirstPacket;
  // RTP arrival time of first packet
  uint32_t m_uiFirstRtpTime;
  // arrival time of previous packet
  boost::posix_time::ptime m_tPrevPlayoutTime;
  // RTP arrival time of previous packet
  uint32_t m_uiPreviousRtpTime;
  uint32_t m_uiWrapAroundCounter;
};

} // rtp_plus_plus

