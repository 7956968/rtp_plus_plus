#include "CorePch.h"
#include <rtp++/RtpSessionManager.h>
#include <cpputil/ExceptionBase.h>
#include <rtp++/RtpJitterBuffer.h>
#include <rtp++/RtpJitterBufferV2.h>
#include <rtp++/PtsBasedJitterBuffer.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/application/ApplicationUtil.h>
#include <rtp++/experimental/ExperimentalRtcp.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/experimental/RtcpHeaderExtension.h>
// #include <rtp++/experimental/Scream.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpFeedbackManager.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/FeedbackManager.h>
#include <rtp++/scheduling/SchedulerFactory.h>
#include <rtp++/TransmissionManager.h>

using boost::asio::deadline_timer;
using boost::optional;

namespace rtp_plus_plus
{

using media::MediaSample;

// jitter buffers
static const uint32_t STANDARD_JITTER_BUFFER = 0;
static const uint32_t PTS_JITTER_BUFFER = 1;
static const uint32_t JITTER_BUFFER_V2 = 2;

std::unique_ptr<RtpSessionManager> RtpSessionManager::create(boost::asio::io_service& ioService,
                                                             const GenericParameters& applicationParameters)
{
  return std::unique_ptr<RtpSessionManager>( new RtpSessionManager(ioService, applicationParameters) );
}

RtpSessionManager::RtpSessionManager(boost::asio::io_service& ioService,
                                     const GenericParameters& applicationParameters)
  :IRtpSessionManager(applicationParameters),
    m_rIoService(ioService),
    m_bExitOnBye(false),
    m_bAnalyse(false),
    m_bFirstSyncedVideo(false),
    m_uiLastReceived(0),
    m_bAckEnabled(false),
    m_bNackEnabled(false)//,
//    m_bScreamEnabled(false),
//    m_uiLastExtendedSNReported(0),
//    m_uiLastLoss(0)
{
  uint32_t uiBufLat = IRtpJitterBuffer::MAX_LATENCY_MS;
  // set buffer latency if configured in application parameters
  optional<uint32_t> uiBufferLatency = applicationParameters.getUintParameter(app::ApplicationParameters::buf_lat);
  if (uiBufferLatency) uiBufLat = *uiBufferLatency;

  optional<uint32_t> uiJitterBufferType = applicationParameters.getUintParameter(app::ApplicationParameters::jitter_buffer_type);
  if (uiJitterBufferType)
  {
    switch(*uiJitterBufferType)
    {
      case PTS_JITTER_BUFFER:
      {
        m_pReceiverBuffer = PtsBasedJitterBuffer::create(uiBufLat);
        break;
      }
      case STANDARD_JITTER_BUFFER:
      {
        m_pReceiverBuffer = RtpJitterBuffer::create(uiBufLat);
        break;
      }
      case JITTER_BUFFER_V2:
      default:
      {
        m_pReceiverBuffer = RtpJitterBufferV2::create(uiBufLat);
        break;
      }
    }
  }
  else
  {
    m_pReceiverBuffer = RtpJitterBufferV2::create(uiBufLat);
  }

  optional<bool> exitOnBye = applicationParameters.getBoolParameter(app::ApplicationParameters::exit_on_bye);
  if (exitOnBye) m_bExitOnBye = *exitOnBye;

  optional<bool> bAnalyse = applicationParameters.getBoolParameter(app::ApplicationParameters::analyse);
  if (bAnalyse) m_bAnalyse = *bAnalyse;

  VLOG(5) << "RtpSessionManager: exit on BYE: " << m_bExitOnBye << " analyse: " << m_bAnalyse;
}

RtpSessionManager::~RtpSessionManager()
{
  std::ostringstream ostr;
  const RtpSessionStatistics& stats = m_pRtpSession->getRtpSessionStatistics();
  VLOG(2) << "Analyse: " << m_bAnalyse << " Total packets received: " << stats.getOverallStatistic().getTotalRtpPacketsReceived();
  if (m_bAnalyse && stats.getOverallStatistic().getTotalRtpPacketsReceived() > 0)
  {
    assert(!m_vSequenceNumbers.empty());
    // this code assumes that the RTP SN has been forced for the measurement.
    // further, it assumes that there has been no wraparound!
    uint32_t uiMin = m_vSequenceNumbers[0];
    uint32_t uiMax = m_vSequenceNumbers[m_vSequenceNumbers.size() - 1];
    uint32_t uiTotal = uiMax - uiMin + 1;
    if (uiMin > uiMax)
    {
      LOG(WARNING) << "First SN > last SN. Invalid measurement";
    }
    else
    {
      uint32_t uiLost = 0;
      for (size_t i = uiMin; i <= uiMax; ++i)
      {
        auto it = std::find_if(m_vSequenceNumbers.begin(), m_vSequenceNumbers.end(), [i](uint32_t uiSN)
        {
          return (uint32_t)i == uiSN;
        });
        if (it == m_vSequenceNumbers.end())
        {
          // couldn't find it
          ostr << i << " ";
          ++uiLost;
        }
      }
      double dLossPercentage = 100*static_cast<double>(uiLost)/uiTotal;
      double dDiscardPercentage = 100*static_cast<double>(m_vDiscards.size())/uiTotal;
      double dTotalUnusable = 100*static_cast<double>(uiLost + m_vDiscards.size())/uiTotal;
      // lateness stats
      double dAverage = std::accumulate(m_vLateness.begin(), m_vLateness.end(), 0)/static_cast<double>(m_vLateness.size());

      std::vector<double> vDiffs(m_vLateness.size());
      std::transform(m_vLateness.begin(), m_vLateness.end(), vDiffs.begin(), std::bind2nd(std::minus<double>(), dAverage));
      double dSum = std::inner_product(vDiffs.begin(), vDiffs.end(), vDiffs.begin(), 0.0);
      double dStdDev = std::sqrt(dSum/vDiffs.size());

      VLOG(2) << "RTP Stats Unusable: " << uiLost + m_vDiscards.size() << "/" << uiTotal << " " <<  dTotalUnusable 
              << "% Lost: " << uiLost << " " <<  dLossPercentage 
              << "% Discarded: " << m_vDiscards.size() << " " << dDiscardPercentage 
              << "% Late Mean: " << dAverage
              << "ms Stddev: " << dStdDev << std::endl
              << " RTP sequence number losses: " << ostr.str() << std::endl
              << " RTP sequence number discards: " << ::toString(m_vDiscards);
    }
  }
}

void RtpSessionManager::onFeedbackGeneration(CompoundRtcpPacket& compoundRtcp)
{
  VLOG(12) << "RtpSessionManager::onFeedbackGeneration";
  const RtpSessionState& state = m_pRtpSession->getRtpSessionState();
  // NACKS
  if (m_bNackEnabled)
  {
    std::vector<uint32_t> vLost = m_pFeedbackManager->getAssumedLost();
    if (!vLost.empty())
    {
      const int MAX_RTX = 30;
      if (vLost.size() < MAX_RTX)
      {
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        VLOG(2) << "Adding NACKS for " << ::toString(vLost);
        // TODO: notify fb manager of RTX request time
#if 1
        for (uint32_t uiSN : vLost)
        {
          m_mRtxDurationMap.insert(std::make_pair(static_cast<uint16_t>(uiSN), tNow));
          // update estimator with feedback time
          m_pFeedbackManager->onRtxRequested(tNow, uiSN);
        }
#endif
        rfc4585::RtcpGenericNack::ptr pNack = rfc4585::RtcpGenericNack::create(vLost, state.getRemoteSSRC(), state.getSSRC());
        compoundRtcp.push_back(pNack);
      }
      else
      {
        // TODO: here we should send a FIR instead
        LOG(INFO) << "Discarding NACKS, too many packets assumed lost (" << vLost.size() << "): "  << ::toString(vLost);
      }
    }
  }

  // ACKS
  if (m_bAckEnabled)
  {
#if 0
    std::vector<uint32_t> vReceived = m_pFeedbackManager->getReceived();
#else
    // need to check if we've received anything since previous ACK feedback generation
    const int MAX_PER_GENERIC_ACK = 17;
    std::vector<uint32_t> vReceived = m_pTxManager->getLastNReceivedSequenceNumbers(MAX_PER_GENERIC_ACK); // this fits into one Generic ACK
#endif
    if (!vReceived.empty())
    {
      uint32_t uiLastReceived = vReceived[vReceived.size() - 1];
      if (uiLastReceived != m_uiLastReceived)
      {
        m_uiLastReceived = uiLastReceived;
        VLOG(2) << "Got last " << vReceived.size() << " received sequence numbers: " << ::toString(vReceived);
        rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(vReceived, state.getRemoteSSRC(), state.getSSRC());
        compoundRtcp.push_back(pAck);
      }
    }
  }

  // moved to ScreamScheduler
#if 0
  // add scream reports if enabled
  if (m_bScreamEnabled)
  {
    uint32_t uiHighestSN = m_pTxManager->getLastReceivedExtendedSN();
    VLOG(2) << "RtpSessionManager::onFeedbackGeneration: highest SN: " << uiHighestSN << " last reported: " << m_uiLastExtendedSNReported;
    if (m_uiLastExtendedSNReported != uiHighestSN) // only report if there is new info
    {
      VLOG(2) << "RtpSessionManager::onFeedbackGeneration: Reporting new scream info";
      m_uiLastExtendedSNReported = uiHighestSN;
      boost::posix_time::ptime tLastTime = m_pTxManager->getPacketArrivalTime(uiHighestSN);
      boost::posix_time::ptime tInitial =  m_pTxManager->getInitialPacketArrivalTime();
      uint32_t uiJiffy = (tLastTime - tInitial).total_milliseconds();
      m_uiLastLoss = m_pRtpSession->getCumulativeLostAsReceiver();
      experimental::RtcpScreamFb::ptr pFb = experimental::RtcpScreamFb::create(uiJiffy, m_uiLastExtendedSNReported, m_uiLastLoss, state.getRemoteSSRC(), state.getSSRC());
      compoundRtcp.push_back(pFb);
    }
  }
#if 0
  else
  {
    VLOG(5) << "Scream not enabled";
  }
#endif
#else
  std::vector<rfc4585::RtcpFb::ptr> feedback = m_pScheduler->retrieveFeedback();
  if (!feedback.empty())
  {
    compoundRtcp.insert(compoundRtcp.end(), feedback.begin(), feedback.end());
    VLOG(12) << "Added " << feedback.size() << " FB reports to compound packet final size " << compoundRtcp.size();
  }
#endif
}

void RtpSessionManager::onMpFeedbackGeneration(uint16_t uiFlowId, CompoundRtcpPacket& compoundRtcp)
{
  // in the new interface we could retrieve ACKS along all paths and send them in the feedback
  // or we could only send feedback along the fastest flow e.g. if (uiFlowId == fastest) then retrieve fb
  const RtpSessionState& state = m_pRtpSession->getRtpSessionState();

  // NACKS
  if (m_bNackEnabled)
  {
    std::vector<uint32_t> vLost = m_pFeedbackManager->getAssumedLost();
    if (!vLost.empty())
    {
      const int MAX_RTX = 30;
      if (vLost.size() < MAX_RTX)
      {
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        VLOG(2) << "Adding NACKS for " << ::toString(vLost);
        // TODO: notify fb manager of RTX request time
#if 1
        for (uint32_t uiSN : vLost)
        {
          m_mRtxDurationMap.insert(std::make_pair(static_cast<uint16_t>(uiSN), tNow));
          // update estimator with feedback time
          m_pFeedbackManager->onRtxRequested(tNow, uiSN);
        }
#endif
        rfc4585::RtcpGenericNack::ptr pNack = rfc4585::RtcpGenericNack::create(vLost, state.getRemoteSSRC(), state.getSSRC());
        compoundRtcp.push_back(pNack);
      }
      else
      {
        // TODO: here we should send a FIR instead
        LOG(INFO) << "Discarding NACKS, too many packets assumed lost (" << vLost.size() << "): "  << ::toString(vLost);
      }
    }
  }

  if (m_bNackEnabled)
  {
    std::vector<std::tuple<uint16_t, uint16_t>> vFlowSpecificLosses = m_pFeedbackManager->getFlowSpecificLost();
    if (!vFlowSpecificLosses.empty())
    {
#if 0
      const int MAX_RTX = 30;
      if (vLost.size() < MAX_RTX)
      {
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        VLOG(2) << "Adding NACKS for " << ::toString(vLost);
        // TODO: notify fb manager of RTX request time
#if 1
        for (uint32_t uiSN : vLost)
        {
          m_mRtxDurationMap.insert(std::make_pair(static_cast<uint16_t>(uiSN), tNow));
          // update estimator with feedback time
          m_pFeedbackManager->onRtxRequested(tNow, uiSN);
        }
#endif
        rfc4585::RtcpGenericNack::ptr pNack = rfc4585::RtcpGenericNack::create(vLost, state.getRemoteSSRC(), state.getSSRC());
        compoundRtcp.push_back(pNack);
      }
      else
      {
        // TODO: here we should send a FIR instead
        LOG(INFO) << "Discarding NACKS, too many packets assumed lost (" << vLost.size() << "): "  << ::toString(vLost);
      }
#else
      std::map<uint16_t, std::vector<uint16_t> > mLossMap;
      // for now there is just one generic NACK
      for (size_t i = 0; i < vFlowSpecificLosses.size(); ++i)
      {
        mLossMap[std::get<0>(vFlowSpecificLosses[i])].push_back(std::get<1>(vFlowSpecificLosses[i]));
      }
      VLOG(2) << "Found " << mLossMap.size() << " flows with losses";
      for (auto& it : mLossMap)
      {
        VLOG(2) << "Adding extended NACK for flow " << it.first << " with " << it.second.size()
                << " items: " << ::toString(it.second);
        rfc4585::RtcpExtendedNack::ptr pNack = rfc4585::RtcpExtendedNack::create(it.first, it.second);
        // get own SSRC
        pNack->setSenderSSRC(state.getSSRC());
        // get the source SSRC
        pNack->setSourceSSRC(state.getRemoteSSRC());
        compoundRtcp.push_back(pNack);
      }
      // The code as it is can only handle one source SSRC?!?
      // We would have to create one RTO per source to be able
      // to handle multiple sources?

      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#if 1
      for (auto& loss : vFlowSpecificLosses)
      {
        uint32_t uiFlowFssn = (std::get<0>(loss) << 16) | std::get<1>(loss);
        VLOG(2) << "Flow specific loss: Flow: " << std::get<0>(loss) << " FSSN: " << std::get<1>(loss) << " Flow-FSSN: " << uiFlowFssn;
        m_mFlowSpecificRtxDurationMap.insert(std::make_pair(uiFlowFssn, tNow));
        // update estimator with feedback time
        m_pFeedbackManager->onRtxRequested(tNow, std::get<0>(loss), std::get<1>(loss));
#if 0
        m_pMpRtoEstimator->onRtxRequested(tNow, rto::SN_NOT_SET, std::get<0>(loss), std::get<1>(loss));
        if (m_pMpRtoEstimator2)
          m_pMpRtoEstimator2->onRtxRequested(tNow, rto::SN_NOT_SET, std::get<0>(loss), std::get<1>(loss));
#endif
      }
    }
#endif

#endif
  }
}

void RtpSessionManager::setRtpSession(RtpSession::ptr pRtpSession)
{
  m_pRtpSession = pRtpSession;
  // bind all the handlers
  m_pRtpSession->setIncomingRtpHandler(
        boost::bind(&RtpSessionManager::handleIncomingRtp, this, _1, _2, _3, _4, _5));
  m_pRtpSession->setIncomingRtcpHandler(
        boost::bind(&RtpSessionManager::handleIncomingRtcp, this, _1, _2));
  m_pRtpSession->setBeforeOutgoingRtpHandler(
        boost::bind(&RtpSessionManager::handleBeforeOutgoingRtp, this, _1, _2));
  m_pRtpSession->setOutgoingRtpHandler(
        boost::bind(&RtpSessionManager::handleOutgoingRtp, this, _1, _2));
  m_pRtpSession->setOutgoingRtcpHandler(
        boost::bind(&RtpSessionManager::handleOutgoingRtcp, this, _1, _2));
  m_pRtpSession->setMemberUpdateHandler(
        boost::bind(&RtpSessionManager::handleMemberUpdate, this, _1));
  m_pRtpSession->setRtpSessionCompleteHandler(
        boost::bind(&RtpSessionManager::handleRtpSessionComplete, this));
  m_pRtpSession->setFeedbackCallback(
        boost::bind(&RtpSessionManager::onFeedbackGeneration, this, _1));
  m_pRtpSession->setMpFeedbackCallback(
        boost::bind(&RtpSessionManager::onMpFeedbackGeneration, this, _1, _2));

  m_sCodec = m_pRtpSession->getRtpSessionParameters().getEncodingName();

  const RtpSessionParameters& rtpParameters = m_pRtpSession->getRtpSessionParameters();
  // configure receiver buffer
  m_pReceiverBuffer->setClockFrequency(rtpParameters.getRtpTimestampFrequency());
  // ack enabled
  m_bAckEnabled = rtpParameters.supportsFeedbackMessage(rfc4585::ACK);
  m_bNackEnabled = rtpParameters.supportsFeedbackMessage(rfc4585::NACK);
#if 0
  m_bScreamEnabled = rtpParameters.supportsFeedbackMessage(experimental::SCREAM);
#endif
  VLOG(2) << "ACK enabled: " << m_bAckEnabled
          << " Nack enabled: " << m_bNackEnabled;
          // << " Scream enabled: " << m_bScreamEnabled;

  RtpSessionState& state = m_pRtpSession->getRtpSessionState();
  // configure RTX
  // if (rtpParameters.isRetransmissionEnabled())
  {
    VLOG(2) << "Creating retransmission manager";

    // investigating hybrid ACK and NACK mode: in that case ACK-mode controls buffer management
    TxBufferManagementMode eMode = (rtpParameters.supportsFeedbackMessage(rfc4585::ACK)) ? TxBufferManagementMode::ACK_MODE
           : (rtpParameters.supportsFeedbackMessage(rfc4585::NACK) ? TxBufferManagementMode::NACK_TIMED_MODE
           : TxBufferManagementMode::CIRCULAR_MODE);

    m_pTxManager = std::unique_ptr<TransmissionManager>(new TransmissionManager(state,
                                                                                     m_rIoService,
                                                                                     eMode,
                                                                                     rtpParameters.getRetransmissionTimeout()));
  }

  if (m_pRtpSession->isMpRtpSession())
  {
    VLOG(2) << "Creating MPRTP feedback manager";
    m_pFeedbackManager = std::unique_ptr<IFeedbackManager>( new mprtp::FeedbackManager(rtpParameters, m_applicationParameters, m_rIoService ));
#if 0
    m_pFeedbackManager->setSSRC(state.getSSRC());
#endif
    m_pFeedbackManager->setLossCallback(boost::bind(&RtpSessionManager::onPacketAssumedLost, this, _1));
    m_pFeedbackManager->setFlowSpecificLossCallback(boost::bind(&RtpSessionManager::onPacketAssumedLostInFlow, this, _1, _2));
#if 0
    m_pRtpSession->setFeedbackManager(m_pFeedbackManager.get());
#endif
  }
  else
  {
    if (rfc4585::isAudioVisualProfileWithFeedback(rtpParameters))
    {
      VLOG(2) << "Creating RTP feedback manager";
      m_pFeedbackManager = std::unique_ptr<rfc4585::FeedbackManager>( new rfc4585::FeedbackManager(rtpParameters, m_applicationParameters, m_rIoService ));
#if 0
      m_pFeedbackManager->setSSRC(state.getSSRC());
#endif
      m_pFeedbackManager->setLossCallback(boost::bind(&RtpSessionManager::onPacketAssumedLost, this, _1));
#if 0
      m_pRtpSession->setFeedbackManager(m_pFeedbackManager.get());
#endif
    }
    else
    {
      VLOG(2) << "Not creating RTP feedback manager";
    }
  }

  // pRtpSession->getNumberOfFlows() not valid until after RtpSession has been started
#if 0
  // create scheduler
  optional<uint32_t> uiScheduler = m_applicationParameters.getUintParameter(app::ApplicationParameters::scheduler);
  optional<std::string> sSchedulingAlgorithm = m_applicationParameters.getStringParameter(app::ApplicationParameters::mprtp_sa);
  uint32_t uiType = uiScheduler ? *uiScheduler : 0;
  std::string sSchedulingParameter = sSchedulingAlgorithm ? *sSchedulingAlgorithm : "";
  m_pScheduler = std::move(SchedulerFactory::create(m_pRtpSession, uiType, m_rIoService, pRtpSession->getNumberOfFlows(), sSchedulingParameter));
#endif
}

std::vector<std::string> RtpSessionManager::getMediaTypes() const
{
  std::vector<std::string> vMediaTypes;
  vMediaTypes.push_back(m_pRtpSession->getRtpSessionParameters().getEncodingName());
  return vMediaTypes;
}

std::string RtpSessionManager::getPrimaryMediaType() const
{
  assert(m_pRtpSession);
  return m_pRtpSession->getRtpSessionParameters().getEncodingName();
}

const RtpSessionParameters& RtpSessionManager::getPrimaryRtpSessionParameters() const
{
  if (!m_pRtpSession)
    BOOST_THROW_EXCEPTION(ExceptionBase("No RTP session has been set"));

  return m_pRtpSession->getRtpSessionParameters();
}

RtpSession& RtpSessionManager::getPrimaryRtpSession()
{
  return *(m_pRtpSession.get());
}

boost::system::error_code RtpSessionManager::start()
{
  assert(m_pRtpSession);
  boost::system::error_code ec = m_pRtpSession->start();
  if (ec)
  {
    LOG(WARNING) << "Failed to start RTP session: " << ec.message();
  }
  else
  {
    onSessionStarted();
  }
  return ec;
}

boost::system::error_code RtpSessionManager::stop()
{
  if (m_pScheduler)
  {
    m_pScheduler->shutdown();
  }

  assert(m_pRtpSession);
  boost::system::error_code ec = m_pRtpSession->stop();
  if (ec)
  {
    // THe session might already have been stopped
    VLOG(5) << "Failed to stop RTP session: " << ec.message();
  }

  if (m_pFeedbackManager)
    m_pFeedbackManager->shutdown();

  // RTX
  if (m_pTxManager)
  {
    m_pTxManager->stop();
  }

  onSessionStopped();

  // stop all scheduled samples
  // cancel all scheduled media sample events
  VLOG(10) << "[" << this << "] Shutting down Playout timers";
  {
    boost::mutex::scoped_lock l(m_timerLock);
    for (auto& pair : m_mPlayoutTimers)
    {
      pair.second->cancel();
    }
  }

  return boost::system::error_code();
}

void RtpSessionManager::send(const MediaSample& mediaSample)
{
  assert(m_pRtpSession);
  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
  if (!rtpPackets.empty())
    m_pScheduler->scheduleRtpPackets(rtpPackets);
}

void RtpSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
  assert(m_pRtpSession);
  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << vMediaSamples[0].getStartTime()
             << " Samples in AU: " << vMediaSamples.size()
             << " into " << rtpPackets.size() << " RTP packets"
             << " RTP TS: " << rtpPackets[0].getRtpTimestamp();

#endif
  m_pScheduler->scheduleRtpPackets(rtpPackets);
}

