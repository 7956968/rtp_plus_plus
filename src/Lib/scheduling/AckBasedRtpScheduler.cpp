#include "CorePch.h"
#include <cpputil/StringTokenizer.h>
#include <rtp++/scheduling/AckBasedRtpScheduler.h>
#include <rtp++/experimental/GoogleRemb.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/experimental/ExperimentalRtcp.h>

namespace rtp_plus_plus
{

const uint32_t OWD_TARGET_LO_MS = 100;
const uint32_t OWD_TARGET_HI_MS = 400;
const double MAX_BYTES_IN_FLIGHT_HEAD_ROOM = 1.1;
const double BETA = 0.6;
const double BETA_R = 0.8;
const uint32_t BYTES_IN_FLIGHT_SLACK = 10;
const uint32_t TARGET_BITRATE_MIN = 100;
const uint32_t TARGET_BITRATE_MAX = 2000;
const uint32_t RAMP_UP_TIME_S = 10;
const double PRE_CONGESTION_GUARD = 0.0;
const double TX_QUEUE_SIZE_FACTOR = 0.0;

const uint32_t CWND_MAX = 400000;
const uint32_t MAX_INIT_CWND = 50;

const uint32_t MIN_INTERVAL_BETWEEN_CCB_MEASUREMENTS_MS = 1000;

AckBasedRtpScheduler::AckBasedRtpScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                                           ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                                           double dFps, uint32_t uiMinKbps, uint32_t uiMaxKps,
                                           const std::string& sSchedulingParameter)
  :RtpScheduler(pRtpSession),
    m_transmissionManager(transmissionManager),
    m_ioService(ioService),
    m_pCooperative(pCooperative),
    m_timer(m_ioService),
    m_dFps(dFps),
    m_uiMinKbps(uiMinKbps),
    m_uiMaxKps(uiMaxKps),
    m_bRembEnabled(false),
    m_eMode(TM_FAST_START),
    m_uiUnsentBytes(0),
    m_uiBytesUnacked(0),
    m_uiBytesNewlyAcked(0),
    m_uiHighestSNAcked(0),
    m_uiAckCount(0),
    m_uiMinIntervalBetweenMeasurementsMs(MIN_INTERVAL_BETWEEN_CCB_MEASUREMENTS_MS),
    m_timerCcb(ioService),
    m_uiBytesIncomingSinceLastMeasurement(0),
    m_uiBytesOutgoingSinceLastMeasurement(0),
    m_uiRtxBytesOutgoingSinceLastMeasurement(0),
    m_dIncomingRateCcb(0.0),
    m_dOutgoingRateCcb(0.0),
    m_dOutgoingRtxRateCcb(0.0),
    owd_target(OWD_TARGET_LO_MS),
    owd_fraction_avg(0.0),
    owd_trend(0.0),
    mss(1400),
    min_cwnd(2*mss),
    cwnd(min_cwnd),
    cwnd_i(1),
    bytes_newly_acked(0),
    send_wnd(0),
    t_pace_ms(1),
    target_bitrate(TARGET_BITRATE_MIN),
    target_bitrate_i(1),
    rate_transmit(0.0),
    rate_acked(0.0),
    s_rtt(0.0),
    beta(BETA)
{
  // processing ACKS directly from feedback now
  if (m_pCooperative)
  {
    VLOG(2) << "Cooperative interface has been set";
    VLOG(2) << "### ACK ###: target bitrate: " << uiMinKbps << "bps";
    m_pCooperative->setBitrate(uiMinKbps);
    // m_uiPreviousKbps = uiKbps;
  }

  if (experimental::supportsReceiverEstimatedMaximumBitrate(pRtpSession->getRtpSessionParameters()))
  {
    VLOG(2) << "GOOG-REMB support enabled";
    m_bRembEnabled = true;
  }
  else
  {
    VLOG(2) << "GOOG-REMB support not enabled";
  }

  // parse dynamic parameters
  std::vector<std::string> vParams = StringTokenizer::tokenize( sSchedulingParameter, ",", true, true);
  for (auto sParam : vParams)
  {
    std::vector<std::string> values = StringTokenizer::tokenize( sParam, "=", true, true);
    if (values.size() == 2)
    {
      bool bRes;
      std::string& sName(values[0]);
      if ( sName == "cwnd")
      {
        uint32_t uiCwnd = convert<int32_t>(values[1], bRes);
        assert(bRes);
        if (uiCwnd > 0 && uiCwnd < MAX_INIT_CWND)
        {
          cwnd = uiCwnd * mss;
          LOG(WARNING) << "Init cwnd: " << cwnd << " n*mss: " << uiCwnd << "*" << mss;
        }
        else
        {
          LOG(WARNING) << "Invalid init cwnd: " << uiCwnd;
        }
      }
      else if ( sName == "beta")
      {
        double dBeta = convert<double>(values[1], bRes);
        assert(bRes);
        if (dBeta > 0.0 && dBeta < 1.0)
        {
          beta = dBeta;
          LOG(WARNING) << "BETA: " << beta;
        }
        else
        {
          LOG(WARNING) << "Invalid beta: " << beta;
        }
      }
    }
    else
    {
      LOG(WARNING) << "Invalid schedling parameter";
    }
  }

  // no pacing in progress, start the timer
  m_tLastCcbMeasurement = boost::posix_time::microsec_clock::universal_time();
  m_timerCcb.expires_from_now(boost::posix_time::milliseconds(m_uiMinIntervalBetweenMeasurementsMs)); 
  m_timerCcb.async_wait(boost::bind(&AckBasedRtpScheduler::onCcbEvalTimeout, this, boost::asio::placeholders::error));
}

