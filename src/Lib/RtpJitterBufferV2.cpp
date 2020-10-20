#include "CorePch.h"
#include <rtp++/RtpJitterBufferV2.h>

namespace rtp_plus_plus
{

RtpJitterBufferV2::ptr RtpJitterBufferV2::create(uint32_t uiPlayoutBufferLatency)
{
  return std::unique_ptr<RtpJitterBufferV2>( new RtpJitterBufferV2(uiPlayoutBufferLatency) );
}

RtpJitterBufferV2::RtpJitterBufferV2(uint32_t uiPlayoutBufferLatency)
  :IRtpJitterBuffer(uiPlayoutBufferLatency),
    m_uiFirstRtpTs(0),
    m_uiRtpDiffMs(0),
    m_uiFirstSN(UINT_MAX),
    m_uiLastSN(UINT_MAX),
    m_uiTotalPackets(0),
    m_uiTotalLatePackets(0),
    m_uiTotalDuplicates(0),
    m_bRtcpSync(false),
    m_recentHistory(150)
{

}

RtpJitterBufferV2::~RtpJitterBufferV2()
{
  VLOG(10) << "Total packets: " << m_uiTotalPackets
             << " First SN: " << m_uiFirstSN
             << " Last SN: " << m_uiLastSN
             << " Late: " << m_uiTotalLatePackets
             << " Dup: " << m_uiTotalDuplicates;
}

const RtpPacketGroup
RtpJitterBufferV2::getNextPlayoutBufferNode()
{
  assert(!m_playoutBufferList.empty());
  boost::mutex::scoped_lock l(m_listLock);
  RtpPacketGroup node = m_playoutBufferList.front();
  m_playoutBufferList.pop_front();
  // add node to history
  m_recentHistory.push_back(node);
  return node;
}

boost::posix_time::ptime
RtpJitterBufferV2::calculatePlayoutTime(const RtpPacket& packet,
                                        const boost::posix_time::ptime& tPresentation,
                                        bool bRtcpSynchronised)
{
  // presentation time jumps when RTCP sync occurs
  if (!m_bRtcpSync)
  {
    if (bRtcpSynchronised)
    {
      // reset for resynchronisation
      m_bRtcpSync = true;
      // m_tFirstPacket = boost::date_time::not_a_date_time;
      m_tFirstSyncedPts = tPresentation;
      // calculate offset in ms: this may be zero e.g. in the case
      // where SCTP sends the first packet of an AU and then performs some
      // bundling before sending the last packet of an AU along with
      // packets that have a different RTP TS.

      // handle the case where the first packet is synced already
      if (m_tFirstPacket.is_not_a_date_time())
      {
        // in this case m_uiFirstRtpTs has not been set yet, so set it first
        m_uiFirstRtpTs = packet.getRtpTimestamp();
      }

      m_uiRtpDiffMs = (packet.getRtpTimestamp() - m_uiFirstRtpTs)*1000/m_uiClockFrequency;

      VLOG(5) << "First RTP TS: " << m_uiFirstRtpTs
              << " RTP TS: " << packet.getRtpTimestamp()
              << " Diff: " << m_uiRtpDiffMs << " ms"
                 << " PTS: " << tPresentation;

      VLOG(5) << "RTCP sync: resetting presentation time calc. Diff RTP: " << m_uiRtpDiffMs << " ms"
              << " Diff PTS after sync:" << (m_tFirstSyncedPts - m_tFirstPts).total_milliseconds() << " ms";
    }
  }

  m_uiLastSN = packet.getExtendedSequenceNumber();

  if (m_tFirstPacket.is_not_a_date_time())
  {
    // store time reference points: everything is relative to the arrival time of the first packet
    m_tFirstPacket = packet.getArrivalTime();
    m_tFirstPts = tPresentation;
    m_uiFirstSN = packet.getExtendedSequenceNumber();
    m_uiFirstRtpTs = packet.getRtpTimestamp();

    VLOG(5) << "Using " << m_uiPlayoutBufferLatencyMs << "ms playout buffer. Clock frequency: "
            << m_uiClockFrequency << "Hz"
            << " 1st packet arrival: " << m_tFirstPacket
            << " PTS: " << m_tFirstPts
            << " First synced PTS: " << m_tFirstSyncedPts;

    return m_tFirstPacket + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs);
  }
  else
  {
    if (m_tFirstSyncedPts.is_not_a_date_time())
    {
      // measure difference in presentation time and add to first packet time
      boost::posix_time::time_duration duration = tPresentation - m_tFirstPts;
      return m_tFirstPacket + duration + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs);
    }
    else
    {
      // measure difference in presentation time and add to first packet time
      // in this case we need to add the difference in RTP TS as an offset
      // This is to cater for protocols such as TCP/SCTP where RTP packets may be bundled together
      // resulting in packets with different RTP timestamps arriving at the same time
      // or packets with the same RTP TS arriving at vastly different times.
      // The playout time would then be calculated to far in the future once sync took place:
      // i.e. with playout delay 150 ms in the case where packet 0 with TS X arrives at t0,
      // and packet 1 with TS X, arrives at t0 + 100ms,
      // and RTCP sync has taken place in between: this caused the playout time to be of packet 1
      // to be calculated as t0 + 100 ms + 150 ms instead of t0 + 150 ms
      boost::posix_time::time_duration duration = tPresentation - m_tFirstSyncedPts;
      // initially we used to reset m_tFirstPacket to the arrival of the synced packet
      // now we are using the arrival of the *first* packet, and therefore have to add the RTP
      // offset between the first RTP and the first synced RTP packet as an offset.
      return m_tFirstPacket + duration + boost::posix_time::milliseconds(m_uiPlayoutBufferLatencyMs + m_uiRtpDiffMs);
    }
  }
}