bool RtpSessionManager::isSampleAvailable() const
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  return !m_qOutgoing.empty();
}

std::vector<MediaSample> RtpSessionManager::getNextSample()
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  if (m_qOutgoing.empty()) return std::vector<MediaSample>();
  std::vector<MediaSample> mediaSamples = m_qOutgoing.front();
  m_qOutgoing.pop_front();
  return mediaSamples;
}

void RtpSessionManager::handleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                          bool bSSRCValidated, bool bRtcpSynchronised,
                                          const boost::posix_time::ptime& tPresentation)
{
  VLOG(15) << "Incoming RTP packet with SN: " << rtpPacket.getExtendedSequenceNumber()
           << " Presentation time: " << tPresentation
           << " From: " << ep
           << " Validated: " << bSSRCValidated
           << " Sync: " << bRtcpSynchronised;

  if (m_bAnalyse)
  {
    m_vPacketStats.push_back(std::make_tuple(rtpPacket.getArrivalTime(), tPresentation, bRtcpSynchronised));
  }

  doHandleIncomingRtp(rtpPacket, ep, bSSRCValidated, bRtcpSynchronised, tPresentation);
}

void RtpSessionManager::handleBeforeOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  VLOG(15) << "Before outgoing RTP to : " << ep;
  doHandleBeforeOutgoingRtp(rtpPacket, ep);
}