AckBasedRtpScheduler::~AckBasedRtpScheduler()
{
  // could add stop method?
  m_timer.cancel();

  VLOG(2) << "~AckBasedRtpScheduler: packets still in buffer: " << m_qRtpPackets.size()
          << " bytes unsent: " << m_uiUnsentBytes
          << " bytes unacked: " << m_uiBytesUnacked;
}

void AckBasedRtpScheduler::shutdown()
{
  m_timerCcb.cancel();
}

void AckBasedRtpScheduler::scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
{
  assert(!vRtpPackets.empty());
  // for now we assume a framerate of 25fps.
  // later we can measure it, or set it via parameter
  // if no send is outstanding, send it immediately
  doScheduleRtpPackets(vRtpPackets);
}

void AckBasedRtpScheduler::sendPacketsIfPossible()
{
  auto it = m_qRtpPackets.begin();
  bool bMorePackets = it != m_qRtpPackets.end();

  if (!bMorePackets) return;

  bool bSpaceInCwnd = (m_uiBytesUnacked + it->getSize()) < (MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd);

  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  bool bLastTransmissionWasMoreThan200MsAgo = m_tLastTx.is_not_a_date_time() ||
      (tNow - m_tLastTx).total_milliseconds() >= 200;
  while (bMorePackets && (bSpaceInCwnd || bLastTransmissionWasMoreThan200MsAgo))
  {
    // our RTCP RTP header ext inserts extra bytes into the packet before sending it-
    // as a result, the size added and subtracted from cwnd differ. Using payload size instead
#if 0
    uint32_t uiSize = it->getSize();
#else
    uint32_t uiSize = it->getPayloadSize();
#endif
#if 1
    // Debugging
    VLOG(2) << "#ACK: Adding bytes unacked - SN: " << it->getSequenceNumber() << " Size: " << uiSize;
#endif
    m_uiBytesUnacked += uiSize;
    m_uiBytesOutgoingSinceLastMeasurement += uiSize;
    m_uiUnsentBytes -= uiSize;
    VLOG(2) << "Sending packet " << it->getSequenceNumber() << " size: " << uiSize
            << " CCB size after send: " << m_uiUnsentBytes
            << " unacked after send: " << m_uiBytesUnacked
            << " cwnd*headroom: " << MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd
            << " last tx > 200ms: " << bLastTransmissionWasMoreThan200MsAgo;
    m_pRtpSession->sendRtpPacket(*it);
    m_qUnacked.push_back(it->getSequenceNumber());
    ++it;
    m_tLastTx = tNow;
    bLastTransmissionWasMoreThan200MsAgo = false;

    bMorePackets = it != m_qRtpPackets.end();
    if (bMorePackets) bSpaceInCwnd = (m_uiBytesUnacked + it->getSize()) < (MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd);
  }

  // removed sent packets from queue
  m_qRtpPackets.erase(m_qRtpPackets.begin(), it);
}

