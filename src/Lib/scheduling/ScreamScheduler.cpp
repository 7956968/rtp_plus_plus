#include "CorePch.h"
#include <rtp++/scheduling/ScreamScheduler.h>
#include <scream/code/RtpQueue.h>
#include <scream/code/ScreamTx.h>
#include <rtp++/experimental/ExperimentalRtcp.h>
#include <rtp++/experimental/Scream.h>

#define ENABLE_SCREAM_LOG true

namespace rtp_plus_plus
{

#ifdef ENABLE_SCREAM

ScreamScheduler::ScreamScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                                 ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                                 float fFps, float fMinBps, float fMaxBps)
  :RtpScheduler(pRtpSession),
    m_transmissionManager(transmissionManager),
    m_ioService(ioService),
    m_timer(m_ioService),
    m_pRtpQueue(new RtpQueue()),
    m_pScreamTx(new ScreamTx()),
    m_pCooperative(pCooperative),
    m_uiPreviousKbps(0),
    m_uiLastExtendedSNReported(0),
    m_uiLastLoss(0)
{
  m_tStart = boost::posix_time::microsec_clock::universal_time();

  float priority = 1.0f;
  m_uiSsrc = pRtpSession->getRtpSessionState().getSSRC();
  VLOG(2) << "Registering new scream stream: " << m_uiSsrc;
  m_pScreamTx->registerNewStream(m_pRtpQueue, m_uiSsrc, priority, fMinBps, fMaxBps, fFps);

  if (m_pCooperative)
  {
    VLOG(2) << "Cooperative interface has been set";
    float fTargetBitrate = m_pScreamTx->getTargetBitrate(m_uiSsrc);
    VLOG(2) << "### SCREAM ###: Scream target bitrate: " << fTargetBitrate << "bps";
    uint32_t uiKbps = static_cast<uint32_t>(fTargetBitrate/1000.0);
    m_pCooperative->setBitrate(uiKbps);
    m_uiPreviousKbps = uiKbps;
  }
}

ScreamScheduler::~ScreamScheduler()
{
  // could add stop method?
  m_timer.cancel();
  delete m_pScreamTx;
}

