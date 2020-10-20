#pragma once
#include <list>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <rtp++/RtpPacket.h>

namespace rtp_plus_plus
{

/**
 * @brief The RtpPacketGroup class: An RtpPacketGroup stores RTP packets that have to be depacketised together.
  * This are generally packets with the same RTP timestamp. In the case of multi-session transmission the
  * presentation time has to be used since RTP timestamps have random offsets in each RTP session.
  *
 */
class RtpPacketGroup
{
public:

  RtpPacketGroup(const RtpPacket& rtpPacket, const boost::posix_time::ptime& tPresentation, bool bRtcpSynchronised, const boost::posix_time::ptime& tPlayout);

  uint32_t getRtpTimestamp() const { return m_uiRtpTs; }
  bool isRtcpSynchronised() const { return m_bRtcpSynchronised;  }
  boost::posix_time::ptime getArrivalTime() const { return m_tArrival; }
  boost::posix_time::ptime getPresentationTime() const { return m_tPresentation; }
  boost::posix_time::ptime getPlayoutTime() const { return m_tPlayout; }
  const std::list<RtpPacket>& getRtpPackets() const { return m_rtpPacketList; }
  uint32_t getOldestExtendedSN() const { return m_rtpPacketList.front().getSequenceNumber(); }
  uint32_t getNextExpectedSN() const { return m_uiNextExpectedSN; }

  /**
   * @brief getSize Returns the number of items in the group
   * @return The number of items in the group
   */
  uint32_t getSize() const { return m_rtpPacketList.size(); }

  /**
   * @return this method returns true if the RTP packet was added in sequence number order
   *         and false if it is a duplicate packet (according to the extended RTP sequence number)
   */
  bool insert(const RtpPacket& rtpPacket);

  // sort function according to source port: should this be the receiving port?
  void sort();

private:

  /// RTP timestamp for this node
  uint32_t m_uiRtpTs;
  /// the time that the node was created
  boost::posix_time::ptime m_tArrival;
  /// calculated presentation time of node
  boost::posix_time::ptime m_tPresentation;
  /// RTCP sync
  bool m_bRtcpSynchronised;
  /// calculated playout time of node
  boost::posix_time::ptime m_tPlayout;
  /// Payload data from RTP packet
  std::list<RtpPacket> m_rtpPacketList;
  /// set of all received sequence numbers used for checking for duplicates
  std::set<uint32_t> m_receivedSequenceNumbers;
  /// sequence number of next expected packet
  uint32_t m_uiNextExpectedSN;
};

} // rtp_plus_plus