void AckBasedRtpScheduler::doScheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
{
  uint32_t uiTotalBytes = std::accumulate(vRtpPackets.begin(), vRtpPackets.end(), 0, [](int sum, const RtpPacket& rtpPacket){ return sum + rtpPacket.getSize(); });
  m_uiUnsentBytes += uiTotalBytes;
  m_uiBytesIncomingSinceLastMeasurement += uiTotalBytes;

  // insert packets into congestion control buffer (CCB)
  m_qRtpPackets.insert(m_qRtpPackets.end(), vRtpPackets.begin(), vRtpPackets.end());

#if 1
  sendPacketsIfPossible();

#else
  auto it = m_qRtpPackets.begin();
  while (it != m_qRtpPackets.end() && ((m_uiBytesUnacked + it->getSize()) < (MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd)))
  {
    VLOG(2) << "Sending packet size: " << it->getSize() << " unacked: " << m_uiBytesUnacked << " cwnd*headroom: " << MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd;
    m_pRtpSession->sendRtpPacket(*it);
    m_uiBytesUnacked += it->getSize();
    ++it;
  }
#endif
}

void AckBasedRtpScheduler::onScheduleTimeout(const boost::system::error_code& ec )
{
  if (!ec)
  {
    auto it = m_qRtpPackets.begin();
    m_pRtpSession->sendRtpPacket(*it);
    m_qRtpPackets.pop_front();
    if (!m_qRtpPackets.empty())
    {
      // no pacing in progress, start the timer
      m_timer.expires_from_now(boost::posix_time::milliseconds(10)); // use fixed timer for now: should actually be adaptive
      m_timer.async_wait(boost::bind(&AckBasedRtpScheduler::onScheduleTimeout, this, boost::asio::placeholders::error));
    }
  }
}

void AckBasedRtpScheduler::onCcbEvalTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    // measure time
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    uint64_t uiDiffMs = (tNow - m_tLastCcbMeasurement).total_milliseconds();

    // eval incoming data rate
    m_dIncomingRateCcb = (m_uiBytesIncomingSinceLastMeasurement * 8 /*/ 1000)*(1000*/ / uiDiffMs); // the kbps mult and ms cancel each other out
    m_uiBytesIncomingSinceLastMeasurement = 0;
    // eval outgoing data rate
    m_dOutgoingRateCcb = (m_uiBytesOutgoingSinceLastMeasurement * 8 /*/ 1000)*(1000*/ / uiDiffMs); // the kbps mult and ms cancel each other out
    m_uiBytesOutgoingSinceLastMeasurement = 0;
    // RTX rate
    m_dOutgoingRtxRateCcb = (m_uiRtxBytesOutgoingSinceLastMeasurement * 8 /*/ 1000)*(1000*/ / uiDiffMs); // the kbps mult and ms cancel each other out
    m_uiRtxBytesOutgoingSinceLastMeasurement = 0;

    VLOG(2) << "CCB rate estimation: incoming " << m_dIncomingRateCcb << " kbps"
      << " outgoing: " << m_dOutgoingRateCcb << " kbps"
      << " outgoing (RTX): " << m_dOutgoingRtxRateCcb << " kbps";

    m_tLastCcbMeasurement = tNow;
    // schedule next check
    m_timerCcb.expires_from_now(boost::posix_time::milliseconds(m_uiMinIntervalBetweenMeasurementsMs));
    m_timerCcb.async_wait(boost::bind(&AckBasedRtpScheduler::onCcbEvalTimeout, this, boost::asio::placeholders::error));
  }
}

