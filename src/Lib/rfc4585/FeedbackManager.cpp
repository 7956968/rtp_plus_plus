#include "CorePch.h"
#include <rtp++/rfc4585/FeedbackManager.h>
#include <cpputil/Conversion.h>
#include <cpputil/GenericParameters.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/rto/AR2Predictor.h>
#include <rtp++/rto/BasicRtoEstimator.h>
#include <rtp++/rto/MovingAveragePredictor.h>
#include <rtp++/rto/PacketInterarrivalTimePredictor.h>
#include <rtp++/rto/PredictorTypes.h>
#include <rtp++/rto/SimpleRtoEstimator.h>
#include <rtp++/application/ApplicationParameters.h>

#define COMPONENT_LOG_LEVEL 5

namespace rtp_plus_plus
{
namespace rfc4585
{

FeedbackManager::FeedbackManager(const RtpSessionParameters& rtpParameters,
                                 const GenericParameters &applicationParameters,
                                 boost::asio::io_service &rIoService)
  :m_eFeedbackMode(FB_NONE)
{
  VLOG(2) << "rfc4585::FeedbackManager: supports NACK: " << rtpParameters.supportsFeedbackMessage(NACK)
          << " ACK support: " << rtpParameters.supportsFeedbackMessage(rfc4585::ACK)
          << " rtx enabled: " << rtpParameters.isRetransmissionEnabled();
  if (rtpParameters.supportsFeedbackMessage(NACK) && rtpParameters.isRetransmissionEnabled())
  {
    VLOG(2) << "NACK feedback mode active";
    m_eFeedbackMode = FB_NACK;
    // configure estimator
    rto::PredictorType eType = rto::getPredictorType(applicationParameters);
    boost::optional<double> prematureTimeoutProb = applicationParameters.getDoubleParameter(app::ApplicationParameters::pto);
    double dPrematureTimeoutProb = prematureTimeoutProb ? *prematureTimeoutProb : 0.05;

    switch (eType)
    {
      case rto::PT_AR2:
      {
        VLOG(2) << "Using AR2 predictor. Premature timout prob: " << dPrematureTimeoutProb;
        rto::BasicRtoEstimator* pEstimator = new rto::BasicRtoEstimator(rIoService);
        pEstimator->setPredictor(std::unique_ptr<rto::InterarrivalPredictor_t>(new rto::PacketInterarrivalTimePredictor<uint64_t, int64_t>(dPrematureTimeoutProb)));

        m_pRtoEstimator = std::unique_ptr<rto::PacketLossDetectionBase>(pEstimator);
        m_pRtoEstimator->setLostCallback(std::bind(&FeedbackManager::onRtpPacketAssumedLost, this, std::placeholders::_1));
        m_pRtoEstimator->setFalsePositiveCallback(std::bind(&FeedbackManager::onRtpPacketLate, this, std::placeholders::_1));
        break;
      }
      case rto::PT_MOVING_AVERAGE:
      {
        boost::optional<uint32_t> historySize = applicationParameters.getUintParameter(app::ApplicationParameters::mavg_hist);
        const uint32_t uiHistorySize = historySize ? *historySize : 20;
        VLOG(2) << "Using Moving average predictor with history size " << uiHistorySize;

        rto::BasicRtoEstimator* pEstimator = new rto::BasicRtoEstimator(rIoService);
        pEstimator->setPredictor(std::unique_ptr<rto::InterarrivalPredictor_t>(new rto::MovingAveragePredictor<uint64_t, int64_t>(uiHistorySize, dPrematureTimeoutProb)));

        m_pRtoEstimator = std::unique_ptr<rto::PacketLossDetectionBase>(pEstimator);
        m_pRtoEstimator->setLostCallback(std::bind(&FeedbackManager::onRtpPacketAssumedLost, this, std::placeholders::_1));
        m_pRtoEstimator->setFalsePositiveCallback(std::bind(&FeedbackManager::onRtpPacketLate, this, std::placeholders::_1));
        break;
      }
      case rto::PT_SIMPLE:
      {
        VLOG(2) << "Using simple predictor";
        m_pRtoEstimator = std::unique_ptr<rto::PacketLossDetectionBase>(new rto::SimplePacketLossDetection());
        m_pRtoEstimator->setLostCallback(std::bind(&FeedbackManager::onRtpPacketAssumedLost, this, std::placeholders::_1));
        m_pRtoEstimator->setFalsePositiveCallback(std::bind(&FeedbackManager::onRtpPacketLate, this, std::placeholders::_1));
        break;
      }
      default:
      {
        VLOG(2) << "No predictor configured";
        break;
      }
    }
  }
  else if (rtpParameters.supportsFeedbackMessage(ACK) && rtpParameters.isRetransmissionEnabled())
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
  if (m_pRtoEstimator)
  {
    DLOG(INFO) << "[" << this << "] Resetting RTO estimator";
    m_pRtoEstimator->reset();
  }
}

void FeedbackManager::shutdown()
{
  if (m_pRtoEstimator)
  {
    DLOG(INFO) << "[" << this << "] Shutting down RTO estimator";
    m_pRtoEstimator->stop();
  }
}

void FeedbackManager::onPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN)
{
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      if (m_pRtoEstimator)
      {
        m_pRtoEstimator->onPacketArrival(tArrival, uiSN);
      }
      break;
    }
    case FB_ACK:
    {
      m_vReceivedSN.push_back(uiSN);
      break;
    }
  }
}