void RtpSessionManager::handleOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  VLOG(15) << "Outgoing RTP to : " << ep;
  doHandleOutgoingRtp(rtpPacket, ep);
}

void RtpSessionManager::handleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp,
                                           const EndPoint& ep)
{
  VLOG(15) << "Incoming RTCP from : " << ep;
  doHandleIncomingRtcp(compoundRtcp, ep);
}

void RtpSessionManager::handleOutgoingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep)
{
  VLOG(15) << "Outgoing RTCP to : " << ep;
}

void RtpSessionManager::doHandleBeforeOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  VLOG(15) << "RtpSessionManager::doHandleBeforeOutgoingRtp to " << ep;
  if (m_pFeedbackManager)
  {
    const RtpSessionParameters& parameters = m_pRtpSession->getRtpSessionParameters();
    boost::optional<rfc5285::Extmap> extmap = parameters.lookupExtmap(experimental::EXTENSION_NAME_RTCP_HEADER_EXT);
    if (extmap)
    {
      CompoundRtcpPacket compoundRtcp;
      // NACKS?
      if (m_pFeedbackManager->getAssumedLostCount() > 0)
      {
        VLOG(2) << "RtpSessionManager::doHandleBeforeOutgoingRtp assumed lost count: " <<  m_pFeedbackManager->getAssumedLostCount();
        // TODO: we could bundle the NACKS into the next outgoing RTP packet
      }

      // if in ACK mode, get the ACKs from feedback manager
      // only add the special header if acks are enabled
      if (m_bAckEnabled && m_pFeedbackManager->getReceivedCount() > 0)
      {
        VLOG(12) << "RtpSessionManager::doHandleBeforeOutgoingRtp received count: sending ACK with HDR ext " <<  m_pFeedbackManager->getReceivedCount();
        // std::vector<uint32_t> vReceived = m_pFeedbackManager->getReceived();
        const int MAX_PER_GENERIC_ACK = 17;
        std::vector<uint32_t> vReceived = m_pTxManager->getLastNReceivedSequenceNumbers(MAX_PER_GENERIC_ACK); // this fits into one Generic ACK

        uint32_t uiLastReceived = vReceived[vReceived.size() - 1];
        if (uiLastReceived != m_uiLastReceived)
        {
          m_uiLastReceived = uiLastReceived;
          VLOG(2) << "Adding ACKs for last " << vReceived.size() << " received sequence numbers: " << ::toString(vReceived);
          RtpSessionState& state = m_pRtpSession->getRtpSessionState();
          rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(vReceived, state.getRemoteSSRC(), state.getSSRC());
          compoundRtcp.push_back(pAck);
        }
      }

#if 0
      // add scream reports if enabled
      if (m_bScreamEnabled)
      {
        uint32_t uiHighestSN = m_pTxManager->getLastReceivedExtendedSN();
        VLOG(2) << "RtpSessionManager::doHandleBeforeOutgoingRtp highest SN: " << uiHighestSN << " last reported: " << m_uiLastExtendedSNReported;
        if (m_uiLastExtendedSNReported != uiHighestSN) // only report if there is new info
        {
          VLOG(2) << "RtpSessionManager::doHandleBeforeOutgoingRtp Reporting new scream info";
          m_uiLastExtendedSNReported = uiHighestSN;
          boost::posix_time::ptime tLastTime = m_pTxManager->getPacketArrivalTime(uiHighestSN);
          boost::posix_time::ptime tInitial =  m_pTxManager->getInitialPacketArrivalTime();
          uint32_t uiJiffy = (tLastTime - tInitial).total_milliseconds();
          m_uiLastLoss = m_pRtpSession->getCumulativeLostAsReceiver();
          RtpSessionState& state = m_pRtpSession->getRtpSessionState();
          experimental::RtcpScreamFb::ptr pFb = experimental::RtcpScreamFb::create(uiJiffy, m_uiLastExtendedSNReported, m_uiLastLoss, state.getRemoteSSRC(), state.getSSRC());
          compoundRtcp.push_back(pFb);
        }
      }
      else
      {
        LOG_FIRST_N(INFO, 1) << "RtpSessionManager::doHandleBeforeOutgoingRtp Scream is not enabled";
      }
#else
      std::vector<rfc4585::RtcpFb::ptr> feedback = m_pScheduler->retrieveFeedback();
      if (!feedback.empty())
      {
        compoundRtcp.insert(compoundRtcp.end(), feedback.begin(), feedback.end());
        VLOG(2) << "Added " << feedback.size() << " FB reports to compound packet final size " << compoundRtcp.size();
      }
#endif

      if (!compoundRtcp.empty())
      {
        VLOG(5) << "Adding RTCP to RTP header extension";
        // TODO: need to make sure that RTP packet + RTCP header ext < MTU
        boost::optional<rfc5285::HeaderExtensionElement> pExt = experimental::RtcpHeaderExtension::create(extmap->getId(), compoundRtcp);
        RtpPacket& packet = const_cast<RtpPacket&>(rtpPacket);
        packet.addRtpHeaderExtension(*pExt);
      }
      else
      {
        VLOG(5) << "Empty compound packet";
      }
#if 0
      else
      {
        VLOG(5) << "Scream not enabled";
      }
#endif
    } // extmap
    else
    {
      // RTCP RTP HDR EXT not supported, need to send ACKs with RTCP
      LOG_FIRST_N(INFO, 1) << "RtpSessionManager::doHandleBeforeOutgoingRtp RTCP-RTP header ext not supported. Sending feedback in RTCP: " <<  m_pFeedbackManager->getReceivedCount();
    }
  } // m_pFeedbackManager
  else
  {
    LOG_FIRST_N(INFO, 1) << "RtpSessionManager::doHandleBeforeOutgoingRtp No feedback manager configured";
  }
}

