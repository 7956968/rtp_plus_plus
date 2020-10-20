#pragma once
#include <algorithm>
#include <iterator>
#include <list>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cpputil/Buffer.h>
#include <rtp++/RtpPacket.h>

// typedef for extended sequence number and payload pair
// typedef std::pair<uint32_t, Buffer> PacketData_t;
// typedef std::list<PacketData_t> PacketDataList_t;

namespace rtp_plus_plus
{

/**
  * The CodecData struct stores the payload data
  * related to a media frame with the same RTP timestamp
  * sorted according to RTP sequence number
  */

struct CodedData
{
  /**
   * we will use the extended sequence number only since this has a very small chance of wrap-around
   * @return this method returns true if the payload was added according to sequence number
   *         and false if it is a duplicate
   */
  bool insert(uint32_t uiSequenceNumber, Buffer payload)
  {
    // insert according to RTP sequence number starting the search at the back
    auto it = std::find_if(PacketDataList.rbegin(), PacketDataList.rend(), [uiSequenceNumber](PacketData_t packetData)
    {
      return packetData.first <= uiSequenceNumber;
    });
    if (it != PacketDataList.rend() && it->first == uiSequenceNumber)
    {
      // Duplicate SN
      VLOG(1) << "Duplicate SN: " << uiSequenceNumber << "- discarding packet";
      return false;
    }
    else
    {
      PacketDataList.insert(it.base(), std::make_pair(uiSequenceNumber, payload));
      return true;
    }
  }

  PacketDataList_t getPacketData() const
  {
    return PacketDataList;
  }

  PacketDataList_t PacketDataList;
};

/**
  * The PlayoutBufferNode stores information
  * regarding the arrival time of the sample
  * as well as the calculated playout time.
  */
class PlayoutBufferNode
{
public:
  PlayoutBufferNode(const RtpPacket& rtpPacket, const boost::posix_time::ptime& tPlayout)
    :m_uiRtpTs(rtpPacket.getRtpTimestamp()),
    m_tArrival(rtpPacket.getArrivalTime()),
    m_tPlayout(tPlayout)
  {
    m_payloadData.insert(rtpPacket.getExtendedSequenceNumber(), rtpPacket.getPayload());
  }

  /**
   * @return this method returns true if the RTP packet was added according to sequence number
   *         and false if it is a duplicate
   */
  bool insertIfUnique(const RtpPacket& rtpPacket)
  {
    return m_payloadData.insert(rtpPacket.getExtendedSequenceNumber(), rtpPacket.getPayload());
  }

  uint32_t getRtpTs() const { return m_uiRtpTs; }
  boost::posix_time::ptime getArrivalTime() const { return m_tArrival; }
  boost::posix_time::ptime getPlayoutTime() const { return m_tPlayout; }
  PacketDataList_t getPacketData() const { return m_payloadData.getPacketData(); }
  uint32_t getOldestSN() const
  {
    return m_payloadData.getPacketData().front().first;
  }

private:

  /// RTP timestamp for this node
  uint32_t m_uiRtpTs;
  /// the time that the node was created
  boost::posix_time::ptime m_tArrival;
  boost::posix_time::ptime m_tPlayout;
  /// Payload data from RTP packet
  CodedData m_payloadData;
};

}
