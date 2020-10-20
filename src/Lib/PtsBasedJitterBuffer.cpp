#include <CorePch.h>
#include <rtp++/PtsBasedJitterBuffer.h>

// #define DEBUG_RTP_LOSSES

namespace rtp_plus_plus
{

PtsBasedJitterBuffer::ptr PtsBasedJitterBuffer::create(uint32_t uiPlayoutBufferLatency)
{
  return std::unique_ptr<PtsBasedJitterBuffer>( new PtsBasedJitterBuffer(uiPlayoutBufferLatency) );
}

PtsBasedJitterBuffer::PtsBasedJitterBuffer(uint32_t uiPlayoutBufferLatency)
  :IRtpJitterBuffer(uiPlayoutBufferLatency),
    m_jitter(JITTER_SAMPLES),
    m_uiTotalLatePackets(0),
    m_uiTotalDuplicates(0),
    m_uiPreviousRtpTime(0),
    m_uiWrapAroundCounter(0)
{

}

PtsBasedJitterBuffer::~PtsBasedJitterBuffer()
{
  VLOG(1) << "Total late packets: " << m_uiTotalLatePackets << " Total duplicates: " << m_uiTotalDuplicates;
}

void PtsBasedJitterBuffer::updateJitter(uint32_t uiJitter)
{
  m_jitter.insert(uiJitter);
}

const RtpPacketGroup PtsBasedJitterBuffer::getNextPlayoutBufferNode()
{
  assert(!m_playoutBufferList.empty());
  boost::mutex::scoped_lock l(m_listLock);
  RtpPacketGroup node = m_playoutBufferList.front();
  node.sort();
  m_playoutBufferList.pop_front();
  return node;
}

bool PtsBasedJitterBuffer::doAddRtpPacket(const RtpPacket& packet, const boost::posix_time::ptime& tPresentation,
                                          bool bRtcpSynchronised,
                                          const boost::posix_time::ptime& tPlayout, uint32_t& uiLateMs, bool& bDuplicate)
{
#ifdef DEBUG_RTP_LOSSES
  DLOG(INFO) << "Adding packet with SN " << packet.getSequenceNumber();
#endif

  boost::mutex::scoped_lock l(m_listLock);
  if (!m_playoutBufferList.empty())
  {
    // check if node with PTS exists in list: update node
    /*auto it = std::find_if(m_playoutBufferList.begin(), m_playoutBufferList.end(), [&tPresentation](RtpPacketGroup& node)
    {
      return (node.getPresentationTime() == tPresentation);
    });*/
    /// It may be happend that for different layers with RTCP per session there are some rounding errors.
    /// However Samples very close to each other in Presentation Time belong to the same Access unit
    auto it =  m_playoutBufferList.begin();
    while(it != m_playoutBufferList.end())
      {
      const RtpPacketGroup& pckGroup = *it;
      boost::posix_time::time_duration dif= (tPresentation - pckGroup.getPresentationTime());
      if(dif.total_milliseconds()<2&&dif.total_milliseconds()>-2)
      break;
      it++;
    }

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
//      tPlayout = calculatePlayoutTime(packet, tPresentation);

      if (tPlayout < tNow)
      {
        // packet is late
        // we're assuming that this number is fairly small. Downcast should be fine.
        uiLateMs = static_cast<uint32_t>((tNow - tPlayout).total_milliseconds());
        LOG(WARNING) << "RTP discard: packet is late by " << uiLateMs << "ms SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber()
                     << " Playout: " << tPlayout << " Now: " << tNow;

//        LOG(WARNING) << "RTP discard: packet is late by " << (tNow - tPlayout).total_milliseconds() << "ms SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
//                     << " Oldest TS: " << uiOldestTs << " SN: " << uiOldestSn;
        ++m_uiTotalLatePackets;
        return false;
      }
      else
      {
        insertAccordingToPts(packet, tPresentation, bRtcpSynchronised, tPlayout);
        return true;
      }
    }
  }
  else
  {
    // just insert the first packet
    if (m_tFirstPacket.is_not_a_date_time())
    {
      insertAccordingToPts(packet, tPresentation, bRtcpSynchronised, tPlayout);
    }
    else
    {
      // only insert if on time
      // TODO: should re-align reference time if this happens
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
//      tPlayout = calculatePlayoutTime(packet, tPresentation);

      if (tPlayout < tNow)
      {
        // packet is late
        // we're assuming that this number is fairly small. Downcast should be fine.
        uiLateMs = static_cast<uint32_t>((tNow - tPlayout).total_milliseconds());
        LOG(WARNING) << LOG_MODIFY_WITH_CARE
                     << " RTP discard: packet is late by " << uiLateMs << "ms SN: "
                     << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber()
                     << " Playout: " << tPlayout << " Now: " << tNow;

        //        LOG(WARNING) << "RTP discard: packet is late by " << (tNow - tPlayout).total_milliseconds() << "ms SN: " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " TS:" << packet.getRtpTimestamp()
        //                     << " Oldest TS: " << uiOldestTs << " SN: " << uiOldestSn;
        ++m_uiTotalLatePackets;
        return false;
      }
      else
      {
        insertAccordingToPts(packet, tPresentation, bRtcpSynchronised, tPlayout);
        return true;
      }
    }
    return true;
  }
}

boost::posix_time::ptime PtsBasedJitterBuffer::calculatePlayoutTime(const RtpPacket& packet,
                                                                    const boost::posix_time::ptime& tPresentation,
                                                                    bool bRtcpSynchronised)
{
  if (m_tFirstPacket.is_not_a_date_time())
  {
    // store arrival time of first packet as a reference
    m_tFirstPacket = packet.getArrivalTime() + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs);
    m_tFirstPresentationTs = tPresentation;
    VLOG(2) << "Using " << m_uiPlayoutBufferLatencyMs << "ms playout buffer. Clock frequency: " << m_uiClockFrequency << "Hz";
    m_tPrevPlayoutTime = m_tFirstPacket;
    return m_tPrevPlayoutTime;
  }
  else
  {
    // calculate playout time based on RTP timestamp difference
    boost::posix_time::time_duration tDifference = tPresentation - m_tFirstPresentationTs;
    return m_tFirstPacket + tDifference;
  }
}

void PtsBasedJitterBuffer::insertAccordingToPts(const RtpPacket& packet,
                                                const boost::posix_time::ptime& tPresentation,
                                                bool bRtcpSynchronised,
                                                const boost::posix_time::ptime& tPlayout)
{
#ifdef DEBUG_PLAYOUT_DEADLINE
  VLOG(5) << "Inserting RTP packet with SN " << packet.getSequenceNumber() << " XSN: " << packet.getExtendedSequenceNumber() << " into playout buffer: " << tPlayout;
#endif

  std::list<RtpPacketGroup>::reverse_iterator it;
  // insert packet before 1st packet with bigger timestamp or the end
  it = std::find_if(m_playoutBufferList.rbegin(), m_playoutBufferList.rend(), [tPresentation](RtpPacketGroup& node)
  {
    return ( node.getPresentationTime() < tPresentation );
  });
  m_playoutBufferList.insert(it.base(), RtpPacketGroup(packet, tPresentation, bRtcpSynchronised, tPlayout) );
}

} // rtp_plus_plus