void FeedbackManager::onPacketArrival(const boost::posix_time::ptime &tArrival, const uint32_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN)
{
  LOG_FIRST_N(WARNING, 1) << "Multipath method not implemented in single path feedback manager";
}

void FeedbackManager::onRtpPacketAssumedLost(uint32_t uiSN)
{
  VLOG(1) << "Packet " << uiSN << " assumed lost";
  m_vAssumedLost.push_back(uiSN);

  if (m_onLoss) m_onLoss(uiSN);
}

void FeedbackManager::onRtxPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiOriginalSN)
{
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      // The RTO estimator has to be valid if we received an RTX
      assert(m_pRtoEstimator);
      // TODO: dummys
      bool bLate = false, bDuplicate = false;
      m_pRtoEstimator->onRtxPacketArrival(tArrival, uiOriginalSN, bLate, bDuplicate);

      // moving responsibility to RtpSessionManager
#if 0
      // lookup delay
      auto it = m_mRtxDurationMap.find(uiOriginalSN);
      VLOG(2) << "Duration map size: " << m_mRtxDurationMap.size();
      assert (it != m_mRtxDurationMap.end());

      VLOG(2) << "Retransmission of " << uiOriginalSN << " successful - took " << (tArrival - it->second).total_milliseconds() << "ms";
#endif
      break;
    }
    case FB_ACK:
    {
      m_vReceivedSN.push_back(uiOriginalSN);
      break;
    }
  }
}

void FeedbackManager::onRtxPacketArrival(const boost::posix_time::ptime &tArrival, const uint16_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN)
{
  LOG_FIRST_N(WARNING, 1) << "Multipath method not implemented in single path feedback manager";
}

// OLD interface
#if 0
void FeedbackManager::retrieveFeedbackReports( CompoundRtcpPacket& fb)
{
#if 0
  VLOG(15) << "Retrieving feedback for report";
  // CompoundRtcpPacket vFeedback;
  switch (m_eFeedbackMode)
  {
    case FB_NACK:
    {
      // process NACKs
      addGenericNacks(fb);
      break;
    }
    case FB_ACK:
    {
      // process ACKs
      addGenericAcks(fb);
      break;
    }
  }
  // return vFeedback;
#endif
}

void FeedbackManager::retrieveFeedbackReports(uint16_t uiFlowId, CompoundRtcpPacket& fb)
{
  LOG_FIRST_N(WARNING, 1) << "Multipath method not implemented in single path feedback manager";
  //return CompoundRtcpPacket();
}
#endif

bool FeedbackManager::onRtpPacketLate(uint32_t uiSN)
{
  VLOG(1) << "Packet " << uiSN << " late";
  // try remove packet if it is not too late already
  for (auto it = m_vAssumedLost.begin(); it != m_vAssumedLost.end(); ++it)
  {
    if (uiSN == *it)
    {
      VLOG(5) << "Removed late packet from feedback request: " << uiSN;
      m_vAssumedLost.erase(it);
      return true;
    }
  }
  VLOG(5) << "Too late to remove late packet from feedback request: " << uiSN;
  return false;
}

void FeedbackManager::onRtxRequested(const boost::posix_time::ptime& tRequested, const uint16_t uiSN)
{
  // update estimator with feedback time
  m_pRtoEstimator->onRtxRequested(tRequested, uiSN);
}

// deprecated
#if 0
void FeedbackManager::addGenericNacks(CompoundRtcpPacket &vFeedback)
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

    const int MAX_RTX = 30;
    if (m_vAssumedLost.size() < MAX_RTX)
    {
      VLOG(2) << "Adding NACKS for " << ::toString(m_vAssumedLost);
      for (uint32_t uiSN : m_vAssumedLost)
      {
        m_mRtxDurationMap.insert(std::make_pair(static_cast<uint16_t>(uiSN), tNow));
        VLOG(2) << uiSN << " inserted into duration map size: " << m_mRtxDurationMap.size();
        // update estimator with feedback time
        m_pRtoEstimator->onRtxRequested(tNow, uiSN);
      }
    }
    else
    {
      // TODO: here we should send a FIR instead
      VLOG(2) << "Discarding NACKS, too many packets assumed lost (" << m_vAssumedLost.size() << "): "  << ::toString(m_vAssumedLost);
    }
    m_vAssumedLost.clear();
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
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
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

} // rfc4585
} // rtp_plus_plus
