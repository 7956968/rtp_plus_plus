#include <CorePch.h>
#include <rtp++/RtpJitterBuffer.h>
#include <limits>

// #define DEBUG_RTP_LOSSES
namespace rtp_plus_plus
{

RtpJitterBuffer::ptr RtpJitterBuffer::create(uint32_t uiPlayoutBufferLatency)
{
  return std::unique_ptr<RtpJitterBuffer>( new RtpJitterBuffer(uiPlayoutBufferLatency) );
}

RtpJitterBuffer::RtpJitterBuffer(uint32_t uiPlayoutBufferLatency)
//  :m_uiPlayoutBufferLatencyMs(uiPlayoutBufferLatency),
//  m_uiClockFrequency(0),
  :IRtpJitterBuffer(uiPlayoutBufferLatency),
    m_jitter(JITTER_SAMPLES),
    m_uiTotalLatePackets(0),
    m_uiTotalDuplicates(0),
    m_uiFirstRtpTime(0),
    m_uiPreviousRtpTime(0),
    m_uiWrapAroundCounter(0)
{

}

RtpJitterBuffer::~RtpJitterBuffer()
{
  VLOG(1) << "Total late packets: " << m_uiTotalLatePackets
          << " Total duplicates: " << m_uiTotalDuplicates;
}

//// Hack for now: need to create codec specific sources in receiver that handle this
//void RtpJitterBuffer::setClockFrequency(uint32_t uiClockFrequency)
//{
//  m_uiClockFrequency = uiClockFrequency;
//}

void RtpJitterBuffer::updateJitter(uint32_t uiJitter)
{
  m_jitter.insert(uiJitter);
}

const RtpPacketGroup RtpJitterBuffer::getNextPlayoutBufferNode()
{
  assert(!m_playoutBufferList.empty());
    boost::mutex::scoped_lock l(m_listLock);
  RtpPacketGroup node = m_playoutBufferList.front();
  m_playoutBufferList.pop_front();
  return node;
}

bool RtpJitterBuffer::doAddRtpPacket(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                                     bool bRtcpSynchronised,
                                     const boost::posix_time::ptime& tPlayout, uint32_t& uiLateMs, bool& bDuplicate)
{
#ifdef DEBUG_RTP_LOSSES
  DLOG(INFO) << "Adding packet with SN " << packet.getSequenceNumber();
#endif

  boost::mutex::scoped_lock l(m_listLock);
  if (!m_playoutBufferList.empty())
  {
    uint32_t uiTs = packet.getRtpTimestamp();
    // check if node with TS exists in list: update node
    auto it = std::find_if(m_playoutBufferList.begin(), m_playoutBufferList.end(), [&uiTs](RtpPacketGroup& node)
    {
      return (node.getRtpTimestamp() == uiTs);
    });
    if (it != m_playoutBufferList.end())
    {
      // update existing entry
      if (!it->insert(packet))
      {
        ++m_uiTotalDuplicates;
        VLOG(2) << "RTP sequence number duplicate: SN: " << packet.getSequenceNumber() << " XSN: "<< packet.getExtendedSequenceNumber();
        bDuplicate = true;
      }
      // we don't schedule another playout on an update!
      // Only the first RTP packet related to an RTP timestamp
      // results in a playout time being returned from this method
      return false;
    }
    else
    {
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
//      tPlayout = calculatePlayoutTime(packet);
      uint32_t uiOldestTs = m_playoutBufferList.front().getRtpTimestamp();

      if (tPlayout < tNow)
      {
        // packet is late
        // we're assuming that this number is fairly small. Downcast should be fine.
        uiLateMs = static_cast<uint32_t>((tNow - tPlayout).total_milliseconds());
        LOG(WARNING) << "RTP discard: packet is late by " << uiLateMs << "ms SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
                     << " Oldest TS: " << uiOldestTs << " Playout: " << tPlayout << " Now: " << tNow;

//        LOG(WARNING) << "RTP discard: packet is late by " << (tNow - tPlayout).total_milliseconds() << "ms SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
//                     << " Oldest TS: " << uiOldestTs << " SN: " << uiOldestSn;
        ++m_uiTotalLatePackets;
        return false;
      }
      else
      {
        if (uiTs < uiOldestTs)
        {
          // must take wrap around into account
          uint32_t uiDiff = uiOldestTs - uiTs;
          if ( uiDiff < std::numeric_limits<uint32_t>::max()>>1 )
          {
            // insert packet before 1st packet with bigger timestamp or the end
            insertAccordingToRtpTs(packet, tPresentation, bRtcpSynchronised, tPlayout, false);
          }
          else
          {
            // Take RTP TS wrap around into account
            insertAccordingToRtpTs(packet, tPresentation, bRtcpSynchronised, tPlayout, true);
          }
          return true;
        }
        else
        {
          // insert packet before 1st packet with bigger timestamp or the end
          insertAccordingToRtpTs(packet, tPresentation, bRtcpSynchronised, tPlayout, false);
          return true;
        }
      }
    }
  }
  else
  {
    // just insert the first packet
    insertAccordingToRtpTs(packet, tPresentation, bRtcpSynchronised, tPlayout, false);
    return true;
  }
}

boost::posix_time::ptime RtpJitterBuffer::calculatePlayoutTime(const RtpPacket& packet,
                                                               const boost::posix_time::ptime& /*tPresentation*/,
                                                               bool /*bRtcpSynchronised*/)
{
  if (m_tFirstPacket.is_not_a_date_time())
  {
    // store arrival time of first packet as a reference
    m_tFirstPacket = packet.getArrivalTime() + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs);
    m_uiFirstRtpTime = packet.getRtpTimestamp();
    m_uiPreviousRtpTime = m_uiFirstRtpTime;
    VLOG(15) << "Using " << m_uiPlayoutBufferLatencyMs << "ms playout buffer. Clock frequency: " << m_uiClockFrequency << "Hz";
    m_tPrevPlayoutTime = m_tFirstPacket;
    return m_tPrevPlayoutTime;
  }
  else
  {
    // calculate playout time based on RTP timestamp difference
    // either need to detect wrap-around so that we can always use the first timestamp as reference
    // alternatively we can always add the difference to the previous RTP packet but that might result
    // in a slowly growing error
    // NOTE: this code assumes that the RTP timestamps are monotonically increasing: we are targetting the
    // low-delay case where there are no B-frames! This algorithm uses the previously received RTP time
    // to determine if wrap-around has occurred
    if (hasWrappedAround(m_uiPreviousRtpTime, packet.getRtpTimestamp()))
    {
      // wrap around
      ++m_uiWrapAroundCounter;
    }

    // now handle offset in RTP time from first packet to calculate relative playout time
    uint32_t uiTotalWraparounds = m_uiWrapAroundCounter;
    if (packet.getRtpTimestamp() < m_uiFirstRtpTime)
    {
      // wrap around is not complete
      --uiTotalWraparounds;
    }

    uint32_t uiDiffFromFirstPacket = packet.getRtpTimestamp() - m_uiFirstRtpTime;
    uint64_t uiTotalTimeElapsedSinceFirstPacket = (uiTotalWraparounds * std::numeric_limits<uint32_t>::max()) + uiDiffFromFirstPacket;
    // convert total time diff to real-time
    double dSeconds = uiTotalTimeElapsedSinceFirstPacket/static_cast<double>(m_uiClockFrequency);
    uint32_t uiMilliseconds = static_cast<uint32_t>(dSeconds * 1000 + 0.5);
#ifdef DEBUG_PLAYOUT_DEADLINE
    LOG(INFO) << "Playout time " << uiMilliseconds << "ms from start";
#endif

    boost::posix_time::ptime tPlayoutTime = m_tFirstPacket + boost::posix_time::milliseconds(uiMilliseconds);
#ifdef DEBUG_PLAYOUT_DEADLINE
    VLOG(5) << "Diff to prev RTP: " << uiDiffFromPreviousPacket
            << " TS: " << (tPlayoutTime - m_tPrevPlayoutTime).total_milliseconds() << "ms"
            << " Diff to start RTP: "  << uiTotalTimeElapsedSinceFirstPacket
            << " Diff to Start TS: " << (tPlayoutTime - m_tFirstPacket).total_milliseconds() << "ms";

#endif
    m_tPrevPlayoutTime = tPlayoutTime;
    m_uiPreviousRtpTime = packet.getRtpTimestamp();
    return tPlayoutTime;
  }
}

