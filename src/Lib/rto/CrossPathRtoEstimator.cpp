#include "CorePch.h"
#include <rtp++/rto/CrossPathRtoEstimator.h>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/date_time.hpp>
#include <rtp++/rto/MovingAveragePredictor.h>
#include <rtp++/rto/PacketInterarrivalTimePredictor.h>
#include <rtp++/rto/PredictorTypes.h>

#define SLIDING_WINDOW_SIZE 50
#define DELTA_QUEUE_SIZE 16

namespace rtp_plus_plus
{
namespace rto
{

CrossPathRtoEstimator::CrossPathRtoEstimator(boost::asio::io_service& rIoService, const GenericParameters &applicationParameters)
  :m_rIoService(rIoService),
  m_referenceTimer(rIoService),
  m_eState(ST_LEARN_FLOW_ORDER),
  m_bShuttingdown(false),
  m_deltaQueue(DELTA_QUEUE_SIZE),
  m_uiWaitingForPacketFlow(0),
  m_uiCrossPathTimeout(0),
  m_uiReferenceSN(0),
  m_bRestartShortTimer(0),
  m_uiExpectingSN(0)
{
  PredictorType eType = getPredictorType(applicationParameters);
  boost::optional<double> prematureTimeoutProb = applicationParameters.getDoubleParameter(app::ApplicationParameters::pto);
  double dPrematureTimeoutProb = prematureTimeoutProb ? *prematureTimeoutProb : 0.05;

  switch (eType)
  {
    case PT_AR2:
    {
      VLOG(2) << "Using AR2 predictor for cross path prediction";
      m_pDeltaPredictor = std::unique_ptr<InterarrivalPredictor_t>(new PacketInterarrivalTimePredictor<uint64_t, int64_t>(dPrematureTimeoutProb));
      m_pCrosspathPredictor = std::unique_ptr<InterarrivalPredictor_t>(new PacketInterarrivalTimePredictor<uint64_t, int64_t>(dPrematureTimeoutProb));
      break;
    }
    case PT_MOVING_AVERAGE:
    default:
    {
      VLOG(2) << "Using moving average predictor for cross path prediction";
      boost::optional<uint32_t> historySize = applicationParameters.getUintParameter(app::ApplicationParameters::mavg_hist);
      const uint32_t uiHistorySize = historySize ? *historySize : 20;
      m_pDeltaPredictor = std::unique_ptr<InterarrivalPredictor_t>(new MovingAveragePredictor<uint64_t, int64_t>(uiHistorySize, 10, dPrematureTimeoutProb));
      m_pCrosspathPredictor = std::unique_ptr<InterarrivalPredictor_t>(new MovingAveragePredictor<uint64_t, int64_t>(uiHistorySize, 1, dPrematureTimeoutProb));
      break;
    }
  }
}

CrossPathRtoEstimator::~CrossPathRtoEstimator()
{
  m_statisticsManager.printStatistics();
  double dDeltaError = m_pDeltaPredictor->getErrorStandardDeviation();
  double dCrosspathError = m_pCrosspathPredictor->getErrorStandardDeviation();
  VLOG(2) << "Prediction errors: delta:" << dDeltaError << " Cross: " << dCrosspathError;
}

void CrossPathRtoEstimator::updateModel(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival)
{
  RtoData data(uiFlowId, uiSN, uiFSSN, tArrival);
  auto it = m_mFlowData.find(uiFlowId);
  if (it == m_mFlowData.end())
  {
    m_mFlowData[uiFlowId] = boost::circular_buffer<RtoData>(SLIDING_WINDOW_SIZE);
    m_mFlowData[uiFlowId].push_back(data);
    m_FlowIds.insert(uiFlowId);
  }
  else
  {
    // retrieve previously stored item to calculate delta
    RtoData& previousData = m_mFlowData[uiFlowId].back();
    uint32_t uiDifference = static_cast<uint32_t>((tArrival - previousData.Arrival).total_milliseconds());
#define LOG_MICRO_SECONDS
#ifdef LOG_MICRO_SECONDS
    uint32_t uiDifferenceUs = static_cast<uint32_t>((tArrival - previousData.Arrival).total_microseconds());
    uint32_t uiDeltaUs = static_cast<uint32_t>(((double)uiDifferenceUs/(uiSN - previousData.GSN) + 0.5));
    VLOG(2) << "Interarrival time " << uiDeltaUs << "us";
#endif
    uint32_t uiDelta = static_cast<uint32_t>(((double)uiDifference/(uiSN - previousData.GSN) + 0.5));
    VLOG(2) << "Difference: " << uiDifference << " Delta: " << uiDelta;
    m_pDeltaPredictor->insert(uiDeltaUs);
    m_deltaQueue.insert(uiDeltaUs);
    m_mFlowData[uiFlowId].push_back(data);
  }
}

#define FAIL 0
#define CONTINUE_WAITING 1
#define SUCCESS 2

int CrossPathRtoEstimator::determineCrossPathDifference(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival)
{
  // search For Missing Packet In Alternate Flow
  // look for packet from alternate flow
  if (uiFlowId == m_uiWaitingForPacketFlow)
  {
    boost::circular_buffer<RtoData>& dataQueue = m_mFlowData[uiFlowId];
    assert (!dataQueue.empty());
    // last elem
    auto it = dataQueue.rbegin();
    // last elem
    RtoData& last = *it;
    // m_reference = last;
    // first missing SN
    if (m_uiMissingSN == last.GSN)
    {
      // found the next packet on the slower flow
      // calculate difference
      // initialisation complete: calculate cross path timeout
      m_uiCrossPathTimeout = static_cast<uint32_t>((tArrival - m_reference.Arrival).total_milliseconds());
      uint32_t uiDifferenceUs = (tArrival - m_reference.Arrival).total_microseconds();
      VLOG(2) << "Crosspath Interarrival time " << uiDifferenceUs << "us";
      VLOG(2) << "Cross path timeout " <<  m_uiCrossPathTimeout << "ms";
      // m_deltaCrossPathQueue.insert(m_uiCrossPathTimeout);
      m_pCrosspathPredictor->insert(uiDifferenceUs);
      return SUCCESS;
    }
    else
    {
      // there might be packets with smaller that were sent earlier but if there is a packet with a bigger SN
      // we might have lost the packet
      if (uiSN > m_uiMissingSN)
      {
        VLOG(2) << "Lost packet " <<  m_uiMissingSN << ". Restarting path difference calculation";
        // go back to previous state
        return FAIL;
      }
    }
  }
  else
  {
    if (m_uiMissingSN == uiSN)
    {
      // the expected packet didn't arrive on the other flow: restart reference
      // if it's on the fastest flow reset the reference
      if (uiFlowId == m_vFlowOrder[0])
      {
        boost::circular_buffer<RtoData>& dataQueue = m_mFlowData[uiFlowId];
        assert(!dataQueue.empty());
        // last elem
        auto it = dataQueue.rbegin();
        // last elem
        RtoData& last = *it;
        m_reference = last;
        // first missing SN
        m_uiMissingSN = m_uiMissingSN + 1;
        m_uiWaitingForPacketFlow = m_vFlowOrder[1];
      }
      else
      {
        // go back to previous state
        return FAIL;
      }
    }
  }
  // m_uiWaitingForPacketFlow = m_vFlowOrder[1];

//    if (uiSN == m_uiMissingSN)
//    {
//      // initialisation complete: calculate cross path timeout
//      m_uiCrossPathTimeout = (tArrival - m_reference.Arrival).total_milliseconds();
//#ifdef LOG_MICRO_SECONDS
//      uint32_t uiDifferenceUs = (tArrival - m_reference.Arrival).total_microseconds();
//      VLOG(2) << "Crosspath Interarrival time " << uiDifferenceUs << "us";
//#endif
//      VLOG(2) << "Cross path timeout " <<  m_uiCrossPathTimeout << "ms";
//      // m_deltaCrossPathQueue.insert(m_uiCrossPathTimeout);
//      m_pCrosspathPredictor->insert(uiDifferenceUs);
//      return SUCCESS;
//    }
//    else
//    {
//      // there might be packets with smaller that were sent earlier but if there is a packet with a bigger SN
//      // we might have lost the packet
//      if (uiSN > m_uiMissingSN)
//      {
//        VLOG(2) << "Lost packet " <<  m_uiMissingSN << ". Restarting path difference calculation";
//        // go back to previous state
//        return FAIL;
//      }
//    }
//  }
  return CONTINUE_WAITING;
}

void CrossPathRtoEstimator::processNewPacket(const uint16_t uiFlowId, const uint16_t uiSN, const boost::posix_time::ptime &tArrival)
{    
  // I: if SN is in long timer list, cancel long timer
  // prediction has started: if packet arrives
  // if SN is in long timer list, cancel long timer
  // else
  // stop reference timer?
  {
    boost::mutex::scoped_lock l(m_lock);
    auto it = m_mLongTimers.find(uiSN);
    if (it != m_mLongTimers.end())
    {
      // try cancel: might not succeed if asio has already scheduled handler
      m_mLongTimers[uiSN]->cancel();
      // the short timer had already expired, and a timeout was restarted
      // for the next higher sequence number: now restart that timer resynched
      // to the arrival times
      if (uiSN == m_uiExpectingSN - 1)
      {
        m_bRestartShortTimer = true;
        m_referenceTimer.cancel();
      }
    }
    else
    {
      m_bRestartShortTimer = false;
      // check if this is the current packet expected: it might be an older packet from another path
      // in which case we do NOT want to restart the timer
      if (uiSN >= m_uiExpectingSN)
      {
        m_referenceTimer.cancel();
      }
    }
  }

  // TODO: debug, fix this: resulted in double detection
#ifdef FSSN_GAP_DETECTION
  // II: if there is a gap in flow specific SN: stop long timer(s)
  // check last two packets in flow and look for gap in GSN, no gap in FSSN
  boost::circular_buffer<RtoData>& dataQueue = m_mFlowData[uiFlowId];
  if (dataQueue.size() > 1)
  {
    // last elem
    auto it = dataQueue.rbegin();
    // last elem
    RtoData& last = *it;
    auto it2 = it + 1;
    RtoData& secondLast = *it2;

    // HACK TO PREVENT FSSN DETECTION on slow flow to prevent double picking
    if (uiFlowId == m_vFlowOrder[0])
    {
      if (last.FSSN != secondLast.FSSN + 1)
      {
        for (uint16_t i = secondLast.FSSN + 1; i < last.FSSN; ++i)
        {
          VLOG(2) << LOG_MODIFY_WITH_CARE
                  << " On Packet Arrival Lost FlowId: " << uiFlowId << " FSSN: " << i << " Second last: " << secondLast.FSSN << " last: " << last.FSSN;
          m_mFlowStatistics[uiFlowId].assumePacketLost(tArrival, i);
          m_onLost(-1, uiFlowId, i);
        }
      }
    }
  }
#endif
}

void CrossPathRtoEstimator::checkState(const uint16_t uiFlowId, const uint16_t uiSN, const uint16_t uiFSSN, const boost::posix_time::ptime& tArrival)
{
  switch (m_eState)
  {
  case ST_LEARN_FLOW_ORDER:
  {
    if (learnFlowOrder())
      m_eState = ST_LEARN_PATH_DIFFERENCE;
    break;
  }
  case ST_LEARN_PATH_DIFFERENCE:
  {
    if (searchForReferencePacketInFastestFlow(uiFlowId))
    {
      m_eState = ST_LEARN_AWAITING_PACKET_FROM_ALTERNATE_FLOW;
      //VLOG(2) << "Initialisation complete: cross path timeout " <<  m_uiCrossPathTimeout << "ms";
      //LOG_FIRST_N(INFO, 1) << "RTO Algorithm initalised";
      // m_eState = ST_EST_AWAITING_PACKET_FROM_FASTEST_FLOW;
    }
    break;
  }
  case ST_LEARN_AWAITING_PACKET_FROM_ALTERNATE_FLOW:
  {
    int res = determineCrossPathDifference(uiFlowId, uiSN, uiFSSN, tArrival);
    if (res == SUCCESS)
    {
      VLOG(2) << "Initialisation complete: cross path timeout " <<  m_uiCrossPathTimeout << "ms";
      LOG_FIRST_N(INFO, 1) << "RTO Algorithm initalised";
      m_eState = ST_EST_AWAITING_PACKET_FROM_FASTEST_FLOW;
    }
    else if (res == FAIL)
    {
      // go back to previous state
      m_eState = ST_LEARN_PATH_DIFFERENCE;
    }
    break;
  }
  case ST_EST_AWAITING_PACKET_FROM_FASTEST_FLOW:
  {
    if (uiFlowId == m_vFlowOrder[0])
    {
      // start first timeout
      m_uiReferenceSN = uiSN;
      uint32_t uiNextSN = uiSN + 1;
      m_uiExpectingSN = uiNextSN;
      // make sure predictors are ready
      if (m_pDeltaPredictor->isReady() && m_pCrosspathPredictor->isReady())
      {
        // start smallest timer
        uint32_t uiDeltaUs = m_pDeltaPredictor->getLastPrediction();
        // TEST: RG See if this avoids false positives in RR scheduling along paths with similar delays
//        uint32_t uiDeltaUs = m_pDeltaPredictor->getLastPrediction() + 1000;
        m_referenceTimer.expires_from_now(boost::posix_time::microseconds(uiDeltaUs));

        // add delta here to prevent fast timer from being too inaccurate


        // Purely for logging
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        // TEST: RG See if this avoids false positives in RR scheduling along paths with similar delays
        // uint32_t uiCrossPredictorUs = m_pCrosspathPredictor->getLastPrediction();
        uint32_t uiCrossPredictorUs = m_pCrosspathPredictor->getLastPrediction() + 1000;
        boost::posix_time::ptime tLongTimeout = tNow + boost::posix_time::microseconds(uiCrossPredictorUs);

        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Now: " << tNow
                << " packet estimation SN: " << uiNextSN
                << " short: " << m_referenceTimer.expires_at()
                << " long: " << tLongTimeout
                << " delta: " << uiDeltaUs
                   ;

        m_referenceTimer.async_wait(boost::bind(&CrossPathRtoEstimator::onShortTimeout, this, _1, uiNextSN, uiDeltaUs) );
        m_eState = ST_EST_RUNNING_A;
      }
      else
      {
        VLOG(2) << "Waiting to start prediction, predictors not ready. Delta: " << m_pDeltaPredictor->isReady()
                   << " Cross: " << m_pCrosspathPredictor->isReady();
      }

    }
    else
    {
      // do nothing, wait for start
    }
    break;
  }
  case ST_EST_RUNNING_A:
  {
    // update cross path model if possible
    if (searchForReferencePacketInFastestFlow(uiFlowId))
    {
      m_eState = ST_EST_RUNNING_B;
    }

    processNewPacket(uiFlowId, uiSN, tArrival);
    break;
  }
  case ST_EST_RUNNING_B:
  {
    int res = determineCrossPathDifference(uiFlowId, uiSN, uiFSSN, tArrival);
    // The result does not matter: we are simply trying to update our cross path difference model
    if (res == FAIL || res == SUCCESS)
    {
      // go back to previous state
      m_eState = ST_EST_RUNNING_A;
    }

    processNewPacket(uiFlowId, uiSN, tArrival);
    break;
  }
  }
}

#define MIN_DELTA_ESTIMATES 5
bool CrossPathRtoEstimator::learnFlowOrder()
{
  if (m_mFlowData.size() > 1)
  {
    if (m_deltaQueue.size() < MIN_DELTA_ESTIMATES)
      return false;

    // estimate smallest flow by delta approximation
    bool bFirst = false;
    uint16_t uiGSNReference = 0;
    std::map<uint16_t, boost::posix_time::ptime> mEstimates;
    uint32_t uiDelta = m_deltaQueue.getAverage();
    for (auto& pair: m_mFlowData)
    {
      RtoData& data = pair.second.back();
      if (!bFirst)
      {
        bFirst = true;
        uiGSNReference = data.GSN;
        mEstimates[pair.first] = data.Arrival;
        VLOG(2) << "Ref: " << pair.first << " GSN: " << uiGSNReference << " T: " << data.Arrival;
      }
      else
      {
        if (data.GSN > uiGSNReference)
        {
          uint16_t iDiff = data.GSN - uiGSNReference;
          mEstimates[pair.first] = data.Arrival - boost::posix_time::microseconds(iDiff * uiDelta);
          VLOG(2) << "Next: " << pair.first << " GSN: " << data.GSN << " Delta: " << uiDelta << " Diff: " << iDiff << " T: " << data.Arrival << " Offset: " << mEstimates[pair.first];
        }
        else
        {
          uint16_t iDiff = uiGSNReference - data.GSN;
          mEstimates[pair.first] = data.Arrival + boost::posix_time::microseconds(iDiff * uiDelta);
          VLOG(2) << "Next: " << pair.first << " GSN: " << data.GSN << " Delta: " << uiDelta << " Diff: " << iDiff << " T: " << data.Arrival << " Offset: " << mEstimates[pair.first];
        }
      }
    }

    uint32_t uiReps = mEstimates.size();
    for (size_t i = 0; i < uiReps; ++i)
    {
      typedef std::pair<const uint16_t, boost::posix_time::ptime> Pair_t;
      auto it = std::min_element(mEstimates.begin(), mEstimates.end(), [](Pair_t& pair1, Pair_t& pair2)
      {
        return pair1.second < pair2.second;
      });
      assert (it != mEstimates.end());
      VLOG(2) << "Fastest Flow: " << it->first;
      m_vFlowOrder.push_back(it->first);
      mEstimates.erase(it);
   }
   return true;
  }
  return false;
}

bool CrossPathRtoEstimator::searchForReferencePacketInFastestFlow(const uint16_t uiFlowId)
{
  if (uiFlowId == m_vFlowOrder[0])
  {
    // check last two packets in flow and look for gap in GSN, no gap in FSSN
    boost::circular_buffer<RtoData>& dataQueue = m_mFlowData[uiFlowId];
    if (dataQueue.size() >= 1)
    {
      // last elem
      auto it = dataQueue.rbegin();
      // last elem
      RtoData& last = *it;
      m_reference = last;
      // first missing SN
      m_uiMissingSN = last.GSN + 1;
      m_uiWaitingForPacketFlow = m_vFlowOrder[1];
      return true;

//      auto it2 = it + 1;
//      RtoData& secondLast = *it2;
//      if (last.FSSN = secondLast.FSSN + 1)
//      {
//        // check for gap in GSN
//        if (last.GSN != secondLast.GSN + 1)
//        {
//          // store reference
//          m_reference = secondLast;
//          // first missing SN
//          m_uiMissingSN = secondLast.GSN + 1;
//          m_uiWaitingForPacketFlow = m_vFlowOrder[1];
//          return true;
//        }
//      }
    }
  }
//  else if (uiFlowId == m_uiWaitingForPacketFlow)
//  {
//    boost::circular_buffer<RtoData>& dataQueue = m_mFlowData[uiFlowId];
//    if (dataQueue.size() >= 1)
//    {
//      // last elem
//      auto it = dataQueue.rbegin();
//      // last elem
//      RtoData& last = *it;
//      // m_reference = last;
//      // first missing SN
//      if (m_uiMissingSN == last.GSN)
//      {
//        // found the next packet on the slower flow
//        // calculate difference
//        // initialisation complete: calculate cross path timeout
//        m_uiCrossPathTimeout = (last.Arrival - m_reference.Arrival).total_milliseconds();
//        uint32_t uiDifferenceUs = (last.Arrival - m_reference.Arrival).total_microseconds();
//        VLOG(2) << "Crosspath Interarrival time " << uiDifferenceUs << "us";
//        VLOG(2) << "Cross path timeout " <<  m_uiCrossPathTimeout << "ms";
//        // m_deltaCrossPathQueue.insert(m_uiCrossPathTimeout);
//        m_pCrosspathPredictor->insert(uiDifferenceUs);
//        return true;
//      }
//      // m_uiWaitingForPacketFlow = m_vFlowOrder[1];
//    }
//  }
  return false;
}

void CrossPathRtoEstimator::doOnPacketArrival(  const boost::posix_time::ptime& tArrival, const uint16_t uiSN, const uint16_t uiFlowId, const uint16_t uiFSSN)
{
//  m_statisticsManager.receivePacket(tArrival, uiSN);

  // update delta model: determine delta_transmission
  updateModel(uiFlowId, uiSN, uiFSSN, tArrival);

  checkState(uiFlowId, uiSN, uiFSSN, tArrival);
}

void CrossPathRtoEstimator::onLongTimeout(const boost::system::error_code& ec, uint16_t uiSN, uint32_t uiDelta)
{
  if (!m_bShuttingdown)
  {
    if (!ec)
    {
      if (!m_statisticsManager.hasPacketBeenReceivedRecently(uiSN))
      {
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        m_statisticsManager.assumePacketLost(tNow, uiSN);
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Packet Long Timeout Lost SN: " << uiSN << " Delta: " << uiDelta;
        m_onLost(uiSN, -1, -1);
      }
      else
      {
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Packet Long Timeout Received SN: " << uiSN << " in the meanwhile";
      }
    }
    else
    {
      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " Packet Long Timeout Received SN: " << uiSN << " arrived in the meanwhile Delta: "<< uiDelta;
    }
  }

  boost::mutex::scoped_lock l(m_lock);
  boost::asio::deadline_timer* pTimer = m_mLongTimers[uiSN];
  delete pTimer;
  m_mLongTimers.erase(uiSN);
}

void CrossPathRtoEstimator::onShortTimeout(const boost::system::error_code& ec, uint16_t uiSN, uint32_t uiDelta)
{
  if (!m_bShuttingdown)
  {
    uint32_t uiNextSN = 0;
    if (!ec)
    {
      // check if packet has been received
      if (!m_statisticsManager.hasPacketBeenReceivedRecently(uiSN))
      {
        uint32_t uiNewDelta = m_pCrosspathPredictor->getLastPrediction() - uiDelta;
        VLOG(2) << "CP: Last predict: " << m_pCrosspathPredictor->getLastPrediction()
                << " Delta: " << uiDelta;
        boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
        pTimer->expires_from_now(boost::posix_time::microseconds(uiNewDelta));
        pTimer->async_wait(boost::bind(&CrossPathRtoEstimator::onLongTimeout, this, _1, uiSN, uiNewDelta));

        {
          boost::mutex::scoped_lock l(m_lock);
          m_mLongTimers[uiSN] = pTimer;
        }

        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Packet Short Timeout SN: " << uiSN;

        if (!m_bRestartShortTimer)
        {
          uiNextSN = uiSN + 1;
        }
        else
        {
          uiNextSN = uiSN; // resync
          VLOG(2) << LOG_MODIFY_WITH_CARE
                  << " Packet resync SN: " << uiNextSN;
          m_bRestartShortTimer = false;
        }
      }
      else
      {
        VLOG(2) << "Short time out: " << uiDelta << "ms packet " << uiSN << " received";
        uiNextSN = uiSN + 1;
      }
    }
    else
    {
      // check flag to see if packet was on time i.e. packet arrived before short timeout
      // or if the timeout was cancelled for resync purposes
      if (!m_bRestartShortTimer)
      {
        // Still log for graphing of timeout estimation
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Packet Short Timeout SN: " << uiSN;
        VLOG(2) << "Cancelled: Short time out: " << uiDelta << "ms packet " << uiSN << " received";
        uiNextSN = uiSN + 1;
      }
      else
      {
        // resync means that short timer is restarted: no special logging here
        VLOG(2) << "Cancelled: Short time out: " << uiDelta << "ms packet " << uiSN << " resync";
        uiNextSN = uiSN; // resync
        m_bRestartShortTimer = false;
      }
    }

    // Always start next short timer
    // cancelled: packet received: schedule next sample
    // start estimation of next highest SN
    m_uiExpectingSN = uiNextSN;
    // uint32_t uiNewDelta = m_deltaQueue.getAverage();
    uint32_t uiNewDelta = m_pDeltaPredictor->getLastPrediction();

    m_referenceTimer.expires_from_now(boost::posix_time::microseconds(uiNewDelta));

    // Purely for logging
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    uint32_t uiCrossPredictorUs = m_pCrosspathPredictor->getLastPrediction();
    boost::posix_time::ptime tLongTimeout = tNow + boost::posix_time::microseconds(uiCrossPredictorUs);

    VLOG(2) << LOG_MODIFY_WITH_CARE
            << " CP packet estimation"
            << " now: " << tNow
            << " SN: " << uiNextSN
            << " short: " << m_referenceTimer.expires_at()
            << " long: " << tLongTimeout
            << " delta: " << uiNewDelta
            << " delta_c: " << uiCrossPredictorUs
               ;

    m_referenceTimer.async_wait(boost::bind(&CrossPathRtoEstimator::onShortTimeout, this, _1, uiNextSN, uiNewDelta) );

  }
}

void CrossPathRtoEstimator::doStop()
{
  m_bShuttingdown = true;
  m_referenceTimer.cancel();
  for (auto& pair: m_mLongTimers)
  {
      pair.second->cancel();
  }
}

void CrossPathRtoEstimator::onRtpPacketAssumedLostInFlow(uint16_t uiFSSN, uint16_t uiFlowId)
{

}

} // rto
} // rtp_plus_plus
