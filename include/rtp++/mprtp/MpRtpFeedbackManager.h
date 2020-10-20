#pragma once
#include <unordered_map>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/IFeedbackManager.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>
#include <rtp++/rto/MultipathRtoEstimator.h>

namespace rtp_plus_plus
{

namespace mprtp
{

typedef boost::function<void (const uint16_t uiFlow, CompoundRtcpPacket&) > MpFeedbackCallback_t;

class FeedbackManager : public IFeedbackManager
{
public:

  FeedbackManager(const RtpSessionParameters& rtpParameters,
                  const GenericParameters &applicationParameters,
                  boost::asio::io_service &rIoService);

  void reset();
  void shutdown();
  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN);
  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN);
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN);
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN);
  // OLD API
#if 0
  void retrieveFeedbackReports(CompoundRtcpPacket& feedback);
  void retrieveFeedbackReports(uint16_t uiFlowId, CompoundRtcpPacket& feedback);
#endif
  // new API
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiSN);
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiFlowId, const uint16_t uiSN);

  uint32_t getAssumedLostCount() const;
  uint32_t getReceivedCount() const;
  std::vector<uint32_t> getAssumedLost();
  std::vector<uint32_t> getReceived();
  virtual uint32_t getFlowSpecificLostCount() const { return m_vFlowSpecificLosses.size(); }
  virtual std::vector<std::tuple<uint16_t, uint16_t>> getFlowSpecificLost()
  {
    std::vector<std::tuple<uint16_t, uint16_t>> vAssumedLost;
    std::swap(vAssumedLost, m_vFlowSpecificLosses);
    return vAssumedLost;
  }

private:
  enum FeedbackMode
  {
    FB_NONE,
    FB_NACK,
    FB_ACK
  };
  FeedbackMode m_eFeedbackMode;

  /**
   * This method should be called if an RTP packet is assumed to be lost
   * @param uiSN: the sequence number of the packet to be assumed lost
   */
  void onRtpPacketAssumedLost(int uiSN, int uiFlowId, int uiFSSN);
  /**
   * This method should be called if an RTP packet is late. In the case where
   * the feedback has not been sent yet, we can avoid a bad report
   * @param uiSN: the sequence number of the late RTP packet
   */
  void onRtpPacketLate(int uiSN, int uiFlowId, int uiFSSN);
  // OLD API
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
  /// Primary RTO estimator
  std::unique_ptr<rto::MultipathPacketLossDetectionBase> m_pMpRtoEstimator;
  /// 2nd estimator for comparison mode
  std::unique_ptr<rto::MultipathPacketLossDetectionBase> m_pMpRtoEstimator2;
  // LOST:
  std::vector<uint32_t> m_vAssumedLost;
  std::vector<std::tuple<uint16_t, uint16_t>> m_vFlowSpecificLosses;
#if 0
  // Map to store RTX times
  std::unordered_map<uint16_t, boost::posix_time::ptime> m_mRtxDurationMap;
  std::unordered_map<uint32_t, boost::posix_time::ptime> m_mFlowSpecificRtxDurationMap;
#endif
  // received SN
  std::vector<uint32_t> m_vReceivedSN;
};

} // mprtp
} // rtp_plus_plus