void RtpSessionManager::doHandleOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  if (m_pTxManager)
  {
    m_pTxManager->storePacketForRetransmission(rtpPacket);
  }
}

void RtpSessionManager::doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                          bool bSSRCValidated, bool bRtcpSynchronised,
                                          const boost::posix_time::ptime& tPresentation)
{
  const RtpSessionParameters& parameters = m_pRtpSession->getRtpSessionParameters();
  if (!parameters.isRtxPayloadType(rtpPacket.getHeader().getPayloadType()))
  {
    // only do RTO if we can also send NACKs
    if (parameters.isRetransmissionEnabled())
    {
      assert(m_pFeedbackManager);
      if (m_pRtpSession->isMpRtpSession())
      {
        // at this point the header extension SHOULD have been parsed and set
        boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
        assert(pSubflowHeader);
        uint16_t uiFlowId = pSubflowHeader->getFlowId();
        uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
        m_pFeedbackManager->onPacketArrival(rtpPacket.getArrivalTime(), rtpPacket.getExtendedSequenceNumber(), uiFlowId, uiFlowSpecificSequenceNumber);
      }
      else
      {
        m_pFeedbackManager->onPacketArrival(rtpPacket.getArrivalTime(), rtpPacket.getExtendedSequenceNumber());
      }
    }

    m_pTxManager->onPacketReceived(rtpPacket);
  }
  else
  {
    RtpPacket restoredRtpPacket = m_pTxManager->processRetransmission(rtpPacket);
    m_pTxManager->onPacketReceived(restoredRtpPacket);
    uint32_t uiOriginalSN = restoredRtpPacket.getSequenceNumber();
    const boost::posix_time::ptime& tArrival = rtpPacket.getArrivalTime();
    // get SSRC of normal RTP session and update SSRC
    // TODO: do we need this?
#if 0
    restoredRtpPacket.getHeader().setSSRC(m_rtpSessionState.getRemoteSSRC());
#endif
    VLOG(5) << "Restored RTX packet: " << restoredRtpPacket.getHeader();
    if (m_pFeedbackManager)
    {
      if (m_pRtpSession->isMpRtpSession())
      {
        // at this point the header extension SHOULD have been parsed and set
        boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
        assert(pSubflowHeader);
        // at this point the header extension SHOULD have been parsed and set
        boost::optional<mprtp::MpRtpSubflowRtpHeader> pRestoredSubflowHeader = restoredRtpPacket.getMpRtpSubflowHeader();
        assert(pSubflowHeader);
        /**
          // in MPRTP we have to deal with new SN, flow ID & FSSN, as well as original SN, flow Id & FSSN
          rfc5285::RtpHeaderExtension::ptr pHeaderExtension = rtpPacket.getHeader().getHeaderExtension();
          rfc5285::RtpHeaderExtension::ptr pRestoredHeaderExtension = restoredRtpPacket.getHeader().getHeaderExtension();
          assert (pHeaderExtension && pRestoredHeaderExtension);
          // Potentially expensive???: create a work around *if* this proves to be a problem
          pSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
          pRestoredSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pRestoredHeaderExtension.get());
          assert(pSubflowHeader && pRestoredSubflowHeader);
          */
        uint16_t uiFlowId = pSubflowHeader->getFlowId();
        uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
        uint16_t uiOriginalFlowId = pRestoredSubflowHeader->getFlowId();
        uint16_t uiOriginalFlowSpecificSequenceNumber = pRestoredSubflowHeader->getFlowSpecificSequenceNumber();
        VLOG(15) << "RTX received. Original SN: " << uiOriginalSN << " Flow: " << uiOriginalFlowId << " FSSN: " << uiOriginalFlowSpecificSequenceNumber
                 << " SN: " << rtpPacket.getSequenceNumber() << " Flow: " << uiFlowId << " FSSN: " << uiFlowSpecificSequenceNumber;

        // lookup delay
        auto it = m_mRtxDurationMap.find(uiOriginalSN);
        // assert (it != m_mRtxDurationMap.end());

        if (it != m_mRtxDurationMap.end())
        {
          VLOG(2) << "Retransmission of " << uiOriginalSN << " successful - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }
        else
        {
          uint32_t uiFlowIdFssn = (uiFlowId << 16) | uiFlowSpecificSequenceNumber;
          VLOG(2) << "RTX Packet arrival:" << uiOriginalSN << " Flow: " << uiFlowId << " FSSN: " << uiFlowSpecificSequenceNumber << " Flow-FSSN: " << uiFlowIdFssn;
          // search in flow specific sequence number map
          auto it = m_mFlowSpecificRtxDurationMap.find(uiFlowIdFssn);
          assert (it != m_mFlowSpecificRtxDurationMap.end());
          VLOG(2) << "Retransmission late of " << uiOriginalSN << " - took " << (tArrival - it->second).total_milliseconds() << "ms";
        }

        m_pFeedbackManager->onRtxPacketArrival(rtpPacket.getArrivalTime(), uiOriginalSN, uiOriginalFlowId, uiOriginalFlowSpecificSequenceNumber);
      }
      else
      {
        // lookup delay
        auto it = m_mRtxDurationMap.find(uiOriginalSN);
        VLOG(2) << "Duration map size: " << m_mRtxDurationMap.size();
        // assert (it != m_mRtxDurationMap.end());
        if (it != m_mRtxDurationMap.end())
        {
          VLOG(2) << "Retransmission of " << uiOriginalSN << " successful - took " << (rtpPacket.getArrivalTime() - it->second).total_milliseconds() << "ms";
        }

        m_pFeedbackManager->onRtxPacketArrival(rtpPacket.getArrivalTime(), uiOriginalSN);
      }
    }
  }

  if (m_pScheduler)
  {
    m_pScheduler->onIncomingRtp(rtpPacket, ep, bSSRCValidated, bRtcpSynchronised, tPresentation);
  }

  boost::posix_time::ptime tPlayout;
  // Note: the playout buffer only returns true if a new playout time was calculated
  // for the sample
  uint32_t uiLateMs = 0;
  bool bDuplicate = false;
  if (m_pReceiverBuffer->addRtpPacket(rtpPacket, tPresentation, bRtcpSynchronised, tPlayout, uiLateMs, bDuplicate))
  {
    using boost::asio::deadline_timer;
    // a playout time has now been scheduled: create a timed event
    deadline_timer* pTimer = new deadline_timer(m_rIoService);
#ifdef DEBUG_PLAYOUT_DEADLINE
    ptime tNow = microsec_clock::universal_time();
    long diffMs = (tPlayout - tNow).total_milliseconds();
    VLOG(10) << "Scheduling packet for playout at " << tPlayout << " (in " << diffMs << "ms)";
#endif
    pTimer->expires_at(tPlayout);
    pTimer->async_wait(boost::bind(&RtpSessionManager::onScheduledSampleTimeout, this, _1, pTimer) );
    boost::mutex::scoped_lock l(m_timerLock);
    m_mPlayoutTimers.insert(std::make_pair(pTimer, pTimer));
  }
  else
  {
    // log discards
    if (m_bAnalyse && uiLateMs)
    {
      m_vDiscards.push_back(rtpPacket.getExtendedSequenceNumber());
      m_vLateness.push_back(uiLateMs);
    }
  }

  if (m_bAnalyse)
    m_vSequenceNumbers.push_back(rtpPacket.getExtendedSequenceNumber());
}

void RtpSessionManager::doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp,
                                           const EndPoint& ep)
{
  for (size_t i = 0; i < compoundRtcp.size(); ++i)
  {
    switch (compoundRtcp[i]->getPacketType())
    {
      case rfc3550::PT_RTCP_SR:
      {
        rfc3550::RtcpSr& sr = *(static_cast<rfc3550::RtcpSr*>(compoundRtcp[i].get()));
        if (m_onSr) m_onSr(sr, ep);
        break;
      }
      case rfc3550::PT_RTCP_RR:
      {
        if (m_onRr) m_onRr(*(static_cast<rfc3550::RtcpRr*>(compoundRtcp[i].get())), ep);
        break;
      }
      case rfc3550::PT_RTCP_SDES:
      {
        if (m_onSdes) m_onSdes(*(static_cast<rfc3550::RtcpSdes*>(compoundRtcp[i].get())), ep);
        break;
      }
      case rfc3550::PT_RTCP_APP:
      {
        if (m_onApp) m_onApp(*(static_cast<rfc3550::RtcpApp*>(compoundRtcp[i].get())), ep);
        break;
      }
      case rfc3550::PT_RTCP_BYE:
      {
        if (m_onBye) m_onBye(*(static_cast<rfc3550::RtcpBye*>(compoundRtcp[i].get())), ep);

        if (m_pFeedbackManager)
          m_pFeedbackManager->reset();

        break;
      }
      case rfc3611::PT_RTCP_XR:
      {
        doHandleXr(*(static_cast<rfc3611::RtcpXr*>(compoundRtcp[i].get())), ep);
      }
      case rfc4585::PT_RTCP_GENERIC_FEEDBACK:
      case rfc4585::PT_RTCP_PAYLOAD_SPECIFIC:
      {
        doHandleFb(*(static_cast<rfc4585::RtcpFb*>(compoundRtcp[i].get())), ep);
      }
    }
  }

  if (m_pScheduler)
  {
    m_pScheduler->onIncomingRtcp(compoundRtcp, ep);
  }
}

