#include "CorePch.h"
#include <rtp++/mprtp/MpRtpFeedbackManager.h>
#include <functional>
#include <cpputil/Conversion.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtcpReportManager.h>
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/rto/CrossPathRtoEstimator.h>
#include <rtp++/rto/MultipathRtoEstimator.h>
#include <rtp++/rto/PredictorTypes.h>

#define COMPONENT_LOG_LEVEL 5

namespace rtp_plus_plus
{
namespace mprtp
{

FeedbackManager::FeedbackManager(const RtpSessionParameters& rtpParameters,
                                 const GenericParameters& applicationParameters,
                                 boost::asio::io_service& rIoService)
  :m_eFeedbackMode(FB_NONE)
{
  VLOG(2) << "mprtp::FeedbackManager: NACK support: " << rtpParameters.supportsFeedbackMessage(rfc4585::NACK)
          << " ACK support: " << rtpParameters.supportsFeedbackMessage(rfc4585::ACK)
          << " rtx enabled: " << rtpParameters.isRetransmissionEnabled();
  if (rtpParameters.supportsFeedbackMessage(rfc4585::NACK) && rtpParameters.isRetransmissionEnabled())
  {
    VLOG(2) << "NACK feedback mode active";
    m_eFeedbackMode = FB_NACK;
    // configure estimator
    rto::MpPredictorType eMpType = rto::getMpPredictorType(applicationParameters);

    switch (eMpType)
    {
      case rto::PT_MP_CROSSPATH:
      {
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << "Creating crosspath RTO estimator";
        m_pMpRtoEstimator = std::unique_ptr<rto::MultipathPacketLossDetectionBase>(new rto::CrossPathRtoEstimator(rIoService, applicationParameters));
        break;
      }
      case rto::PT_MP_COMPARE:
      {
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << "Creating basic and crosspath RTO estimator for comparison";
        m_pMpRtoEstimator = std::unique_ptr<rto::MultipathPacketLossDetectionBase>(new rto::CrossPathRtoEstimator(rIoService, applicationParameters));
        m_pMpRtoEstimator2 = std::unique_ptr<rto::MultipathPacketLossDetectionBase>(new rto::MultipathRtoEstimator(rIoService, applicationParameters));
        break;
      }
      default:
      {
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << "Creating multiple single path RTO estimators";
        m_pMpRtoEstimator = std::unique_ptr<rto::MultipathPacketLossDetectionBase>(new rto::MultipathRtoEstimator(rIoService, applicationParameters));
        break;
      }
    }

    m_pMpRtoEstimator->setLostCallback(
                std::bind(&mprtp::FeedbackManager::onRtpPacketAssumedLost, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_pMpRtoEstimator->setFalsePositiveCallback(
                std::bind(&mprtp::FeedbackManager::onRtpPacketLate, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    if (m_pMpRtoEstimator2)
    {
      m_pMpRtoEstimator2->setLostCallback(
                  std::bind(&mprtp::FeedbackManager::onRtpPacketAssumedLost, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
      m_pMpRtoEstimator2->setFalsePositiveCallback(
                  std::bind(&mprtp::FeedbackManager::onRtpPacketLate, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    }
  }
  else if (rtpParameters.supportsFeedbackMessage(rfc4585::ACK) && rtpParameters.isRetransmissionEnabled())
  {
    VLOG(2) << "ACK feedback mode active";
    m_eFeedbackMode = FB_ACK;
  }
  else
  {
    VLOG(2) << "No feedback mode active";
  }
}

void FeedbackManager::reset()
{
  // TODO
#if 0
  if (m_pMpRtoEstimator)
  {
    DLOG(INFO) << "[" << this << "] Shutting down RTO estimator";
    m_pMpRtoEstimator->reset();
    if (m_pMpRtoEstimator2)
    {
      DLOG(INFO) << "[" << this << "] Shutting down RTO 2nd estimator";
      m_pMpRtoEstimator2->reset();
    }
  }
#endif
}

void FeedbackManager::shutdown()
{
  if (m_pMpRtoEstimator)
  {
    VLOG(5) << "[" << this << "] Shutting down RTO estimator";
    m_pMpRtoEstimator->stop();
    if (m_pMpRtoEstimator2)
    {
      VLOG(5) << "[" << this << "] Shutting down RTO 2nd estimator";
      m_pMpRtoEstimator2->stop();
    }
  }
}

void FeedbackManager::onPacketArrival(const boost::posix_time::ptime &tArrival, const uint32_t uiSN)
{
  LOG_FIRST_N(WARNING, 1) << "Single path method not applicable in multipath feedback manager";
}

void FeedbackManager::onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN)
{
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      if (m_pMpRtoEstimator)
      {
        VLOG(2) << LOG_MODIFY_WITH_CARE
                << " Packet arrived SN: " << uiSN
                << " Flow Id: " << uiFlowId
                << " FSSN: " << uiFSSN
                << " Arrival: " << tArrival;

        m_pMpRtoEstimator->onPacketArrival(tArrival, uiSN, uiFlowId, uiFSSN);
        if (m_pMpRtoEstimator2)
        {
          m_pMpRtoEstimator2->onPacketArrival(tArrival, uiSN, uiFlowId, uiFSSN);
        }
      }
    }
    case FB_ACK:
    {
      m_vReceivedSN.push_back(uiSN);
      break;
    }
  }
}

void FeedbackManager::onRtxPacketArrival(const boost::posix_time::ptime &tArrival, const uint16_t uiSN)
{
  LOG_FIRST_N(WARNING, 1) << "Single path method not applicable in multipath feedback manager";
}

void FeedbackManager::onRtxPacketArrival(const boost::posix_time::ptime& tArrival,
                                         const uint16_t uiOriginalSN, uint16_t uiFlowId, uint16_t uiFSSN)
{
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      // The RTO estimator has to be valid if we received an RTX
      // TODO:
      bool bLate = false, bDuplicate = false;
      assert(m_pMpRtoEstimator);
      m_pMpRtoEstimator->onRtxPacketArrival(tArrival,
                                            uiOriginalSN, uiFlowId, uiFSSN,
                                            bLate, bDuplicate);

      if (m_pMpRtoEstimator2)
        m_pMpRtoEstimator2->onRtxPacketArrival(tArrival,
                                               uiOriginalSN, uiFlowId, uiFSSN,
                                               bLate, bDuplicate);

#if 0
      // lookup delay
      auto it = m_mRtxDurationMap.find(uiOriginalSN);
      // assert (it != m_mRtxDurationMap.end());

      if (it != m_mRtxDurationMap.end())
      {
        if (bLate)
        {
          VLOG(2) << "Retransmission late of " << uiOriginalSN << " - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
        else if (bDuplicate)
        {
          VLOG(2) << "Retransmission duplicate of " << uiOriginalSN << " - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
        else
        {
          VLOG(2) << "Retransmission of " << uiOriginalSN << " successful - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
      }
      else
      {
        uint32_t uiFlowIdFssn = (uiFlowId << 16) | uiFSSN;
        VLOG(2) << "RTX Packet arrival:" << uiOriginalSN << " Flow: " << uiFlowId << " FSSN: " << uiFSSN << " Flow-FSSN: " << uiFlowIdFssn;
        // search in flow specific sequence number map
        auto it = m_mFlowSpecificRtxDurationMap.find(uiFlowIdFssn);
        assert (it != m_mFlowSpecificRtxDurationMap.end());
        if (bLate)
        {
          VLOG(2) << "Retransmission late of " << uiOriginalSN << " - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
        else if (bDuplicate)
        {
          VLOG(2) << "Retransmission duplicate of " << uiOriginalSN << " - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
        else
        {
          VLOG(2) << "Retransmission of " << uiOriginalSN << " successful - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
      }
#endif
    }
    case FB_ACK:
    {
      m_vReceivedSN.push_back(uiOriginalSN);
      break;
    }
  }
}

#if 0
void FeedbackManager::retrieveFeedbackReports(CompoundRtcpPacket& feedback)
{
  LOG_FIRST_N(WARNING, 1) << "Single path method not applicable in multipath feedback manager";
  // return CompoundRtcpPacket();
}

void FeedbackManager::retrieveFeedbackReports(uint16_t uiFlowId, CompoundRtcpPacket& feedback)
{
  VLOG(20) << "######## Retrieving feedback for report######### " << uiFlowId;
  // TODO: might want to get all feedback reports e.g. when there's a NACK for
  // the other channel waiting to be sent?
  // or only send on the fastest channel?
  // CompoundRtcpPacket vFeedback;
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      // process NACKs
      addGenericNacks(feedback);
    }
    case FB_ACK:
    {
      // process ACKs
      addGenericAcks(feedback);
      break;
    }
  }
  // return vFeedback;
}
#endif

void FeedbackManager::onRtpPacketAssumedLost(int uiSN, int uiFlowId, int uiFSSN)
{
  VLOG(1) << "Packet SN: " << uiSN << " Flow: " << uiFlowId << " FSSN: " << uiFSSN << " assumed lost";
  if (uiSN != rto::SN_NOT_SET)
  {
    m_vAssumedLost.push_back(uiSN);
    VLOG(2) << "Assumed lost (" << m_vAssumedLost.size() << ") " << ::toString(m_vAssumedLost);
    if (m_onLoss) m_onLoss(uiSN);
  }
  else
  {
    // here need to store packet so that when retrieve feedback reports is called we can add
    // them to the compound RTCP packet
    m_vFlowSpecificLosses.push_back(std::make_tuple(uiFlowId, uiFSSN));
    if (m_onFlowSpecificLoss) m_onFlowSpecificLoss(uiFlowId, uiFSSN);
  }
}

void FeedbackManager::onRtpPacketLate(int uiSN, int uiFlowId, int uiFSSN)
{
  VLOG(1) << "Packet SN: " << uiSN << " FlowId: " << uiFlowId << " FSSN: " << uiFSSN << " late";
  // try remove packet if it is not too late already
  if (uiSN != rto::SN_NOT_SET)
  {
    for (auto it = m_vAssumedLost.begin(); it != m_vAssumedLost.end(); ++it)
    {
      if (uiSN == *it)
      {
        VLOG(2) << "Removed late packet from feedback request: " << uiSN;
        m_vAssumedLost.erase(it);
        return;
      }
    }
    VLOG(2) << "Too late to remove late packet from feedback request: " << uiSN;
  }
  else
  {
    auto it = std::find_if(m_vFlowSpecificLosses.begin(),
                           m_vFlowSpecificLosses.end(),
                           [uiFlowId, uiFSSN](std::tuple<uint16_t, uint16_t>& item)
    {
      return std::get<0>(item) == uiFlowId && std::get<1>(item) == uiFSSN;
    });
    if ( it != m_vFlowSpecificLosses.end())
    {
      VLOG(2) << "Removed late packet from feedback request FlowId: " << uiFlowId
              << " FSSN: " << uiFSSN;
      m_vFlowSpecificLosses.erase(it);
      return;
    }
    VLOG(2) << "Too late to remove late packet from feedback request: FlowId: " << uiFlowId
            << " FSSN: " << uiFSSN;
  }
}

void FeedbackManager::onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiSN)
{
  // update estimator with feedback time
  m_pMpRtoEstimator->onRtxRequested(tRequested, uiSN, rto::SN_NOT_SET, rto::SN_NOT_SET);
  if (m_pMpRtoEstimator2)
    m_pMpRtoEstimator2->onRtxRequested(tRequested, uiSN, rto::SN_NOT_SET, rto::SN_NOT_SET);
}

void FeedbackManager::onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiFlowId, const uint16_t uiSN)
{
  // update estimator with feedback time
  m_pMpRtoEstimator->onRtxRequested(tRequested, rto::SN_NOT_SET, uiFlowId, uiSN);
  if (m_pMpRtoEstimator2)
    m_pMpRtoEstimator2->onRtxRequested(tRequested, rto::SN_NOT_SET, uiFlowId, uiSN);
}

#if 0
// TODO: rename generically addNacks
void FeedbackManager::addGenericNacks(CompoundRtcpPacket& vFeedback)
{
  // NB: only create a NACK if there are lost packets!
  if (!m_vAssumedLost.empty())
  {
    // for now there is just one generic NACK
    rfc4585::RtcpGenericNack::ptr pNack = rfc4585::RtcpGenericNack::create(m_vAssumedLost);
    // get own SSRC
    pNack->setSenderSSRC(m_uiSourceSSRC);
    // get the source SSRC
    pNack->setSourceSSRC(m_uiSSRC);
    vFeedback.push_back(pNack);
    // The code as it is can only handle one source SSRC?!?
    // We would have to create one RTO per source to be able
    // to handle multiple sources?

    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();

    VLOG(2) << "Sending RTX request for " << ::toString(m_vAssumedLost);
    for (uint16_t uiSN : m_vAssumedLost)
    {
      m_mRtxDurationMap.insert(std::make_pair(uiSN, tNow));
      // update estimator with feedback time
      m_pMpRtoEstimator->onRtxRequested(tNow, uiSN, rto::SN_NOT_SET, rto::SN_NOT_SET);
      if (m_pMpRtoEstimator2)
        m_pMpRtoEstimator2->onRtxRequested(tNow, uiSN, rto::SN_NOT_SET, rto::SN_NOT_SET);
    }
    m_vAssumedLost.clear();
  }
  else
  {
    VLOG(20) << "m_vAssumedLost empty";
  }

  // add FSSN NACKS, not just specific to this flow!
  if (!m_vFlowSpecificLosses.empty())
  {
    std::map<uint16_t, std::vector<uint16_t> > mLossMap;
    // for now there is just one generic NACK
    for (size_t i = 0; i < m_vFlowSpecificLosses.size(); ++i)
    {
      mLossMap[std::get<0>(m_vFlowSpecificLosses[i])].push_back(std::get<1>(m_vFlowSpecificLosses[i]));
    }
    VLOG(2) << "Found " << mLossMap.size() << " flows with losses";
    for (auto& it : mLossMap)
    {
      VLOG(2) << "Adding extended NACK for flow " << it.first << " with " << it.second.size()
              << " items: " << ::toString(it.second);
      rfc4585::RtcpExtendedNack::ptr pNack = rfc4585::RtcpExtendedNack::create(it.first, it.second);
      // get own SSRC
      pNack->setSenderSSRC(m_uiSourceSSRC);
      // get the source SSRC
      pNack->setSourceSSRC(m_uiSSRC);
      vFeedback.push_back(pNack);
    }
    // The code as it is can only handle one source SSRC?!?
    // We would have to create one RTO per source to be able
    // to handle multiple sources?

    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#if 1
    for (auto& loss : m_vFlowSpecificLosses)
    {
      uint32_t uiFlowFssn = (std::get<0>(loss) << 16) | std::get<1>(loss);
      VLOG(2) << "Flow specific loss: Flow: " << std::get<0>(loss) << " FSSN: " << std::get<1>(loss) << " Flow-FSSN: " << uiFlowFssn;
      m_mFlowSpecificRtxDurationMap.insert(std::make_pair(uiFlowFssn, tNow));
      // update estimator with feedback time
      m_pMpRtoEstimator->onRtxRequested(tNow, rto::SN_NOT_SET, std::get<0>(loss), std::get<1>(loss));
      if (m_pMpRtoEstimator2)
        m_pMpRtoEstimator2->onRtxRequested(tNow, rto::SN_NOT_SET, std::get<0>(loss), std::get<1>(loss));
    }
#endif
    m_vFlowSpecificLosses.clear();
  }
}

void FeedbackManager::addGenericAcks(CompoundRtcpPacket &vFeedback)
{
  // NB: only create a NACK if there are lost packets!
  if (!m_vReceivedSN.empty())
  {
    // for now there is just one generic ACK
    rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(m_vReceivedSN);
    // get own SSRC
    pAck->setSenderSSRC(m_uiSourceSSRC);
    // get the source SSRC
    pAck->setSourceSSRC(m_uiSSRC);
    vFeedback.push_back(pAck);
    // The code as it is can only handle one source SSRC?!?
    // We would have to create one RTO per source to be able
    // to handle multiple sources?
    VLOG(COMPONENT_LOG_LEVEL) << "Sending ACKs request for " << ::toString(m_vReceivedSN);
    m_vReceivedSN.clear();
  }
}
#endif

uint32_t FeedbackManager::getAssumedLostCount() const
{
  return m_vAssumedLost.size();
}

uint32_t FeedbackManager::getReceivedCount() const
{
  return m_vReceivedSN.size();
}

std::vector<uint32_t> FeedbackManager::getAssumedLost()
{
  std::vector<uint32_t> vAssumedLost;
  std::swap(vAssumedLost, m_vAssumedLost);
  return vAssumedLost;
}

std::vector<uint32_t> FeedbackManager::getReceived()
{
  std::vector<uint32_t> vReceivedSN;
  std::swap(vReceivedSN, m_vReceivedSN);
  return vReceivedSN;
}

} // mprtp
} // rtp_plus_plus
