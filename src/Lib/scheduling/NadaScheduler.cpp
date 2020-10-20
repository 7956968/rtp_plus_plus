#include "CorePch.h"
#include <rtp++/scheduling/NadaScheduler.h>
#include <rtp++/experimental/Nada.h>

// log window for calculating outgoing sending rate
#define LOGWIN_ms 500
#define TWENTY_KBPS 20

namespace rtp_plus_plus
{

NadaScheduler::NadaScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                                 ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                                 float fFps, float fMinBps, float fMaxBps)
  :RtpScheduler(pRtpSession),
    m_transmissionManager(transmissionManager),
    m_nadaTx(transmissionManager, static_cast<uint32_t>(fFps), static_cast<uint32_t>(fMinBps/1000), static_cast<uint32_t>(fMaxBps/1000)),
    m_ioService(ioService),
    m_timer(m_ioService),
    m_pCooperative(pCooperative),
    m_uiPreviousKbps(0),
    m_uiMaxOvershootKbps(TWENTY_KBPS),
    m_uiBufferLengthBytes(0),
    m_uiEncoderRateKbps(static_cast<uint32_t>(fMinBps/1000)),
    m_uiNetworkRateKbps(static_cast<uint32_t>(fMinBps/1000))
{
  m_tStart = boost::posix_time::microsec_clock::universal_time();

  if (m_pCooperative)
  {
    VLOG(2) << "Cooperative interface has been set";
    float fTargetBitrate = fMinBps;
    uint32_t uiKbps = static_cast<uint32_t>(fTargetBitrate/1000.0);
    VLOG(2) << "### NADA ###: Nada target bitrate: " << uiKbps << "kbps";
    m_pCooperative->setBitrate(uiKbps);
    m_uiPreviousKbps = uiKbps;
  }
}

NadaScheduler::~NadaScheduler()
{
  // could add stop method?
  m_timer.cancel();
}

void NadaScheduler::scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets)
{
  VLOG(12) << "Scheduling RTP packets: SSRC " << hex(m_uiSsrc);
#if 1
  uint32_t uiTotalBytes = std::accumulate(vRtpPackets.begin(), vRtpPackets.end(), 0, [](int sum, const RtpPacket& rtpPacket){ return sum + rtpPacket.getSize(); });
  m_uiBufferLengthBytes += uiTotalBytes;
  // insert packets into congestion control buffer (CCB)
  m_qRtpPackets.insert(m_qRtpPackets.end(), vRtpPackets.begin(), vRtpPackets.end());
  sendPacketsIfPossible();
#else
  //TEST code to just send at encoder rate: no network rate adjustment
  for (const RtpPacket& rtpPacket: vRtpPackets)
  {
    m_pRtpSession->sendRtpPacket(rtpPacket);
  }

#endif
}

void NadaScheduler::updateOutgoingRateQueue(const boost::posix_time::ptime& tNow)
{
  boost::posix_time::ptime tStartOfLogWin = tNow - boost::posix_time::milliseconds(LOGWIN_ms);
  for (auto it = m_qRecentlySendPackets.begin(); it != m_qRecentlySendPackets.end(); /**/)
  {
   if (it->getSendTime() < tStartOfLogWin)
   {
     it = m_qRecentlySendPackets.erase(it); // c++11 erase
   }
   else
   {
     // ++it;
     break; // we can break early here since we can assume that the packets are sorted by arrival time
   }
  }
}

inline uint32_t NadaScheduler::calculateSendRateKbps(const boost::posix_time::ptime& tNow, uint32_t& uiTotalBytesSentInWindow, uint32_t& uiDurationMs)
{
  if (m_qRecentlySendPackets.empty()) return 0;
  uiTotalBytesSentInWindow = std::accumulate(m_qRecentlySendPackets.begin(),
                                                          m_qRecentlySendPackets.end(),
                                                          0,
                                                          [](int iSum, const RtpPacket& rtpPacket)
  {
    return iSum + rtpPacket.getSize();
  });
  // when using the actual duration of the packets in the window, the value is inaccurate for small packet windows
  // e.g. when starting to stream. Just using window size as duration even though this might not be 100% accurate when the window
  // is full
  // automatic conversion to kbit by dividing by ms
#if 0
  uiDurationMs = (tNow - m_qRecentlySendPackets.begin()->getSendTime()).total_milliseconds();
#else
  uiDurationMs = LOGWIN_ms;
#endif
  // fix for div 0 when the first set of RTP packets is sent
  if (uiDurationMs == 0) uiDurationMs = 1;
  uint32_t uiKbps = (uiTotalBytesSentInWindow * 8)/uiDurationMs;
  return uiKbps;
}