void RtpSessionManager::handleMemberUpdate(const MemberUpdate& memberUpdate)
{
  if (memberUpdate.isMemberJoin())
  {

  }
  else if (memberUpdate.isMemberUpdate())
  {
    VLOG(12) << "Updating path info";
    const PathInfo& info = memberUpdate.getPathInfo();
    m_pTxManager->updatePathChar(info);
    // This is the wrong place: on the sender?
#if 0
    // store last cumu loss for scream fb
    if (info.getFlowId() == -1)
      m_uiLastLoss = info.getLost(); // to avoid multipath context
#endif
  }
  else if (memberUpdate.isMemberLeave())
  {
    VLOG(2) << "Bye received.";
    if( m_bExitOnBye)
    {
      VLOG(10) << "Shutting down session.";
      stop();
      VLOG(10) << "Session stopped.";
      app::ApplicationUtil::handleApplicationExit(boost::system::error_code(), boost::shared_ptr<SimpleMediaSession>());
      VLOG(10) << "Session stop signal fired.";
    }
  }
}

void RtpSessionManager::prepareSampleForOutput(const std::vector<MediaSample>& vMediaSamples)
{
  if (m_mediaCallback)
  {
    m_mediaCallback(vMediaSamples);
  }
  else
  {
    boost::mutex::scoped_lock l(m_outgoingSampleLock);
    m_qOutgoing.push_back(vMediaSamples);
  }
}

