#pragma once
#include <cstdint>
#include <boost/function.hpp>
#include <rtp++/RtcpPacketBase.h>

namespace rtp_plus_plus
{

/**
 * @brief The IFeedbackManager class abstracts the feedback interface for single and multipath
 * RTP sessions. This class defines the feedback manager for a single SSRC.
 * When a packet is assumed to be lost, the loss manager should fire the m_onLoss
 * callback if it has been set. The calling class is responsible for then
 * calling the retrieveFeedbackReports method. Since these operations may be
 * asynchronous (e.g. if the RTCP report manager delays the creation of the
 * report, etc.), there is no guarantee that there will be a loss report by
 * the time that retrieveFeedbackReports is called.
 *
 */
class IFeedbackManager
{
public:
  /// loss callback for single path and global multipath scenarios
  typedef boost::function<void (uint32_t)> LossCallback_t;
  /// loss callback for multipath loss scenarios: uiFlowId, uiFSSN
  typedef boost::function<void (uint16_t, uint16_t)> FlowSpecificLossCallback_t;

  virtual ~IFeedbackManager()
  {
  }

  void setLossCallback( LossCallback_t fnOnLoss ) { m_onLoss = fnOnLoss; }

  void setFlowSpecificLossCallback( FlowSpecificLossCallback_t fnFlowSpecificLoss ) { m_onFlowSpecificLoss = fnFlowSpecificLoss; }
  /**
   * The reset method should be called to reset state. This might be the case
   * when a BYE is received from the other participant
   */
  virtual void reset() = 0;
  /**
   * The shutdown method should be called on application exit.
   * Implementations should cleanly finish tasks that have been started.
   */
  virtual void shutdown() = 0;
  /**
   * This method should be called every time a packet arrives.
   */
  virtual void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN) = 0;
  /**
   * This method should be called every time a retransmission packet arrives.
   */
  virtual void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN) = 0;
  /**
   * This method should be called every time a packet arrives in multipath scenarios.
   */
  virtual void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN) = 0;
  /**
   * This method should be called every time a retransmission packet arrives in multipath scenarios.
   */
  virtual void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN) = 0;

  // new API
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiSN){}
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiFlowId, const uint16_t uiSN){}

  /**
   * @brief getAssumedLostCount
   * @return
   */
  virtual uint32_t getAssumedLostCount() const = 0;
  /**
   * @brief getReceivedCount
   * @return
   */
  virtual uint32_t getReceivedCount() const = 0;
  /**
   * @brief getAssumedLost
   * @return
   */
  virtual std::vector<uint32_t> getAssumedLost() = 0;
  /**
   * @brief getReceived
   * @return
   */
  virtual std::vector<uint32_t> getReceived() = 0;
  /**
   * @brief getAssumedLostCount
   * @return
   */
  virtual uint32_t getFlowSpecificLostCount() const = 0;
  /**
   * @brief getAssumedLost
   * @return
   */
  virtual std::vector<std::tuple<uint16_t, uint16_t>> getFlowSpecificLost() = 0;

protected:

  /// This callback should be invoked when a global loss is detected
  LossCallback_t m_onLoss;
  /// This callback should be invoked when a global loss is detected
  FlowSpecificLossCallback_t m_onFlowSpecificLoss;
};

} // rtp_plus_plus
