#pragma once
#include <cstdint>
#include <boost/date_time/posix_time/ptime.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpPacketGroup.h>

namespace rtp_plus_plus
{

class IRtpJitterBuffer
{
public:
  /// 100 ms for video? 30 - 50 for audio?
  static const int MAX_LATENCY_MS = 100;

  virtual ~IRtpJitterBuffer(){}

  uint32_t getPlayoutBufferLatency() const { return m_uiPlayoutBufferLatencyMs; }
  /// method to set playout buffer latency
  void setPlayoutBufferLatency(uint32_t uiBufferLatencyMs) { m_uiPlayoutBufferLatencyMs = uiBufferLatencyMs; }

  uint32_t getClockFrequency() const { return m_uiClockFrequency; }
  /// Hack for now: need to create codec specific sources in receiver that handle this
  void setClockFrequency(uint32_t uiClockFrequency) { m_uiClockFrequency = uiClockFrequency; }

  /**
   * @brief It is the callees responsibility to only call this method
   * once the scheduled playout time has been reached.
   * The scheduled playout time is obtained when a packet
   * is successfully inserted into playout buffer list creating
   * a new node.
   */
  virtual const RtpPacketGroup getNextPlayoutBufferNode() = 0;

  /**
   * @brief This method creates a PlayoutBufferNode per RTP timestamp. Subsequent RTP packets
   * belonging to the same RTP timestamp are added to the exisiting PlayoutBufferNode.
   * The scheduled playout time is calculated based on when the first RTP packet arrives.
   * RTP packets that miss their playout point should be discarded
   * @param packet: the RTP packet to be inserted into the playout buffer
   * @param tPlayout: if the method was successful, tPlayout contains the calculated playout time
   * @return true if tPlayout contains the scheduled playout time for the media related to the RTP timestamp
   *         false if an existing node was updated, or if the RTP packet was late or a duplicate
   */
  bool addRtpPacket(const RtpPacket& packet,const boost::posix_time::ptime& tPresentation,
                    bool bRtcpSynchronised, boost::posix_time::ptime& tPlayout,
                    uint32_t &uiLateMs, bool &bDuplicate)
  {
    tPlayout = calculatePlayoutTime(packet, tPresentation, bRtcpSynchronised);

// #define LOG_SAMPLE_TIME_INFO
#ifdef LOG_SAMPLE_TIME_INFO
    boost::posix_time::ptime tArrival = packet.getArrivalTime();
    if (tArrival < tPlayout)
    {
      VLOG(5) << "Playout time of packet " << packet.getSequenceNumber()
              << " in " << (tPlayout - tArrival).total_milliseconds()
              << " RTP TS: " << packet.getRtpTimestamp();
    }
    else
    {
      VLOG(5) << "Playout time of packet " << packet.getSequenceNumber()
              << " in -" << (tArrival - tPlayout).total_milliseconds()
              << " RTP TS: " << packet.getRtpTimestamp();
    }
#endif

    return doAddRtpPacket(packet, tPresentation, bRtcpSynchronised, tPlayout, uiLateMs, bDuplicate);
  }

protected:

  IRtpJitterBuffer(uint32_t uiPlayoutBufferLatency = MAX_LATENCY_MS)
    :m_uiPlayoutBufferLatencyMs(uiPlayoutBufferLatency),
    m_uiClockFrequency(0)
  {

  }

  virtual bool doAddRtpPacket(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                              bool bRtcpSynchronised, const boost::posix_time::ptime& tPlayout, uint32_t &uiLateMs, bool &bDuplicate) = 0;

  /**
   * @brief The implementation should calculate the playout time of the received RTP packet
   */
  virtual boost::posix_time::ptime calculatePlayoutTime(const RtpPacket& packet,
                                                        const boost::posix_time::ptime& tPresentation,
                                                        bool bRtcpSynchronised) = 0;

protected:

  // Maximum latency for buffer
  uint32_t m_uiPlayoutBufferLatencyMs;
  uint32_t m_uiClockFrequency;

};

} // rtp_plus_plus