void RtpSessionManager::onScheduledSampleTimeout(const boost::system::error_code& ec,
                                                 boost::asio::deadline_timer* pTimer)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // trigger sample callback
    RtpPacketGroup node = m_pReceiverBuffer->getNextPlayoutBufferNode();
    // call the appropriate depacketizer to aggregate the packets, do error concealment, etc
    std::vector<MediaSample> vSamples = m_pRtpSession->depacketise(node);
    if (!vSamples.empty())
      prepareSampleForOutput(vSamples);
    else
    {
      VLOG(5) << "Depacketise yielded 0 samples";
    }
  }

  {
    boost::mutex::scoped_lock l(m_timerLock);
    std::size_t uiRemoved = m_mPlayoutTimers.erase(pTimer);
    assert(uiRemoved == 1);
  }
  delete pTimer;

  triggerNotification();
}

void RtpSessionManager::onSessionStarted()
{
  VLOG(15) << "RTP session started";
  // in the new design, a transmission manager will always be created
  assert(m_pTxManager);
  // create scheduler: it can only be created once the session has been started as the number of flows is not know prior
  optional<uint32_t> uiScheduler = m_applicationParameters.getUintParameter(app::ApplicationParameters::scheduler);
  //optional<std::string> sSchedulingAlgorithm = m_applicationParameters.getStringParameter(app::ApplicationParameters::mprtp_sa);
  optional<std::string> sSchedulingAlgorithm = m_applicationParameters.getStringParameter(app::ApplicationParameters::scheduler_param);
  uint32_t uiType = uiScheduler ? *uiScheduler : 0;
  std::string sSchedulingParameter = sSchedulingAlgorithm ? *sSchedulingAlgorithm : "";
  auto pCooperative = m_pCooperative.lock();
  m_pScheduler = std::move(SchedulerFactory::create(m_pRtpSession, *m_pTxManager.get(), pCooperative.get(), uiType, m_rIoService, m_applicationParameters, m_pRtpSession->getNumberOfFlows(), sSchedulingParameter));
}