bool NadaScheduler::canSendPacket(const RtpPacket& rtpPacket, uint32_t uiOutgoingRateKbps, uint32_t uiTotalBytesSentInWindow, uint32_t uiDurationMs)
{
  auto it = m_qRecentlySendPackets.begin();
  if (it != m_qRecentlySendPackets.end())
  {
    uint32_t uiTotalBytes = uiTotalBytesSentInWindow + rtpPacket.getSize();
    // when using the actual duration of the packets in the window, the value is inaccurate for small packet windows
    // e.g. when starting to stream. Just using window size as duration even though this might not be 100% accurate when the window
    // is full
    uint32_t uiKbps = (uiTotalBytes * 8)/uiDurationMs;
    bool bCanSend = uiKbps <= (m_uiNetworkRateKbps + m_uiMaxOvershootKbps);
    VLOG(2) << "NadaScheduler::canSendPacket: " << bCanSend << " outgoing rate: " << uiOutgoingRateKbps << " rate after packet send: " << uiKbps << " Total bytes in send window : " << uiTotalBytesSentInWindow << " measurement interval: " << uiDurationMs << " ms network rate: " << m_uiNetworkRateKbps << " overshoot: " << + m_uiMaxOvershootKbps;
    return bCanSend;
  }
  else
  {
    // no packets sent recently
    return true;
  }
}

void NadaScheduler::sendPacketsIfPossible()
{
  // cancel timer in case one has been scheduled
  m_timer.cancel();

  auto it = m_qRtpPackets.begin();
  bool bMorePackets = it != m_qRtpPackets.end();

  if (!bMorePackets) return;

  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  updateOutgoingRateQueue(tNow);
  while(it != m_qRtpPackets.end())
  {
    uint32_t uiTotalBytesSentInWindow = 0;
    uint32_t uiDurationMs = 0;
    uint32_t uiOutgoingRateKbps = calculateSendRateKbps(tNow, uiTotalBytesSentInWindow, uiDurationMs);
    RtpPacket& rtpPacket = *it;
    if (canSendPacket(rtpPacket, uiOutgoingRateKbps, uiTotalBytesSentInWindow, uiDurationMs))
    {
      rtpPacket.setSendTime(tNow);
      m_pRtpSession->sendRtpPacket(rtpPacket);
      m_qRecentlySendPackets.push_back(rtpPacket);
      m_uiBufferLengthBytes -= rtpPacket.getSize();
      ++it;
    }
    else
    {
      // rate would be too high
      break;
    }
  }

  // removed sent packets from queue
  m_qRtpPackets.erase(m_qRtpPackets.begin(), it);

  // if queue non-empty schedule timer to call sendPacketsifPossible again?
  if (!m_qRtpPackets.empty())
  {
    const uint32_t uiPaceIntervalMs = 10;
    m_timer.expires_from_now(boost::posix_time::milliseconds(uiPaceIntervalMs));
    m_timer.async_wait([&](const boost::system::error_code& ec)
    {
      if (!ec)
      {
        sendPacketsIfPossible();
      }
    });
  }
}

void NadaScheduler::scheduleRtxPacket(const RtpPacket& rtpPacket)
{

}

void NadaScheduler::shutdown()
{

}

void NadaScheduler::processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep)
{
  if (fb.getTypeSpecific() ==  experimental::TL_FB_NADA)
  {
    const experimental::RtcpNadaFb& nadaFb = static_cast<const experimental::RtcpNadaFb&>(fb);
    VLOG(2) << "NadaScheduler::processFeedback: SSRC: " << hex(nadaFb.getSenderSSRC())
            << " x_n: " <<  nadaFb.get_x_n()
            << " r_recv: " << nadaFb.get_r_recv()
            << " rmode: " << nadaFb.get_rmode();

    updateNadaParameters(nadaFb);
  }
}

void NadaScheduler::onIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                            bool bSSRCValidated, bool bRtcpSynchronised,
                                            const boost::posix_time::ptime& tPresentation)
{
  m_nadaRx.receivePacket(rtpPacket);
}

void NadaScheduler::onIncomingRtcp(const CompoundRtcpPacket& /*compoundRtcp*/,
                                             const EndPoint& /*ep*/)
{

}

std::vector<rfc4585::RtcpFb::ptr> NadaScheduler::retrieveFeedback()
{
  std::vector<rfc4585::RtcpFb::ptr> feedback;
  int64_t x_n = 0;
  uint32_t ui_r_recv = 0;
  uint32_t ui_rmode = 0;
  if (m_nadaRx.generateFeedback(ui_rmode, x_n, ui_r_recv))
  {
    VLOG(2) << "Generating NADA feedback - rmode: " << ui_rmode << " x_n: " << x_n << " r_recv: " << ui_r_recv;
    const RtpSessionState& state = m_pRtpSession->getRtpSessionState();
    experimental::RtcpNadaFb::ptr pFb = experimental::RtcpNadaFb::create(x_n, ui_r_recv, ui_rmode, state.getRemoteSSRC(), state.getSSRC());
    feedback.push_back(pFb);
  }
  return feedback;
}

void NadaScheduler::updateNadaParameters(const experimental::RtcpNadaFb& fb)
{
  uint32_t uiRateKbps = m_nadaTx.calculateRateKbps(fb, m_uiBufferLengthBytes, m_uiEncoderRateKbps, m_uiNetworkRateKbps);
  VLOG(2) << "NADA FB: r_n: " << uiRateKbps << " r_vin:" << m_uiEncoderRateKbps << " r_send: " << m_uiNetworkRateKbps;
  if (m_pCooperative && m_uiPreviousKbps != m_uiEncoderRateKbps)
  {
    m_pCooperative->setBitrate(m_uiEncoderRateKbps);
    m_uiPreviousKbps = m_uiEncoderRateKbps;
  }

  /// on a rate update, we might be able to send more packets
  sendPacketsIfPossible();
}

} // rtp_plus_plus