bool
RtpJitterBufferV2::doAddRtpPacket(const RtpPacket& packet,
                                  const boost::posix_time::ptime& tPresentation,
                                  bool bRtcpSynchronised, 
                                  const boost::posix_time::ptime& tPlayout,
                                  uint32_t &uiLateMs, bool &bDuplicate)
{
  boost::mutex::scoped_lock l(m_listLock);
  ++m_uiTotalPackets;

  VLOG(15) << LOG_MODIFY_WITH_CARE
           << " RTP TS: " << packet.getRtpTimestamp()
           << " SN: " << packet.getExtendedSequenceNumber()
           << " PTS: " << tPresentation
           << " Playout: " << tPlayout
           << " Now: " << boost::posix_time::microsec_clock::universal_time();

  // search through our list to see if there is a associated RtpPacketGroup.
  // This can be identfied by a similar timestamp or presentation time
  // Note: For multisession use this code should be adapted to use PTS instead of RTP TS!
  auto it = std::find_if(m_playoutBufferList.begin(),
                         m_playoutBufferList.end(),
                         [&packet](const RtpPacketGroup& group)
  {
    return packet.getRtpTimestamp() == group.getRtpTimestamp();
  });
  if (it != m_playoutBufferList.end())
  {
    // update group: try and insert item
    if (!it->insert(packet))
    {
      // duplicate
      ++m_uiTotalDuplicates;
      bDuplicate = true;
    }
    return false;
  }
  else
  {
    // check if the item is in the recent history which means
    // that it has already been processed.
    auto it = std::find_if(m_recentHistory.rbegin(),
                           m_recentHistory.rend(),
                           [&packet](const RtpPacketGroup& group)
    {
      return packet.getRtpTimestamp() == group.getRtpTimestamp();
    });

    if (it != m_recentHistory.rend())
    {
      // found in recently processed list: packet is late
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
      // we're assuming that this number is fairly small. Downcast should be fine.
      uiLateMs = static_cast<uint32_t>((tNow - tPlayout).total_milliseconds());

      LOG(WARNING) << LOG_MODIFY_WITH_CARE
                   << " RTP discard: packet is late by " << uiLateMs 
                   << " ms but not in history. SN: " << packet.getExtendedSequenceNumber()
                   << " PTS: " << tPresentation
                   << " Playout: " << tPlayout
                   << " Now: " << tNow;

      ++m_uiTotalLatePackets;
      // check if it's duplicate by trying to insert item
      if (!it->insert(packet))
      {
        // duplicate
        ++m_uiTotalDuplicates;
        bDuplicate = true;
      }
      return false;
    }
    else
    {
      // Packet belongs to a new group as long as the history is long enough
      // to have stored sufficient packets
      // Check how the calculated playout time relates to the current time
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
      if (tPlayout < tNow)
      {
        // we're assuming that this number is fairly small. Downcast should be fine.
        uiLateMs = static_cast<uint32_t>((tNow - tPlayout).total_milliseconds());
        // not expecting this to happen but log anyway
        LOG(WARNING) << LOG_MODIFY_WITH_CARE
                     << " RTP discard: packet is late by " << uiLateMs
                     << " ms but not in history. SN: " << packet.getExtendedSequenceNumber()
                     << " PTS: " << tPresentation
                     << " Playout: " << tPlayout
                     << " Now: " << tNow;
        ++m_uiTotalLatePackets;
        return false;
      }
      else
      {
        // create new node
        insertAccordingToPts(packet, tPresentation, bRtcpSynchronised, tPlayout);
        return true;
      }
    }
  }
}

void
RtpJitterBufferV2::insertAccordingToPts(const RtpPacket& packet,
                                        const boost::posix_time::ptime& tPresentation,
                                        bool bRtcpSynchronised, 
                                        const boost::posix_time::ptime& tPlayout)
{
  std::list<RtpPacketGroup>::reverse_iterator it;
  it = std::find_if(m_playoutBufferList.rbegin(),
                    m_playoutBufferList.rend(),
                    [tPresentation](RtpPacketGroup& node)
  {
      return ( node.getPresentationTime() < tPresentation );
  });
  m_playoutBufferList.insert(it.base(), RtpPacketGroup(packet, tPresentation, bRtcpSynchronised, tPlayout) );
}

} // rtp_plus_plus
