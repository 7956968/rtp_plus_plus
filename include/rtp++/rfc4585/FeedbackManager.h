#pragma once
#include <unordered_map>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/IFeedbackManager.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>
// TODO: get rid of inheritance?
#include <rtp++/rto/RtoManagerInterface.h>
#include <rtp++/rto/BasicRtoEstimator.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

/**
 * @brief The FeedbackManager class is responsible for managing the RTCP feedback for single path RTP scenarios.
 * If the session parameters support retransmission, an RTO estimator will be created.
 * This version of the feedback manager is designed to deal with one host i.e. it is
 * not possible to use this class to send feedback to multiple other participants
 * TODO: need to change this to store lost packets per SSRC
 * if we need to handle incoming packets from multiple SSRCs
 */
class FeedbackManager : public IFeedbackManager
{
public:
  /**
   * @brief FeedbackManager
   * @param rtpParameters
   * @param applicationParameters
   * @param rIoService
   */
  FeedbackManager(const RtpSessionParameters& rtpParameters,
                  const GenericParameters& applicationParameters,
                  boost::asio::io_service& rIoService);

  void reset();
  void shutdown();

  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN);
  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN);
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN);
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN);
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiSN);

  // OLD interface
#if 0
  /**
   * @brief retrieveFeedbackReports retrieves all feedback reports generated in the last interval
   * @return
   *
   * @DEPRECATED
   */
  void retrieveFeedbackReports(CompoundRtcpPacket& fb);
  /**
   * @brief retrieveFeedbackReports retrieves all flow-specific feedback reports generated in the last interval
   * @return
   *
   * @DEPRECATED
   */
  void retrieveFeedbackReports(uint16_t uiFlowId, CompoundRtcpPacket& fb);
#endif

  /**
   * @brief getAssumedLostCount
   * @return
   */
  uint32_t getAssumedLostCount() const;
  /**
   * @brief getReceivedCount
   * @return
   */
  uint32_t getReceivedCount() const;
  /**
   * @brief getAssumedLost Gets a vector containing sequence numbers of packets assumed lost
   * @return
   *
   * This is applicable when the application is using NACK feedback
   */
  std::vector<uint32_t> getAssumedLost();
  /**
   * @brief getReceived Gets a vector containing sequence numbers of packets received.
   * @return
   *
   * This is applicable when the application is using ACK feedback
   */
  std::vector<uint32_t> getReceived();

  virtual uint32_t getFlowSpecificLostCount() const{ return 0; }
  virtual std::vector<std::tuple<uint16_t, uint16_t>> getFlowSpecificLost(){ return std::vector<std::tuple<uint16_t, uint16_t>>(); }

private:
  /**
   * Callback for loss estimator. This method should be called if an RTP packet is late. In the case where
   * the feedback has not been sent yet, we can avoid a bad report
   * @param uiSN: the sequence number of the late RTP packet
   */
  bool onRtpPacketLate(uint32_t uiSN);
  /**
   * Callback for loss estimator. This method should be called if an RTP packet is assumed to be lost
   * @param uiSN: the sequence number of the packet to be assumed lost
   */
  void onRtpPacketAssumedLost(uint32_t uiSN);

  // OLD interface
#if 0
  /**
   * @brief addGenericNacks
   * @param vFeedback
   */
  void addGenericNacks(CompoundRtcpPacket& vFeedback);
  /**
   * @brief addGenericAcks
   * @param vFeedback
   */
  void addGenericAcks(CompoundRtcpPacket& vFeedback);
#endif

private:

  enum FeedbackMode
  {
    FB_NONE,
    FB_NACK,
    FB_ACK
  };

  FeedbackMode m_eFeedbackMode;
  // RTO
  std::unique_ptr<rto::PacketLossDetectionBase> m_pRtoEstimator;
  // LOST
  std::vector<uint32_t> m_vAssumedLost;
  // received SN
  std::vector<uint32_t> m_vReceivedSN;
};

} // rfc4585
} // rtp_plus_plus