void RtpJitterBuffer::insertAccordingToRtpTs(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                                             bool bRtcpSynchronised, 
                                             const boost::posix_time::ptime& tPlayout, bool bWrapAroundDetected)
{
  uint32_t uiTs = packet.getRtpTimestamp();
#ifdef DEBUG_PLAYOUT_DEADLINE
  VLOG(5) << "Inserting RTP packet with SN " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " into playout buffer: " << tPlayout;
#endif

  std::list<RtpPacketGroup>::reverse_iterator it;
  if (bWrapAroundDetected)
  {
    // wrap around
    LOG(INFO) << "Wrap around detected: SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp();
    // insert packet after last packet
    // we need to handle the case where there is wrap around and packets were reordered
    // we search from the end of the list for the first packet with TS ts_n where (ts_n -ts) > UINT_MAX/2
    it = std::find_if(m_playoutBufferList.rbegin(), m_playoutBufferList.rend(), [uiTs](RtpPacketGroup& node)
    {
      return (node.getRtpTimestamp() - uiTs > (std::numeric_limits<uint32_t>::max() >> 1) );
    });
  }
  else
  {
    // insert packet before 1st packet with bigger timestamp or the end
    it = std::find_if(m_playoutBufferList.rbegin(), m_playoutBufferList.rend(), [uiTs](RtpPacketGroup& node)
    {
      return ( node.getRtpTimestamp() < uiTs );
    });
  }

  m_playoutBufferList.insert(it.base(), RtpPacketGroup(packet, tPresentation, bRtcpSynchronised, tPlayout) );
}

} // rtp_plus_plus
