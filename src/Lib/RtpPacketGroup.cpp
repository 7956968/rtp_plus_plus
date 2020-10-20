#include "CorePch.h"
#include <rtp++/RtpPacketGroup.h>

namespace rtp_plus_plus
{

RtpPacketGroup::RtpPacketGroup(const RtpPacket& rtpPacket, const boost::posix_time::ptime& tPresentation, bool bRtcpSynchronised, const boost::posix_time::ptime& tPlayout)
  :m_uiRtpTs(rtpPacket.getRtpTimestamp()),
    m_tArrival(rtpPacket.getArrivalTime()),
    m_tPresentation(tPresentation),
    m_bRtcpSynchronised(bRtcpSynchronised),
    m_tPlayout(tPlayout),
    m_uiNextExpectedSN(rtpPacket.getExtendedSequenceNumber() + 1)
{
  m_rtpPacketList.insert(m_rtpPacketList.begin(), rtpPacket);
  m_receivedSequenceNumbers.insert(rtpPacket.getExtendedSequenceNumber());
}

bool RtpPacketGroup::insert(const RtpPacket& rtpPacket)
{
  // commenting out since use in multi RTP sessions result in warning message
#if 0
  if (rtpPacket.getRtpTimestamp() != m_uiRtpTs)
  {
    // this could be the case for packets from different RTP sessions
    VLOG(20) << "RTP TS mismatch: RTP TS:" << rtpPacket.getRtpTimestamp() << " m_uiRtpTs:" << m_uiRtpTs;
  }
#endif
  // assert(rtpPacket.getRtpTimestamp() == m_uiRtpTs);
  uint32_t uiReceivedSN = rtpPacket.getExtendedSequenceNumber();

  // common case
  if ( uiReceivedSN == m_uiNextExpectedSN )
  {
    m_rtpPacketList.insert(m_rtpPacketList.end(), rtpPacket);
    m_receivedSequenceNumbers.insert(uiReceivedSN);
    ++m_uiNextExpectedSN;
    return true;
  }

  // check for duplicates
  if ( m_receivedSequenceNumbers.find(uiReceivedSN) != m_receivedSequenceNumbers.end() )
  {
    return false;
  }

  // next most common case: loss or old packet from before wrap around
  if (uiReceivedSN > m_uiNextExpectedSN)
  {
    // two cases:
    // 1. Packet loss occurred and received packet is greater than expected
    // 2. Wrap around has occurred, but an older packet from before the wrap-around was received
    uint32_t uiDiff = uiReceivedSN - m_uiNextExpectedSN;
    if ((uiDiff & 0x80000000) == 0)
    {
      // case 1: insert at end of list
      m_rtpPacketList.insert(m_rtpPacketList.end(), rtpPacket);
      m_receivedSequenceNumbers.insert(uiReceivedSN);
      m_uiNextExpectedSN = uiReceivedSN + 1;
      return true;
    }
    else
    {
      // case 2: to handle wrap around we map both the beginning and end sequence numbers into a range in the middle
      // and search for the first smaller sequence number starting at the end
      // insert according to RTP sequence number starting the search at the back. If no insertion point is
      // found, the packet is inserted at the beginning

      // map received sequence number into middle
      uint32_t uiMappedReceivedSequenceNumber = uiReceivedSN - 0x80000000;
      auto it = std::find_if(m_rtpPacketList.rbegin(), m_rtpPacketList.rend(), [uiMappedReceivedSequenceNumber](RtpPacket& rtpPacketInList)
      {
          uint32_t uiOrigSN = rtpPacketInList.getExtendedSequenceNumber();
          // map into middle range
          uint32_t uiMappedSequenceNumber = ((uiOrigSN & 0x80000000) == 0) ? uiOrigSN + 0x80000000 : uiOrigSN - 0x80000000;
          return uiMappedSequenceNumber < uiMappedReceivedSequenceNumber;
    });
      m_rtpPacketList.insert(it.base(), rtpPacket);
      m_receivedSequenceNumbers.insert(uiReceivedSN);
      return true;
    }
  }
  else
  {
    // two cases:
    // 1. wrap around has occurred and next expected packet was lost
    // 2. an old packet has been received that was assumed to have been lost
    uint32_t uiDiff = m_uiNextExpectedSN - uiReceivedSN;

    if ((uiDiff & 0x80000000) != 0)
    {
      // case 1: insert at end of list
      m_rtpPacketList.insert(m_rtpPacketList.end(), rtpPacket);
      m_receivedSequenceNumbers.insert(uiReceivedSN);
      m_uiNextExpectedSN = uiReceivedSN + 1;
      return true;
    }
    else
    {
      // case 2: old packet: find correct insertion point starting from end of list
      auto it = std::find_if(m_rtpPacketList.rbegin(), m_rtpPacketList.rend(), [uiReceivedSN](RtpPacket& rtpPacketInList)
      {
          // do signed comparison to handle wrap-around cases
          int32_t iSN = rtpPacketInList.getExtendedSequenceNumber();
          return iSN < (int32_t)uiReceivedSN;
    });
      m_rtpPacketList.insert(it.base(), rtpPacket);
      m_receivedSequenceNumbers.insert(uiReceivedSN);
      return true;
    }
  }
}

void RtpPacketGroup::sort()
{
  m_rtpPacketList.sort([](const RtpPacket& rtpPacket1, const RtpPacket& rtpPacket2)
  {
    return rtpPacket1.getSource().getPort() < rtpPacket2.getSource().getPort();
  });
}

} // rtp_plus_plus