void ScreamScheduler::scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
{
  VLOG(12) << "Scheduling RTP packets: SSRC " << hex(m_uiSsrc);

  uint32_t ssrc = 0;
  void *rtpPacketDummy = 0;
  int size = 0;
  uint16_t seqNr = 0;
  float retVal = -1.0;

  static float time = 0.0;
  for (const RtpPacket& rtpPacket: vRtpPackets)
  {
    VLOG(12) << "Pushing RTP packet: SN: " << rtpPacket.getSequenceNumber() << " Size: " << rtpPacket.getSize();
    // from VideoEnc.cpp
    m_pRtpQueue->push(0, rtpPacket.getSize(), rtpPacket.getSequenceNumber(), time);
    // store in our own map
    m_mRtpPackets[rtpPacket.getSequenceNumber()] = rtpPacket;
  }
  // FIXME: HACK for now
  time += 0.03;

  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint64_t uiTimeUs = (tNow - m_tStart).total_microseconds();
  uint32_t uiTotalBytes = std::accumulate(vRtpPackets.begin(), vRtpPackets.end(), 0, [](int sum, const RtpPacket& rtpPacket){ return sum + rtpPacket.getSize(); });

  if (m_pCooperative)
  {
    float fTargetBitrate = m_pScreamTx->getTargetBitrate(m_uiSsrc);
    VLOG(2) << "### SCREAM ###: Scream target bitrate: " << fTargetBitrate << "bps";
    uint32_t uiKbps = static_cast<uint32_t>(fTargetBitrate/1000.0);

    if (uiKbps != m_uiPreviousKbps)
      m_pCooperative->setBitrate(uiKbps);
  }

  VLOG(2) << "### SCREAM ###: Calling new media frame: time_us: " << uiTimeUs << " SSRC: " << m_uiSsrc << " total bytes: " << uiTotalBytes;
  m_pScreamTx->newMediaFrame(uiTimeUs, m_uiSsrc, uiTotalBytes);
  retVal = m_pScreamTx->isOkToTransmit(uiTimeUs, ssrc);

  if (ENABLE_SCREAM_LOG)
  {
    LOG(INFO) << m_pScreamTx->printLog(uiTimeUs);
  }

  VLOG_IF(12, retVal != -1) << "### SCREAM ###: isOkToTransmit: " << retVal << " ssrc: " << ssrc;

  while (retVal == 0)
  {
    /*
     * RTP packet can be transmitted
     */
    if (m_pRtpQueue->sendPacket(rtpPacketDummy, size, seqNr))
    {
      VLOG(12) << "Retrieved packet from queue: SN: " << seqNr << " size: " << size;
      // netQueueRate->insert(time,rtpPacket, ssrc, size, seqNr);
      RtpPacket rtpPacket = m_mRtpPackets[seqNr];
      m_mRtpPackets.erase(seqNr);

      VLOG(2) << "Sending RTP packet in session: SN: " << rtpPacket.getSequenceNumber() << " Size: " << rtpPacket.getSize();
      m_pRtpSession->sendRtpPacket(rtpPacket);
      VLOG(2) << "### SCREAM ###: time_us: " << uiTimeUs << " ssrc: " << ssrc << " size: " << size << " seqNr: "  << seqNr;
      retVal = m_pScreamTx->addTransmitted(uiTimeUs, ssrc, size, seqNr);
      VLOG_IF(12, retVal > 0.0) << "### SCREAM ###: addTransmitted: " << retVal;
    }
    else
    {
      VLOG(12) << "Failed to retrieve rtp packet from RTP queue";
      retVal = -1.0;
    }
  }

  if (retVal > 0)
  {
    int milliseconds = (int)(retVal * 1000);
    VLOG(12) << "Starting timer in : " << milliseconds << " ms";
    m_timer.expires_from_now(boost::posix_time::milliseconds(milliseconds));
    m_timer.async_wait(boost::bind(&ScreamScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
  }
}

void ScreamScheduler::onScheduleTimeout(const boost::system::error_code& ec )
{
  if (!ec)
  {
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    uint64_t uiTimeUs = (tNow - m_tStart).total_microseconds();

    uint32_t ssrc = 0;
    float retVal = m_pScreamTx->isOkToTransmit(uiTimeUs, ssrc);
    VLOG(12) << "### SCREAM ###: isOkToTransmit: " << retVal << " ssrc: " << ssrc;

    while (retVal == 0.0)
    {
      /*
       * RTP packet can be transmitted
       */
      void *rtpPacketDummy = 0;
      int size = 0;
      uint16_t seqNr = 0;
      if (m_pRtpQueue->sendPacket(rtpPacketDummy, size, seqNr))
      {
        VLOG(12) << "### SCREAM ###: Retrieved packet from queue: SN: " << seqNr << " size: " << size;
        // netQueueRate->insert(time,rtpPacket, ssrc, size, seqNr);
        RtpPacket rtpPacket = m_mRtpPackets[seqNr];
        m_mRtpPackets.erase(seqNr);

        VLOG(2) << "ScreamScheduler::onScheduleTimeout Sending RTP packet in session: SN: " << rtpPacket.getSequenceNumber() << " Size: " << rtpPacket.getSize();
        m_pRtpSession->sendRtpPacket(rtpPacket);
        retVal = m_pScreamTx->addTransmitted(uiTimeUs, ssrc, size, seqNr);
        VLOG(12) << "### SCREAM ###: ScreamScheduler::onScheduleTimeout addTransmitted: " << retVal;

        if (ENABLE_SCREAM_LOG)
        {
          LOG(INFO) << m_pScreamTx->printLog(uiTimeUs);
        }
      }
      else
      {
        VLOG(2) << "Failed to retrieve rtp packet from RTP queue";
        retVal = -1.0;
      }

    }

    if (retVal > 0.0)
    {
      int milliseconds = (int)(retVal * 1000);
      VLOG(12) << "Starting timer in : " << milliseconds << " ms";
      m_timer.expires_from_now(boost::posix_time::milliseconds(milliseconds));
      m_timer.async_wait(boost::bind(&ScreamScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
    }
  }
}

void ScreamScheduler::scheduleRtxPacket(const RtpPacket& rtpPacket)
{

}

void ScreamScheduler::shutdown()
{

}

void ScreamScheduler::updateScreamParameters(uint32_t uiSSRC, uint32_t uiJiffy, uint32_t uiHighestSNReceived, uint32_t uiCumuLoss)
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint64_t uiTimeUs = (tNow - m_tStart).total_microseconds();

  LOG(INFO) << "### SCREAM ###" << "time_us: " << uiTimeUs
            << " SSRC: " << uiSSRC
            << " Jiffy: " << uiJiffy
            << " highest SN: " << uiHighestSNReceived
            << " loss: " << uiCumuLoss;

  m_pScreamTx->incomingFeedback(uiTimeUs, uiSSRC, uiJiffy, uiHighestSNReceived, uiCumuLoss, false);
  float retVal = m_pScreamTx->isOkToTransmit(uiTimeUs, uiSSRC);

  if (ENABLE_SCREAM_LOG)
  {
    LOG(INFO) << m_pScreamTx->printLog(uiTimeUs);
  }

  // TODO: do we need to cancel an existing timer here?
  if (retVal > 0.0)
  {
    int milliseconds = (int)(retVal * 1000);
    VLOG(12) << "### SCREAM ###: Starting timer in : " << milliseconds << " ms";
    m_timer.expires_from_now(boost::posix_time::milliseconds(milliseconds));
    m_timer.async_wait(boost::bind(&ScreamScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
  }
}

void ScreamScheduler::processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep)
{
  if (fb.getTypeSpecific() == experimental::TL_FB_SCREAM_JIFFY)
  {
    const experimental::RtcpScreamFb& screamFb = static_cast<const experimental::RtcpScreamFb&>(fb);
    VLOG(2) << "Received scream jiffy: SSRC: " << hex(screamFb.getSenderSSRC())
            << " jiffy: " <<  screamFb.getJiffy()
            << " highest SN: " << screamFb.getHighestSNReceived()
            << " lost: " << screamFb.getLost();

    updateScreamParameters(screamFb.getSenderSSRC(), screamFb.getJiffy(), screamFb.getHighestSNReceived(), screamFb.getLost());
  }
}

std::vector<rfc4585::RtcpFb::ptr> ScreamScheduler::retrieveFeedback()
{
  std::vector<rfc4585::RtcpFb::ptr> feedback;
  // add SCReaAM feedback
  uint32_t uiHighestSN = m_transmissionManager.getLastReceivedExtendedSN();
  VLOG(2) << "ScreamScheduler::retrieveFeedback: highest SN: " << uiHighestSN << " last reported: " << m_uiLastExtendedSNReported;
  if (m_uiLastExtendedSNReported != uiHighestSN) // only report if there is new info
  {
    VLOG(2) << "RtpSessionManager::onFeedbackGeneration: Reporting new scream info";
    m_uiLastExtendedSNReported = uiHighestSN;
    boost::posix_time::ptime tLastTime = m_transmissionManager.getPacketArrivalTime(uiHighestSN);
    boost::posix_time::ptime tInitial =  m_transmissionManager.getInitialPacketArrivalTime();
    uint32_t uiJiffy = (tLastTime - tInitial).total_milliseconds();
    m_uiLastLoss = m_pRtpSession->getCumulativeLostAsReceiver();
    RtpSessionState& state = m_pRtpSession->getRtpSessionState();
    experimental::RtcpScreamFb::ptr pFb = experimental::RtcpScreamFb::create(uiJiffy, m_uiLastExtendedSNReported, m_uiLastLoss, state.getRemoteSSRC(), state.getSSRC());
    feedback.push_back(pFb);
  }
  return feedback;
}

#endif
} // rtp_plus_plus
