#pragma once
#include <rtp++/rto/RtoManagerInterface.h>

namespace rtp_plus_plus
{
namespace rto
{

/**
 * @brief The SimplePacketLossDetection class is the most basic estimator. A packet is assumed lost when
 * there is a gap in sequence number
 */
class SimplePacketLossDetection : public PacketLossDetectionBase
{
public:
  /**
   * @brief SimplePacketLossDetection
   */
  SimplePacketLossDetection();
  /**
   * @brief ~SimplePacketLossDetection
   */
  ~SimplePacketLossDetection();
  /**
   * @brief This method resets the prediction model when things go wrong.
   * It should be called if the other party sends a BYE
   */
  void reset();

protected:
  /**
   * @brief This method should be called each time a non-retransmission RTP packet arriv
   * @param tArrival The arrival time of the RTP packet
   * @param uiSN The sequence number of the RTP packet
   */
  void doOnPacketArrival(const boost::posix_time::ptime &tArrival, const uint32_t uiSN);
  virtual void doStop();

private:

  /// shutdown flag
  bool m_bShuttingdown;
  /// first
  bool m_bFirst;
  /// keeps track of SN
  uint32_t m_uiPreviouslyReceivedMaxSN;
};

} // rto
} // rtp_plus_plus
