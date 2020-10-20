#pragma once
#include <rtp++/rto/MultipathRtoManagerInterface.h>
#include <map>
#include <set>
#include <unordered_map>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <cpputil/GenericParameters.h>
#include <cpputil/RunningAverageQueue.h>
#include <rtp++/rto/PredictorBase.h>

namespace rtp_plus_plus
{
namespace rto
{

struct RtoData
{
  RtoData()
    :FlowId(0),
    GSN(0),
    FSSN(0)
  {

  }

  RtoData(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival)
    :FlowId(uiFlowId),
    GSN(uiSN),
    FSSN(uiFSSN),
    Arrival(tArrival)
  {

  }

  uint16_t FlowId;
  uint16_t GSN;
  uint16_t FSSN;
  boost::posix_time::ptime Arrival;
};

typedef PredictorBase<uint64_t, int64_t> InterarrivalPredictor_t;

/**
  * This implementation is targeted at MPRTP with two flows
  */
class CrossPathRtoEstimator : public MultipathPacketLossDetectionBase
{
public:
  CrossPathRtoEstimator(boost::asio::io_service& rIoService, const GenericParameters &applicationParameters);
  ~CrossPathRtoEstimator();

protected:
  void doOnPacketArrival(  const boost::posix_time::ptime& tArrival, const uint16_t uiSN, const uint16_t uiFlowId, const uint16_t uiFSSN);
  void doOnRtxRequested(const boost::posix_time::ptime& tRequested, const int uiSN, const int uiFlowId, const int uiFSSN)
  {
//    if (uiFlowId != NOT_SET && uiFSSN != NOT_SET)
//    {
//      m_mEstimators[uiFlowId]->onRtxRequested(tRequested, uiFSSN);
//    }
  }
  void doOnRtxPacketArrival(const boost::posix_time::ptime& tArrival,
                                  const int uiSN, const int uiFlowId, const int uiFSSN,
                                  bool bLate, bool bDuplicate)
  {
//    if (uiFlowId != NOT_SET && uiFSSN != NOT_SET)
//    {
//      m_mEstimators[uiFlowId]->onRtxPacketArrival(tArrival, uiFSSN, bLate, bDuplicate);
//    }
  }
  virtual void doStop();

private:
  void onRtpPacketAssumedLostInFlow(uint16_t uiFSSN, uint16_t uiFlowId);

  /**
    * This method is called once a timer expires.
    * If the ec is 0, then the packet is assumed to be lost, and a timer for the packet with the next
    * greater sequence number is started.
    * Otherwise the timer was cancelled which means that the packet with the sequence number was received.
    * \param ec The error code of the timer as explained above
    * \param uiSN The sequence number of the packet
    */
  void onShortTimeout(const boost::system::error_code& ec, uint16_t uiSN, uint32_t uiDelta);
  void onLongTimeout(const boost::system::error_code& ec, uint16_t uiSN, uint32_t uiDelta);

  void checkState(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival);
  void updateModel(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival);
  bool learnFlowOrder();
  bool searchForReferencePacketInFastestFlow(const uint16_t uiFlowId);
  int determineCrossPathDifference(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival);
  void processNewPacket(const uint16_t uiFlowId, const uint16_t uiSN, const boost::posix_time::ptime& tArrival);
private:

  enum State
  {
      ST_LEARN_FLOW_ORDER = 0,
      ST_LEARN_PATH_DIFFERENCE,
      ST_LEARN_AWAITING_PACKET_FROM_ALTERNATE_FLOW,
      ST_EST_AWAITING_PACKET_FROM_FASTEST_FLOW,
      ST_EST_RUNNING_A,
      ST_EST_RUNNING_B
  };

  /// asio service
  boost::asio::io_service& m_rIoService;
  /// timer for RTO
  boost::asio::deadline_timer m_referenceTimer;
  /// Learning state
  State m_eState;
  /// shutdown flag
  bool m_bShuttingdown;

  // single path predictor
  std::unique_ptr<InterarrivalPredictor_t> m_pDeltaPredictor;
  // cross path predictor
  std::unique_ptr<InterarrivalPredictor_t> m_pCrosspathPredictor;

  std::map<uint16_t, boost::posix_time::ptime> m_mLastArrival;

  std::map<uint16_t, boost::circular_buffer<RtoData> > m_mFlowData;
  std::set<uint16_t> m_FlowIds;

  // keep track of average intertransmission interval
  RunningAverageQueue<uint32_t> m_deltaQueue;

  std::vector<uint16_t> m_vFlowOrder;
  RtoData m_reference;
  uint16_t m_uiWaitingForPacketFlow;
  uint16_t m_uiMissingSN;
  uint32_t m_uiCrossPathTimeout;

  uint32_t m_uiReferenceSN;

  bool m_bRestartShortTimer;
  uint16_t m_uiExpectingSN;
  mutable boost::mutex m_lock;
  std::unordered_map<uint16_t, boost::asio::deadline_timer*> m_mLongTimers;
};

} // rto
} // rtp_plus_plus
