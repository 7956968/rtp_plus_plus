#pragma once
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/rto/PacketTransmissionInfo.h>
#include <rtp++/rto/RetransmissionInfo.h>

namespace rtp_plus_plus
{
namespace rto
{

/**
 * @brief This class manages packet statistics such as estimation, arrival, losses, rtx, etc.
 *
 * Replacing uint16_t with uint32_t for sequence numbers as this caters for using extended sequence numbers.
 */
class StatisticsManager : public boost::noncopyable
{
public:
  /**
   * @brief StatisticsManager
   */
  StatisticsManager();
  /**
   * @brief setId
   * @param id
   */
  void setId(int32_t id) { m_id = id; }
  /**
   * @brief reset
   */
  void reset()
  {
    LOG(WARNING) << "TODO:RESET STATS";
  }
  /**
   * @brief printStatistics This method prints out all the statistics to the logger
   */
  void printStatistics();
  /**
   * @brief This method checks if the packet with the specified sequence number
   * was previously assumed to be lost
   * @param uiSN the sequence number of the packet to be checked
   * @return if the packet was previously assumed to be lost
   */
  bool isPacketAssumedLost(const uint32_t uiSN) const;
  /**
   * @brief This method checks if the packet with the specified sequence number
   * was a false positive.
   * @param uiSN the sequence number of the packet to be checked
   * @return if the packet is a false positive
   */
  bool isFalsePositive(const uint32_t uiSN) const;
  /**
   * @brief This method checks if a packet has been received recently
   * It does so by means of a size-limited circular queue
   * @param[in] uiSN The sequence number of the packet to be checked
   */
  bool hasPacketBeenReceivedRecently(const uint32_t uiSN) const;
  /**
   * @brief This method should be called when the arrival time of a packet is estimated
   * @param[in] tEstimation The estimated arrival time of the packet
   * @param[in] uiSN The sequence number of the packet received
   */
  void estimatePacketArrivalTime(const boost::posix_time::ptime &tEstimation, const uint32_t uiSN);
  /**
   * @brief This method should be called when the packet arrives
   * @param[in] tArrival The arrival time of the packet
   * @param[in] uiSN The sequence number of the packet received
   */
  void receivePacket(const boost::posix_time::ptime &tArrival, const uint32_t uiSN);
  /**
   * @brief This method should be called when an RTX packet arrives
   * @param[in] tArrival The arrival time of the RTX packet
   * @param[in] uiSN The sequence number of the original packet received
   */
  void receiveRtxPacket(const boost::posix_time::ptime &tArrival, const uint32_t uiSN, bool bLate, bool bDuplicate);
  /**
   * @brief This method should be called when a packet has been assumed lost
   * @param[in] tLost The time the packet was assumed lost
   * @param[in] uiSN The sequence number of packet assumed lost
   */
  void assumePacketLost(const boost::posix_time::ptime &tLost, const uint32_t uiSN);
  /**
   * @brief This method should be called once an RTX has been requested
   * @param[in] tRequested The time the RTX packet was requested
   * @param[in] uiSN The original sequence number of packet for which the RTX was requested
   */
  void retransmissionRequested(const boost::posix_time::ptime& tRequested, const uint32_t uiSN);
  /**
   * @brief This method should be called if an RTX could be removed before the actual request occurred
   * @param[in] uiSN The sequence number of the packet for which the RTX could be cancelled
   */
  void falsePositiveRemovedBeforeRetransmission(uint32_t uiSN);
  /**
   * @brief setRtxCancelled should be called if an RTX is cancelled successfully
   * @param uiSN
   */
  void setRtxCancelled(uint32_t uiSN);
private:

  int32_t m_id;
  /// Total number of packets received excluding RTX
  uint32_t m_uiTotalOriginalPacketsReceived;
  /// Total number of RTX packets received
  uint32_t m_uiTotalRtxPacketsReceived;
  mutable boost::mutex m_lock;
  /// Circular buffer to keep track of recently received packets
  boost::circular_buffer<uint32_t> m_qRecentlyReceived;
  // TOREMOVE
  // TODO: replace RetransmissionInfo with m_mPacketTransmissionInfo
  // RetransmissionInfo m_rtxInfo;
  // TODO: replace with multimap to handle SN rollovers
  std::unordered_map<uint32_t, PacketTransmissionInfo> m_mPacketTransmissionInfo;
};

} // rto
} // rtp_plus_plus
