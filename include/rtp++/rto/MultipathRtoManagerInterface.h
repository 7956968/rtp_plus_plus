#pragma once
#include <cstdint>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <rtp++/rto/StatisticsManager.h>

namespace rtp_plus_plus
{
namespace rto
{

// TODO: change to int so that we can use NOT_SET
typedef boost::function<void (int, int, int)> MpLostCallback_t;

static const int SN_NOT_SET = -1;

/**
 * @brief The MultipathPacketLossDetectionBase class
 */
class MultipathPacketLossDetectionBase
{
public:
  /**
   * @brief MultipathPacketLossDetectionBase
   */
  MultipathPacketLossDetectionBase()
    :m_bStopped(false)
  {

  }
  /**
   * @brief ~MultipathPacketLossDetectionBase
   */
  virtual ~MultipathPacketLossDetectionBase()
  {
    if (!m_bStopped)
    {
      LOG(ERROR) << "Warning: the stop method has not been called prior to destruction: this could cause access violations";
    }
    m_statisticsManager.printStatistics();
    for (auto& it: m_mFlowStatistics)
    {
      it.second->printStatistics();
      delete it.second;
    }
  }
  /** @brief This method gives implementations an opportunity to clean up before the destructor
   * is called, which is particulat important for anyvhronous operations
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
  void setLostCallback(MpLostCallback_t onLost) { m_onLost = onLost; }
  /**
   * @brief This method must be called to configure the callback
   * which will be called every time a packet is late
   * @param onLate The callback to be invoked when a packet is late
   */
  void setFalsePositiveCallback(MpLostCallback_t onLate) { m_onFalsePositive = onLate; }
  /**
   * @brief This method should be called for each RTP packet that is received, that is not a retransmission!
   * If a retransmission is received, the onRtxPacketArrival should be called
   * The implementation should call the lost callback once it has assumed that a packet has been lost
   */
  void onPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, const uint16_t uiFlowId, const uint16_t uiFSSN )
  {
    // global SN stats
    m_statisticsManager.receivePacket(tArrival, uiSN);
    // flow stats
    if (m_mFlowStatistics.find(uiFlowId) == m_mFlowStatistics.end())
    {
      m_mFlowStatistics[uiFlowId] = new StatisticsManager();
    }
    m_mFlowStatistics[uiFlowId]->receivePacket(tArrival, uiFSSN);
    // TODO: set ID/Flow of new flows
    doOnPacketArrival(tArrival, uiSN, uiFlowId, uiFSSN);
  }
  /**
   * @brief onRtxRequested This method should be called when a NACK for a packet is sent. int is used as the data type
   * since the receiver can use either the SN, Flow Id + FSSN, or all three. When a parameter is not used, the value
   * NOT_SET should be used.
   * @param tRequested
   * @param uiSN
   * @param uiFlowId
   * @param uiFSSN
   */
  void onRtxRequested(const boost::posix_time::ptime& tRequested, const int uiSN, const int uiFlowId, const int uiFSSN)
  {
    if (uiSN != SN_NOT_SET)
    {
      m_statisticsManager.retransmissionRequested(tRequested, uiSN);
    }
    if (uiFlowId != SN_NOT_SET && uiFSSN != SN_NOT_SET)
    {
      m_mFlowStatistics[uiFlowId]->retransmissionRequested(tRequested, uiFSSN);
    }
    doOnRtxRequested(tRequested, uiSN, uiFlowId, uiFSSN);
  }
  /**
   * @brief This method should be called when a previously estimated lost packet is received.
   * @param uiSN The sequence number of the packet which was previously lost and for which the retransmission has now been received.
   * @param bLate If the retransmission was late
   * @param bDuplicate if the retransmission was a duplicate
   */
  void onRtxPacketArrival(const boost::posix_time::ptime& tArrival,
                                  const int uiSN, const int uiFlowId, const int uiFSSN,
                                  bool bLate, bool bDuplicate)
  {
#if 0
    VLOG(2) << "On RTX arrival SN: " << uiSN << " Flow: " << uiFlowId << " FSSN: " << uiFSSN;
#endif
    if (uiSN != SN_NOT_SET)
    {
#if 0
      VLOG(2) << "On RTX arrival SN: " << uiSN;
#endif
      m_statisticsManager.receiveRtxPacket(tArrival, uiSN, bLate, bDuplicate);
    }
    if (uiFlowId != SN_NOT_SET && uiFSSN != SN_NOT_SET)
    {
#if 0
      VLOG(2) << "On RTX arrival Flow: " << uiFlowId << " FSSN: " << uiFSSN;
#endif
      m_mFlowStatistics[uiFlowId]->receiveRtxPacket(tArrival, uiFSSN, bLate, bDuplicate);
    }
    doOnRtxPacketArrival(tArrival, uiSN, uiFlowId, uiFSSN, bLate, bDuplicate);
  }

protected:
  virtual void doOnPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, const uint16_t uiFlowId, const uint16_t uiFSSN ) = 0;
  virtual void doOnRtxRequested(const boost::posix_time::ptime& tRequested, const int uiSN, const int uiFlowId, const int uiFSSN) = 0;
  virtual void doOnRtxPacketArrival(const boost::posix_time::ptime& tArrival,
                                    const int uiSN, const int uiFlowId, const int uiFSSN,
                                    bool bLate, bool bDuplicate) = 0;

  virtual void doStop() = 0;

  MpLostCallback_t m_onLost;
  MpLostCallback_t m_onFalsePositive;
  bool m_bStopped;

  StatisticsManager m_statisticsManager;
  // TODO: is this necessary: when re-using the single path components
  // they alreayd log all info
  std::map<int16_t, StatisticsManager*> m_mFlowStatistics;
};

} // rto
} // rtp_plus_plus
