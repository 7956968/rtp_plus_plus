#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/rto/PredictorBase.h>
#include <rtp++/rto/RtoManagerInterface.h>

/// @def DEBUG_TIMEOUT_LATENCY Measures the latency between the timeout time and the actual timeout

namespace rtp_plus_plus
{
namespace rto
{

typedef PredictorBase<uint64_t, int64_t> InterarrivalPredictor_t;

/**
 * @brief Basic single path RTO estimator
 * FIXME TODO (14/02/2013): reminder about implementation error: if a packet is received that is not expected yet,
 * the estimator goes into a state from which it can not recover. This occured when an error
 * in the channel model caused a newer packet to arrive before an older one, which crashed the estimator
 */
class BasicRtoEstimator : public PacketLossDetectionBase
{
public:
  /**
   * @brief BasicRtoEstimator
   * @param rIoService
   */
  BasicRtoEstimator(boost::asio::io_service& rIoService);
  /**
   *@brief Destructor
   */
  ~BasicRtoEstimator();
  /**
   * @brief Setter method for linear predictor
   */
  void setPredictor(std::unique_ptr<InterarrivalPredictor_t> pPredictor)
  {
    m_pPredictor = std::move(pPredictor);
  }
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
  /**
   * @brief This method checks if the sequence number is relatively greater
   * than the previously received max SN taking rollover into account
   * @param uiSN The sequence number to be checked against the previous max
   */
  bool isGreaterThanPreviouslyReceivedSN( const uint32_t uiSN) const;
  /**
   * @brief This method checks whether the maximum difference in duration between packets
   * has been exceeded
   * @param tArrival The arrival time of the latest packet
   * @return If the maximum gap has been exceeded
   */
  bool hasMaximumTimeBetweenPacketsBeenExceeded(const boost::posix_time::ptime &tArrival);
  /**
   * @brief This method is called once a timer expires.
   * If the ec is 0, then the packet is assumed to be lost, and a timer for the packet with the next
   * greater sequence number is started.
   * Otherwise the timer was cancelled which means that the packet with the sequence number was received.
   * @param ec The error code of the timer as explained above
   * @param uiSN The sequence number of the packet
   */
  void onEstimationTimeout(const boost::system::error_code& ec, uint32_t uiSN);
  /**
   * @brief This method calculate when the next packet should be received based on the past history and schedules a new timeout for
   * the specified sequence number if there is sufficient data for the prediction
   * @param uiNextSN The sequence number of the packet that we are estimating the RTO for
   */
  void predictRtoAndScheduleTimer(uint32_t uiNextSN);
  /**
   * @brief This method schedules a timeout for the sequence number to occur as specified by the uiOffsetUs parameter
   * @param uiNextSN The sequence number of the packet for which the arrival time is being estimated
   * @param uiOffsetUs The offset in microseconds after which the timeout should occur.
   */
  void scheduleTimer(uint32_t uiNextSN, uint32_t uiOffsetUs);
  void scheduleTimerFromNow(uint32_t uiNextSN, uint32_t uiOffsetUs);
  /**
   * @brief This method inserts a new entry into the prediction model. If there is a gap between the new sequence
   * number and the previously received one, the interarrival time is averaged by dividing by the sequence
   * number difference.
   * @param tArrival The actual arrival time of the packet
   * @param uiSN The sequence number of the packet
   */
  bool insertNewPredictionEntryIfPossible(const boost::posix_time::ptime &tArrival, const uint32_t uiSN);

private:

  /// asio service
  boost::asio::io_service& m_rIoService;
  /// timer for RTO
  boost::asio::deadline_timer m_timer;
  /// shutdown flag
  bool m_bShuttingdown;
  /// reset flag
  bool m_bResetting;
  /// Timer outstanding
  bool m_bTimerActive;
  /// store sequence number of currently expected packet
  uint32_t m_uiSNCurrentlyExpected;
  /// Arrival time of the packet with the "greatest" SN
  boost::posix_time::ptime m_tPrev;
  /// stores the "greatest" SN received so far
  /// This variable allows us to not update the timers
  /// if an old packet arrives late
  uint32_t m_uiPreviouslyReceivedMaxSN;

  bool m_bFirstTimerScheduled;

  std::unique_ptr<InterarrivalPredictor_t> m_pPredictor;
  /// previous estimate
  uint32_t m_uiPrevEstimateUs;
  /// Stores number of consecutive losses so that estimation can be disabled once the max is reached
  uint32_t m_uiConsecutiveLosses;
};

} // rto
} // rtp_plus_plus
