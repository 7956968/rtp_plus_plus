#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <rtp++/rto/StatisticsManager.h>

namespace rtp_plus_plus
{
namespace rto
{

/**
 * @brief The PacketLossDetectionBase class detects packet loss for a  single SSRC
 */
class PacketLossDetectionBase
{
public:
  /// This callback allows the class to communicate if packets are assumed lost
  typedef boost::function<void (uint32_t)> LostCallback_t;
  /// This callback allows the class to try to cancel an RTX request if the RTX has not been sent yet
  typedef boost::function<bool (uint32_t)> FalsePositiveCallback_t;
  /**
   * @brief PacketLossDetectionBase constructor
   */
  PacketLossDetectionBase()
    :m_id(-1),
      m_bStopped(false)
  {

  }
  /**
   * @brief ~PacketLossDetectionBase destructor
   */
  virtual ~PacketLossDetectionBase()
  {
    if (!m_bStopped)
    {
      LOG(ERROR) << "Warning: the stop method has not been called prior to destruction: this could cause access violations";
    }
    m_statisticsManager.printStatistics();
  }
  /**
   * @brief reset Under certain circumstances, the detection algorithm may need to be reset
   */
  virtual void reset(){}
  /**
   * @brief The Id can be configured for logging purposes and serves no other function
   * @param[in] id The ID to be used for logging
   */
  void setId(int32_t id) { m_id = id; }
  /**
   * @brief This method gives implementations an opportunity to clean up before the destructor
   * is called, which is particulat important for asynchronous operation completion.
   * Note: This method must be called before the destructor to shutdown
   * the asynchronous timers cleanly
   */
  void stop()
  {
    // set flag for clean destruction
    m_bStopped = true;
    doStop();
  }
  /**
   * @brief This method must be called to configure the callback
   * which will be called every time a packet is assumed to be lost.
   * @param onLost The callback to be invoked when a packet is assumed to be lost.
   */
  void setLostCallback(LostCallback_t onLost) { m_onLost = onLost; }
  /**
   * @brief This method must be called to configure the callback
   * which will be called every time a packet is late
   * @param onLate The callback to be invoked when a packet is late
   */
  void setFalsePositiveCallback(FalsePositiveCallback_t onLate) { m_onFalsePositive = onLate; }
  /**
   * @brief onPacketArrival This method should be called for each RTP packet that is received, that is not a retransmission!
   * If a retransmission is received, the onRtxPacketArrival should be called
   * The implementation should call the lost callback once it has assumed that a packet has been lost
   * @param tArrival The arrival time of the RTP packet
   * @param uiSN The (extended) sequence number of the RTP packet
   */
  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN)
  {
    m_statisticsManager.receivePacket(tArrival, uiSN);
    doOnPacketArrival(tArrival, uiSN);
  }
  /**
   * @brief onRtxRequested This method should be called when a NACK for a packet is sent
   * @param tRequested
   * @param uiSN
   */
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint32_t uiSN)
  {
    m_statisticsManager.retransmissionRequested(tRequested, uiSN);
  }
  /**
   * @brief This method should be called when a previously estimated lost packet is received.
   * @param uiSN The sequence number of the packet which was previously lost and for which the retransmission has now been received.
   * @param bLate If the retransmission was late
   * @param bDuplicate if the retransmission was a duplicate
   */
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, bool bLate, bool bDuplicate)
  {
    m_statisticsManager.receiveRtxPacket(tArrival, uiSN, bLate, bDuplicate);
  }

protected:
  virtual void doOnPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN) = 0;
  virtual void doStop() = 0;

  /// Id which can be set for logging purposes
  int32_t m_id;
  ///
  LostCallback_t m_onLost;
  FalsePositiveCallback_t m_onFalsePositive;
  bool m_bStopped;
  StatisticsManager m_statisticsManager;
};

} // rto
} // rtp_plus_plus