void RtpSessionManager::onSessionStopped()
{
  VLOG(15) << "RTP session stopped";
}

void RtpSessionManager::doHandleXr(const rfc3611::RtcpXr& xr, const EndPoint& ep)
{
  VLOG(2) << "Received XR from " << ep;
}

void RtpSessionManager::doHandleFb(const rfc4585::RtcpFb& fb, const EndPoint& ep)
{
  VLOG(6) << "Received FB from " << ep;
  uint8_t uiFormat = fb.getTypeSpecific();
  switch (uiFormat)
  {
    case rfc4585::TL_FB_GENERIC_NACK:
    {
      const rfc4585::RtcpGenericNack& nack = static_cast<const rfc4585::RtcpGenericNack&>(fb);
      std::vector<uint16_t> nacks = nack.getNacks();
      VLOG(10) << "Received generic NACK containing " << nacks.size();
      handleNacks(nack.getNacks(), ep);
      break;
    }
    case rfc4585::TL_FB_EXTENDED_NACK:
    {
      const rfc4585::RtcpExtendedNack& nack = static_cast<const rfc4585::RtcpExtendedNack&>(fb);
      uint16_t uiFlowId = nack.getFlowId();
      std::vector<uint16_t> nacks = nack.getNacks();
      VLOG(10) << "Received extended NACK on flow " << uiFlowId << " containing " << nacks.size();
      handleNacks(uiFlowId, nacks, ep);
      break;
    }
    case rfc4585::TL_FB_GENERIC_ACK:
    {
      const rfc4585::RtcpGenericAck& ack = static_cast<const rfc4585::RtcpGenericAck&>(fb);
      std::vector<uint16_t> acks = ack.getAcks();
      VLOG(10) << "Received generic ACK of size (" << acks.size() << ") containing SNs " << ::toString(acks);
      // TODO once RTX and feedback has moved to this layer
#if 1
      handleAcks(acks, ep);
#endif
      break;
    }
    // moved handling into scheduler
#if 0
    // issue here is that the jiffy is not together with the RR
    case experimental::TL_FB_SCREAM_JIFFY:
    {
      const experimental::RtcpScreamFb& screamFb = static_cast<const experimental::RtcpScreamFb&>(fb);
      VLOG(2) << "Received scream jiffy: SSRC: " << hex(screamFb.getSenderSSRC())
              << " jiffy: " <<  screamFb.getJiffy()
              << " highest SN: " << screamFb.getHighestSNReceived()
              << " lost: " << screamFb.getLost();

      // update scream scheduler
      ScreamScheduler* pScreamScheduler = static_cast<ScreamScheduler*>(m_pScheduler.get());
      pScreamScheduler->updateScreamParameters(screamFb.getSenderSSRC(), screamFb.getJiffy(), screamFb.getHighestSNReceived(), screamFb.getLost());
      break;
    }
#endif
    default:
    {
      LOG_FIRST_N(WARNING, 1) << "Unhandled generic feedback report type: " << (uint32_t)uiFormat;
    }
  }

  assert(m_pScheduler);
  m_pScheduler->processFeedback(fb, ep);
}

