#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>

namespace rtp_plus_plus
{
namespace rto
{

/**
 * @brief The PacketTransmissionInfo class stores details regarding packet state.
 *
 * A packet is estimated to arrive at a certain time.
 * It may be assumed lost.
 * If it arrives after having been assumed lost, it's a false positive.
 * A retransmission may be requested.
 * The retransmission may arrive at some time later.
 *
 */
class PacketTransmissionInfo
{
  friend std::ostream& operator<< (std::ostream& stream, const PacketTransmissionInfo& info);

public:
  /**
   * @brief PacketTransmissionInfo
   */
  PacketTransmissionInfo()
    :m_bRtxCancelled(false)
  {

  }
  /**
   * @brief PacketTransmissionInfo
   * @param tEstimated Time the packet is estimated to arrive at
   */
  PacketTransmissionInfo(const boost::posix_time::ptime& tEstimatedArrival)
    :m_tEstimated(tEstimatedArrival),
      m_bRtxCancelled(false)
  {

  }
  /**
   * @brief ~PacketTransmissionInfo
   */
  ~PacketTransmissionInfo()
  {

  }
  /**
   * @brief getErrorUs
   * @return
   */
  uint32_t getErrorUs() const
  {
    // we're assuming that this number is fairly small. Downcast should be fine.
    return static_cast<uint32_t>((m_tEstimated - m_tArrival).total_microseconds());
  }
  /**
   * @brief getRtxRequestDelayUs
   * @return
   */
  uint32_t getRtxRequestDelayUs() const
  {
    // we're assuming that this number is fairly small. Downcast should be fine.
    return static_cast<uint32_t>((m_tRtxRequested - m_tEstimated).total_microseconds());
  }
  /**
   * @brief getRetransmissionTimeUs
   * @return
   */
  uint32_t getRetransmissionTimeUs() const
  {
    // we're assuming that this number is fairly small. Downcast should be fine.
    return static_cast<uint32_t>((m_tRtxArrival - m_tRtxRequested).total_microseconds());
  }
  /**
   * @brief getTotalRetransmissionTimeUs
   * @return
   */
  uint32_t getTotalRetransmissionTimeUs() const
  {
    // we're assuming that this number is fairly small. Downcast should be fine.
    return static_cast<uint32_t>((m_tRtxArrival - m_tEstimated).total_microseconds());
  }
  /**
   * @brief getEstimated
   * @return
   */
  const boost::posix_time::ptime& getEstimated() const
  {
    return m_tEstimated;
  }
  /**
   * @brief setEstimated
   * @param tTime
   */
  void setEstimated(const boost::posix_time::ptime& tTime)
  {
    m_tEstimated = tTime;
  }
  /**
   * @brief getArrival
   * @return
   */
  const boost::posix_time::ptime& getArrival() const
  {
    return m_tArrival;
  }
  /**
   * @brief setArrival
   * @param tTime
   */
  void setArrival(const boost::posix_time::ptime& tTime)
  {
    m_tArrival = tTime;
  }
  /**
   * @brief isAssumedLost
   * @return
   */
  bool isAssumedLost() const
  {
    return !m_tAssumedLost.is_not_a_date_time();
  }
  /**
   * @brief isFalsePositive
   * @return
   */
  bool isFalsePositive() const
  {
    return !m_tFalsePositive.is_not_a_date_time();
  }
  /**
   * @brief getAssumedLost
   * @return
   */
  const boost::posix_time::ptime& getAssumedLost() const
  {
    return m_tAssumedLost;
  }
  /**
   * @brief setAssumedLost
   * @param tTime
   */
  void setAssumedLost(const boost::posix_time::ptime& tTime)
  {
    m_tAssumedLost = tTime;
  }
  /**
   * @brief getRtxRequested
   * @return
   */
  const boost::posix_time::ptime& getRtxRequested() const
  {
    return m_tRtxRequested;
  }
  /**
   * @brief setRtxRequestedAt
   * @param tTime
   */
  void setRtxRequestedAt(const boost::posix_time::ptime& tTime)
  {
    m_tRtxRequested = tTime;
  }
  /**
   * @brief getRtxArrival
   * @return
   */
  const boost::posix_time::ptime& getRtxArrival() const
  {
    return m_tRtxArrival;
  }
  /**
   * @brief setRtxArrival
   * @param tTime
   */
  void setRtxArrival(const boost::posix_time::ptime& tTime)
  {
    m_tRtxArrival = tTime;
  }
  /**
   * @brief getFalsePositive
   * @return
   */
  const boost::posix_time::ptime& getFalsePositive() const
  {
    return m_tFalsePositive;
  }
  /**
   * @brief setFalsePositive
   * @param tTime
   */
  void setFalsePositive(const boost::posix_time::ptime& tTime)
  {
    m_tFalsePositive = tTime;
  }
  /**
   * @brief setRtxCancelled should be called if the NACK could be cancelled before
   * the RTCP FB is sent.
   */
  void setRtxCancelled()
  {
    m_bRtxCancelled = true;
  }

private:
  /// time packet was estimated to arrive
  boost::posix_time::ptime m_tEstimated;
  /// Actual packet arrival time
  boost::posix_time::ptime m_tArrival;
  /// Time packet was assumed lost
  boost::posix_time::ptime m_tAssumedLost;
  /// Time packet was marked as false positive
  boost::posix_time::ptime m_tFalsePositive;
  /// Time retransmission was requested at
  boost::posix_time::ptime m_tRtxRequested;
  /// Arrival time of retransmitted packet
  boost::posix_time::ptime m_tRtxArrival;
  /// If RTX is cancelled
  bool m_bRtxCancelled;
};

} // rto
} // rtp_plus_plus