void AckBasedRtpScheduler::scheduleRtxPacket(const RtpPacket& rtpPacket)
{
#if 0
  // push it in the front
  m_qRtpPackets.push_front(rtpPacket);
  sendPacketsIfPossible();
#else
  m_uiRtxBytesOutgoingSinceLastMeasurement += rtpPacket.getSize();

  // send immediately
  VLOG(2) << "Resending packet size: " << rtpPacket.getSize()
          << " unacked: " << m_uiBytesUnacked
          << " cwnd*headroom: " << MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd;
  m_pRtpSession->sendRtpPacket(rtpPacket);
#endif
}

void AckBasedRtpScheduler::processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep)
{
  if (fb.getTypeSpecific() == rfc4585::TL_FB_GENERIC_NACK)
  {
    const rfc4585::RtcpGenericNack& nackFb = static_cast<const rfc4585::RtcpGenericNack&>(fb);
    std::vector<uint16_t> nacks = nackFb.getNacks();
    VLOG(10) << "Received generic NACK containing " << nacks.size();
    nack(nacks);
  }
  else if (fb.getTypeSpecific() == rfc4585::TL_FB_GENERIC_ACK)
  {
    const rfc4585::RtcpGenericAck& ackFb = static_cast<const rfc4585::RtcpGenericAck&>(fb);
    ack(ackFb.getAcks());
  }
  else if (fb.getTypeSpecific() == rfc4585::FMT_APPLICATION_LAYER_FEEDBACK)
  {
    VLOG(2) << "Application layer feedback received";
    const rfc4585::RtcpApplicationLayerFeedback& appLayerFb = static_cast<const rfc4585::RtcpApplicationLayerFeedback&>(fb);
    boost::optional<experimental::RtcpRembFb> remb = experimental::RtcpRembFb::parseFromAppData(appLayerFb.getAppData());
    if (remb)
    {
      VLOG(2) << "Receiver estimated bitrate: " << remb->getBps()/1000.0 << "kbps";
    }
  }
  else
  {
    LOG_FIRST_N(INFO, 2) << "Unhandled RTCP feedback received: " << fb.getTypeSpecific();
  }
}

