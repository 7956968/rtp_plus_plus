#include "CorePch.h"
#include <rtp++/rto/MultipathRtoEstimator.h>
#include <rtp++/rto/AR2Predictor.h>
#include <rtp++/rto/BasicRtoEstimator.h>
#include <rtp++/rto/MovingAveragePredictor.h>
#include <rtp++/rto/PacketInterarrivalTimePredictor.h>
#include <rtp++/rto/SimpleRtoEstimator.h>

/// the maximum number of consecutive losses before the estimator stops trying to estimate the next arrival
#define MAX_CONSECUTIVE_LOSSES 500

namespace rtp_plus_plus
{
namespace rto
{

MultipathRtoEstimator::MultipathRtoEstimator(boost::asio::io_service& rIoService, const GenericParameters &applicationParameters)
  :m_rIoService(rIoService),
  m_applicationParameters(applicationParameters),
  m_bShuttingdown(false),
  m_eType(getPredictorType(applicationParameters))
{

}

void MultipathRtoEstimator::doOnPacketArrival(const boost::posix_time::ptime& tArrival, const uint16_t uiSN, uint16_t uiFlowId, uint16_t uiFSSN)
{
  auto it = m_mEstimators.find(uiFlowId);
  if (it == m_mEstimators.end())
  {

    switch (m_eType)
    {
      case PT_AR2:
      {
        boost::optional<double> prematureTimeoutProb = m_applicationParameters.getDoubleParameter(app::ApplicationParameters::pto);
        double dPrematureTimeoutProb = prematureTimeoutProb ? *prematureTimeoutProb : 0.05;
        VLOG(2) << "Creating AR2 RTO estimator for flow id " << uiFlowId;
        std::unique_ptr<BasicRtoEstimator> pEstimator = std::unique_ptr<BasicRtoEstimator>(new BasicRtoEstimator(m_rIoService));
        pEstimator->setId(uiFlowId);
        pEstimator->setPredictor(std::unique_ptr<InterarrivalPredictor_t>(new PacketInterarrivalTimePredictor<uint64_t, int64_t>(dPrematureTimeoutProb)));
        m_mEstimators[uiFlowId] = std::move(pEstimator);
        break;
      }
      case PT_MOVING_AVERAGE:
      {
        boost::optional<double> prematureTimeoutProb = m_applicationParameters.getDoubleParameter(app::ApplicationParameters::pto);
        boost::optional<uint32_t> historySize = m_applicationParameters.getUintParameter(app::ApplicationParameters::mavg_hist);
        const uint32_t uiHistorySize = historySize ? *historySize : 20;
        double dPrematureTimeoutProb = prematureTimeoutProb ? *prematureTimeoutProb : 0.05;
        std::unique_ptr<BasicRtoEstimator> pEstimator = std::unique_ptr<BasicRtoEstimator>(new BasicRtoEstimator(m_rIoService));
        pEstimator->setId(uiFlowId);
        pEstimator->setPredictor(std::unique_ptr<InterarrivalPredictor_t>(new MovingAveragePredictor<uint64_t, int64_t>(uiHistorySize, uiHistorySize >> 1, dPrematureTimeoutProb)));
        m_mEstimators[uiFlowId] = std::move(pEstimator);
        VLOG(2) << "Creating MAVG RTO estimator for flow id " << uiFlowId;
        break;
      }
      case PT_SIMPLE:
      default:
      {
        VLOG(2) << "Creating Simple RTO estimator for flow id " << uiFlowId;
        m_mEstimators[uiFlowId] = std::unique_ptr<PacketLossDetectionBase>(new SimplePacketLossDetection());
      }
    }

    m_mEstimators[uiFlowId]->setLostCallback(std::bind(&MultipathRtoEstimator::onRtpPacketAssumedLostInFlow, this, std::placeholders::_1, uiFlowId));
  }

  m_mEstimators[uiFlowId]->onPacketArrival(tArrival, uiFSSN);

  // TODO: create mapping between FSSN and SN
}

//void MultipathRtoEstimator::onRtxRequested(const boost::posix_time::ptime& tRequested, const int uiSN, const int uiFlowId, const int uiFSSN)
//{

//}

//void MultipathRtoEstimator::onRtxPacketArrival(const boost::posix_time::ptime &tArrival, const int uiSN, const int uiFlowId, const int uiFSSN, bool bLate, bool bDuplicate)
//{

//}

void MultipathRtoEstimator::doStop()
{
  // set flag so that cancel doesn't respawn next timer
  m_bShuttingdown = true;
  // shutdown estimators
  for (auto& pair: m_mEstimators)
  {
    pair.second->stop();
  }
}

void MultipathRtoEstimator::onRtpPacketAssumedLostInFlow(uint16_t uiFSSN, uint16_t uiFlowId)
{
  // packet has been estimated as lost in flow: map to global SN
  VLOG(2) << "Packet with FSSN " << uiFSSN << " assumed lost in flow " << uiFlowId;
  m_onLost(-1, uiFlowId, uiFSSN);
}

} // rto
} // rtp_plus_plus