void RtpSessionManager::handleNacks(const std::vector<uint16_t>& nacks, const EndPoint &ep)
{
  VLOG(6) << "Received NACKS: " << ::toString(nacks);
  if (m_pTxManager)
    m_pTxManager->nack(nacks);
  for (uint16_t uiSN : nacks)
  {
    if (m_pTxManager)
    {
      boost::optional<RtpPacket> pRtpPacket = m_pTxManager->generateRetransmissionPacket(uiSN);
      if (pRtpPacket)
      {
        m_pScheduler->scheduleRtxPacket(*pRtpPacket);
      }
      else
      {
        VLOG(3) << "Couldn't find RTX " << uiSN << " in buffer";
      }
    }
    else
    {
      LOG_FIRST_N(WARNING, 1) << "NACK received, but no RTX manager configured";
      break;
    }
  }
}

void RtpSessionManager::handleNacks(uint16_t uiFlowId, const std::vector<uint16_t>& nacks, const EndPoint &ep)
{
  LOG(WARNING) << "RTX request for flow: " << uiFlowId << " FSSN: " << ::toString(nacks);
  for (uint16_t uiFSSN : nacks)
  {
    if (m_pTxManager)
    {
#if 1
      //  new approach: unified generateRetransmissionPacket method
      uint16_t uiSN = 0;
      if (m_pTxManager->lookupSequenceNumber(uiSN, uiFlowId, uiFSSN))
      {
        boost::optional<RtpPacket> pRtpPacket = m_pTxManager->generateRetransmissionPacket(uiSN);
        if (pRtpPacket)
        {
          m_pScheduler->scheduleRtxPacket(*pRtpPacket);
        }
        else
        {
          VLOG(3) << "Couldn't find RTX " << uiSN << " in buffer";
        }
      }
#else
      boost::optional<RtpPacket> pRtpPacket = m_pTxManager->generateRetransmissionPacket(uiFlowId, uiSN);
      if (pRtpPacket)
      {
        // TODO: this section still schedules packets directly: shouldn't we be using the scheduler?
        double dRtt = 0.0;
        uint16_t uiSubflowId = m_pRtpSession->findSubflowWithSmallestRtt(dRtt);
        VLOG(6) << "RTX request info flow: " << uiFlowId
                << " FSSN: " << uiSN
                << " New flow: " << uiSubflowId
                << " RTX SN: " << pRtpPacket->getSequenceNumber();
        m_pRtpSession->sendRtpPacket(*pRtpPacket, uiSubflowId);
      }
      else
      {
        VLOG(3) << "Couldn't find RTX " << uiSN << " in buffer";
      }
#endif
    }
    else
    {
      LOG_FIRST_N(WARNING, 1) << "NACK received, but no RTX manager configured";
      break;
    }
  }
}

void RtpSessionManager::handleAcks(const std::vector<uint16_t>& acks, const EndPoint &ep )
{
  // TODO: use the acks to determine which packets need to be transmitted
  // or use the ACK to adjust CC parameters
  VLOG(2) << "Received ACKS: " << ::toString(acks);
  if (m_pTxManager)
    m_pTxManager->ack(acks);
}

void RtpSessionManager::onPacketAssumedLost(uint16_t uiSN)
{
  // Single path RTP
  // for multipath we need to select the "better" path
  // TODO: once 3611 is implemented we should check if there is sufficient time for an RTX
  boost::posix_time::ptime tScheduled;
  uint32_t uiMs = 0;
  VLOG(2) << "Scheduling feedback report to retrieve lost packet " << uiSN;
  m_pRtpSession->tryScheduleEarlyFeedback(0, tScheduled, uiMs);
}

void RtpSessionManager::onPacketAssumedLostInFlow(uint16_t uiFlowId, uint16_t uiFSSN)
{
  // TODO: lookup which RTCP report manager is responsible for flow and use it to send feedback?
  // or use LRU?
  // or use flow with least OWD: only possible if receiver is aware of RTT
  boost::posix_time::ptime tScheduled;
  uint32_t uiMs = 0;
  VLOG(6) << "Scheduling feedback report for packet lost in flow: " << uiFlowId << " FSSN: " << uiFSSN;
  // TODO: if the RTCP reverse RTT is supported: the receiver can pick the quickest path
  m_pRtpSession->tryScheduleEarlyFeedback(uiFlowId, tScheduled, uiMs);
}

} // rtp_plus_plus