void AckBasedRtpScheduler::ack(const std::vector<uint16_t>& acks)
{
  // lookup sizes
  for (uint16_t uiSN : acks)
  {
    if (m_uiAckCount == 0 )
    {
      m_uiHighestSNAcked = uiSN;
    }
    else
    {
      uint32_t uiDiff = uiSN - m_uiHighestSNAcked;
      if (uiDiff > 0 && uiDiff < (UINT32_MAX >> 1)) // TODO: handle rollover
      {
        m_uiHighestSNAcked = uiSN;
      }
    }

    TransmissionManager::RtpPacketInfo* pInfo = m_transmissionManager.lookupRtpPacketInfo(uiSN);
    if (pInfo != nullptr)
    {
      if (!pInfo->bProcessedByScheduler)
      {
        uint32_t uiSize = pInfo->rtpPayloadSize;
  #if 1
      VLOG(2) << "#ACK: Subtracting bytes unacked - SN: " << pInfo->originalRtpPacket.getSequenceNumber() << " Size: " << uiSize;
  #endif
        m_uiBytesUnacked -= uiSize;
        m_uiBytesNewlyAcked += uiSize;
        if (cwnd < CWND_MAX)
          cwnd += mss;
        pInfo->bProcessedByScheduler = true;

        // check in ack vector
        for (auto it = m_qUnacked.begin(); it != m_qUnacked.end(); ++it)
        {
          if (*it == uiSN)
          {
            m_qUnacked.erase(it);

            // check if we sent an RTX previously
            auto it_rtx = m_mRtxInfo.find(uiSN);
            if (it_rtx != m_mRtxInfo.end())
            {
              boost::posix_time::ptime& tLastRtx = it_rtx->second;
              boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
              VLOG(2) << "Rtx of " << uiSN << " took " << (tNow - tLastRtx).total_milliseconds() << " ms";
              m_mRtxInfo.erase(it_rtx);
            }
            break;
          }
        }
        ++m_uiAckCount;
      }
      else
      {
        // previously processed by scheduler
      }
    }
  }

  for (auto it = m_qUnacked.begin(); it != m_qUnacked.end(); ++it)
  {
    uint32_t uiUnackedSN = *it;
    uint32_t uiDiff = m_uiHighestSNAcked - uiUnackedSN;
    if (uiDiff < (UINT32_MAX >> 1))
    {
      // lost or unacked
      auto it_unacked = m_mRtxInfo.find(uiUnackedSN);
      if (it_unacked == m_mRtxInfo.end())
      {
        // approach retransmission
        VLOG(2) << "Packet unacked " << uiUnackedSN << " Sending retransmission";
        boost::optional<RtpPacket> pRtpPacket = m_transmissionManager.generateRetransmissionPacket(uiUnackedSN);
        if (pRtpPacket)
        {
          boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
          m_mRtxInfo[uiUnackedSN]= tNow;
          scheduleRtxPacket(*pRtpPacket);
        }
        else
        {
          // this is not possible if packets aren't removed on timeout
          assert(false);
        }

        // decrease window
        VLOG(2) << "Packet " << uiUnackedSN << " unacked. Decreasing cwmd";
        cwnd *= beta;
      }
      else
      {
        // retransmission sent previously, see if more than one RTT has expired
        boost::posix_time::ptime& tLastRtx = it_unacked->second;
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        const int MINRTX_MS = 200;
        if ((tNow - tLastRtx).total_milliseconds() >= MINRTX_MS) // TODO: replace const with RTT-related
        {
          boost::optional<RtpPacket> pRtpPacket = m_transmissionManager.generateRetransmissionPacket(uiUnackedSN);
          if (pRtpPacket)
          {
            m_mRtxInfo[uiUnackedSN] = tNow;
            scheduleRtxPacket(*pRtpPacket);
          }
          else
          {
            // this is not possible if packets aren't removed on timeout
            assert(false);
          }
        }
        else
        {
          VLOG(6) << "Already sent RTX: waiting for RTX";
        }
      }
    }
    else
    {

    }
  }
#if 0
  m_qLost.insert(m_qLost.end(), vLost.begin(), vLost.end());

  for (uint16_t uiUnackedSN: vLost)
  {
    uint32_t uiCount = std::count_if(m_qLost.begin(), m_qLost.end(), [uiUnackedSN](uint16_t uiSN2){
        return uiSN2 = uiUnackedSN;
    });

    const int MAX_LOSS_COUNT = 1;
    if (uiCount >= MAX_LOSS_COUNT)
    {
      VLOG(2) << "Packet " << uiUnackedSN << " unacked " << MAX_LOSS_COUNT << " times. Decreasing cwmd";
      m_qLost.erase(std::remove_if(m_qLost.begin(), m_qLost.end(), [uiUnackedSN](uint16_t uiSN2){
                      return uiSN2 = uiUnackedSN;
                    }));

#if 1
      auto it_unacked = m_mRtxInfo.find(uiUnackedSN);
      if (it_unacked == m_mRtxInfo.end())
      {
        // approach retransmission
        VLOG(2) << "Packet unacked " << uiUnackedSN << " Sending retransmission";
        boost::optional<RtpPacket> pRtpPacket = m_transmissionManager.generateRetransmissionPacket(uiUnackedSN);
        if (pRtpPacket)
        {
          boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
          m_mRtxInfo[uiUnackedSN]= tNow;
          scheduleRtxPacket(*pRtpPacket);
        }
        else
        {
          // this is not possible if packets aren't removed on timeout
          assert(false);
        }
      }
      else
      {
        // retransmission sent previously, see if more than one RTT has expired
        boost::posix_time::ptime& tLastRtx = it_unacked->second;
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        const int MINRTX_MS = 200;
        if ((tNow - tLastRtx).total_milliseconds() >= MINRTX_MS) // TODO: replace const with RTT-related
        {
          boost::optional<RtpPacket> pRtpPacket = m_transmissionManager.generateRetransmissionPacket(uiUnackedSN);
          if (pRtpPacket)
          {
            m_mRtxInfo[uiUnackedSN] = tNow;
            scheduleRtxPacket(*pRtpPacket);
          }
          else
          {
            // this is not possible if packets aren't removed on timeout
            assert(false);
          }
        }
        else
        {
          VLOG(2) << "Already sent RTX: waiting for RTX";
        }
      }

#else
      // approach: remove from unacked
      for (auto it_unacked = m_qUnacked.begin(); it_unacked != m_qUnacked.end(); ++it_unacked )
      {
        if (*it_unacked == uiUnackedSN)
        {
          m_qUnacked.erase(it_unacked);
          break;
        }
      }
#endif
      ++uiLost;
    }
  }

  if (uiLost > 0)
  {
    // decrease window
    cwnd *= BETA;
  }
#if 0
  owd_fraction = owd/owd_target;
  owd_fraction_avg = 0.9* owd_fraction_avg + 0.1* owd_fraction;
  owd_fraction_hist.push_back(owd_fraction);
#endif

#if 1
    sendPacketsIfPossible();
#else
  // check if we can now send any other new packets
  auto it = m_qRtpPackets.begin();
  while (it != m_qRtpPackets.end() && ((m_uiBytesUnacked + it->getSize()) < (MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd)))
  {
    VLOG(2) << "Sending packet size: " << it->getSize() << " unacked: " << m_uiBytesUnacked << " cwnd*headroom: " << MAX_BYTES_IN_FLIGHT_HEAD_ROOM * cwnd;
    m_pRtpSession->sendRtpPacket(*it);
    ++it;
  }
#endif

  // end TODO
#endif
}

void AckBasedRtpScheduler::nack(const std::vector<uint16_t>& nacks)
{

}

std::vector<rfc4585::RtcpFb::ptr> AckBasedRtpScheduler::retrieveFeedback()
{
  VLOG(12) << "AckBasedRtpScheduler::retrieveFeedback()";
  std::vector<rfc4585::RtcpFb::ptr> feedback;
  if (m_bRembEnabled)
  {
    // add REMB feedback
    const RtpSessionStatistics& stats = m_pRtpSession->getRtpSessionStatistics();

    boost::posix_time::ptime tEstimate;
    uint32_t uiCurrentReceiveRateKbps = stats.getLastIncomingRtpRateEstimate(tEstimate);
    VLOG(12) << "AckBasedRtpScheduler::retrieveFeedback: Receive rate estimate: " << uiCurrentReceiveRateKbps << "kbps";
    if (tEstimate != m_tPreviousEstimate)
    {
      VLOG(2) << "AckBasedRtpScheduler::retrieveFeedback: Receive rate estimate: " << uiCurrentReceiveRateKbps << "kbps - sending feedback";
      m_tPreviousEstimate = tEstimate;
      RtpSessionState& state = m_pRtpSession->getRtpSessionState();
      experimental::RtcpRembFb remb(uiCurrentReceiveRateKbps*1000, 1, 0);
      rfc4585::RtcpApplicationLayerFeedback::ptr pFb = rfc4585::RtcpApplicationLayerFeedback::create(remb.toString(), state.getRemoteSSRC(), 0);
      feedback.push_back(pFb);
    }
  }
  return feedback;
}

} // rtp_plus_plus
