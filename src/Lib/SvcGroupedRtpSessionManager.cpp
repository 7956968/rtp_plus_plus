#include "CorePch.h"
#include <rtp++/SvcGroupedRtpSessionManager.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>

namespace rtp_plus_plus
{

using media::MediaSample;

SvcGroupedRtpSessionManager::SvcGroupedRtpSessionManager(boost::asio::io_service& ioService,
                                                         const GenericParameters& applicationParameters)
  :GroupedRtpSessionManager(applicationParameters),
    m_rIoService(ioService)
{
  // set buffer latency if configured in application parameters
  boost::optional<uint32_t> uiBufferLatency = applicationParameters.getUintParameter(app::ApplicationParameters::buf_lat);
  if (uiBufferLatency) m_receiverBuffer.setPlayoutBufferLatency(*uiBufferLatency);
}

void SvcGroupedRtpSessionManager::doSend(const media::MediaSample& mediaSample, const std::string& /*sMid*/)
{
  // NOOP
}

void SvcGroupedRtpSessionManager::doSend(const media::MediaSample& mediaSample)
{
  // HACK: example code to illustrate what to do here: this code just select the RTP session via
  // round robin
  Buffer buffer = mediaSample.getDataBuffer();
  IBitStream in(buffer);
  // forbidden
  in.skipBits(1);
  // nri
  uint32_t uiNri = 0, uiNut = 0;
  in.read(uiNri, 2);
  //  NUT
  in.read(uiNut, 5);
  int gSessionIndex = (uiNut==media::h264::NUT_CODED_SLICE_EXT||uiNut==media::h264::NUT_RESERVED_21);// NUT_RESERVED_21 is in this old version of the encoder

  RtpSession::ptr pRtpSession = getRtpSession(getMid(gSessionIndex));


  std::vector<RtpPacket> rtpPackets = pRtpSession->packetise(mediaSample);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
  pRtpSession->sendRtpPackets(rtpPackets);
}

void SvcGroupedRtpSessionManager::doSend(const std::vector<media::MediaSample>& vMediaSamples, const std::string &/*sMid*/)
{
  // NOOP
}

void SvcGroupedRtpSessionManager::doSend(const std::vector<media::MediaSample>& vMediaSamples)
{
  // HACK: example code to illustrate what to do here: this code just select the RTP session via
  // round robin
  //boost::posix_time::ptime currentTime =  boost::posix_time::microsec_clock::universal_time();
  for (int i=0; i<2;i++){
    std::vector<MediaSample> MediaSamples;
    for (const MediaSample& mediaSample: vMediaSamples)
    {
      Buffer buffer = mediaSample.getDataBuffer();
      IBitStream in(buffer);

      // forbidden
      in.skipBits(1);
      // nri
      uint32_t uiNri = 0, uiNut = 0;
      in.read(uiNri, 2);
      //  NUT
      in.read(uiNut, 5);
      if(i==(uiNut==media::h264::NUT_CODED_SLICE_EXT||uiNut==media::h264::NUT_RESERVED_21))// NUT_RESERVED_21 is in this old version of the encoder
          MediaSamples.push_back(mediaSample);

    }
    RtpSession::ptr pRtpSession = getRtpSession(getMid(i));
    //gSessionIndex = (gSessionIndex + 1)%getSessionCount();

    std::vector<RtpPacket> rtpPackets = pRtpSession->packetise(MediaSamples/*, currentTime*/);
#if 0
    DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
    pRtpSession->sendRtpPackets(rtpPackets);
   }
}


bool SvcGroupedRtpSessionManager::doIsSampleAvailable() const
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  return !m_qOutgoing.empty();

}

std::vector<media::MediaSample> SvcGroupedRtpSessionManager::doGetNextSample()
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  if (m_qOutgoing.empty()) return std::vector<MediaSample>();
  std::vector<MediaSample> mediaSamples = m_qOutgoing.front();
  m_qOutgoing.pop_front();
  return mediaSamples;
}

void SvcGroupedRtpSessionManager::onRtpSessionAdded(const std::string& sMid,
                                                    const RtpSessionParameters& rtpParameters,
                                                    RtpSession::ptr pRtpSession)
{
  // configure buffer: note: this should be the same for all RTP sessions?
  // Or should this only be done for the primary RTP session?
  m_receiverBuffer.setClockFrequency(rtpParameters.getRtpTimestampFrequency());
}

void SvcGroupedRtpSessionManager::doHandleIncomingRtp(const RtpPacket &rtpPacket,
                                                      const EndPoint &ep, bool bSSRCValidated,
                                                      bool bRtcpSynchronised,
                                                      const boost::posix_time::ptime &tPresentation,
                                                      const std::string &sMid)
{
  // return;

  // only add them into the jitter buffer once we know the streams have been synced with one another
  if (!bRtcpSynchronised) return;

  boost::posix_time::ptime tPlayout;
  // Note: the playout buffer only returns true if a new playout time was calculated
  // for the sample
  uint32_t uiLateMs = 0;
  bool bDuplicate = false;
  if (m_receiverBuffer.addRtpPacket(rtpPacket, tPresentation, bRtcpSynchronised, tPlayout, uiLateMs, bDuplicate))
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
    pTimer->async_wait(boost::bind(&SvcGroupedRtpSessionManager::onScheduledSampleTimeout, this, _1, pTimer) );
    m_mPlayoutTimers.insert(std::make_pair(pTimer, pTimer));
  }

}

void SvcGroupedRtpSessionManager::doHandleIncomingRtcp(const CompoundRtcpPacket &compoundRtcp,
                                                       const EndPoint &ep, const std::string &sMid)
{

}

void SvcGroupedRtpSessionManager::onSessionsStarted()
{
  VLOG(15) << "RTP sessions started";
}

void SvcGroupedRtpSessionManager::onSessionsStopped()
{
  VLOG(15) << "RTP sessions stopped";
  using boost::asio::deadline_timer;

  // stop all scheduled samples
  // cancel all scheduled media sample events
  VLOG(10)  << "[" << this << "] Shutting down Playout timers";
  std::for_each(m_mPlayoutTimers.begin(), m_mPlayoutTimers.end(), [this](const std::pair<deadline_timer*, deadline_timer*>& pair)
  {
    pair.second->cancel();
  });
}

void SvcGroupedRtpSessionManager::onScheduledSampleTimeout(const boost::system::error_code& ec,
                                                 boost::asio::deadline_timer* pTimer)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // trigger sample callback
    RtpPacketGroup node = m_receiverBuffer.getNextPlayoutBufferNode();
    // call the appropriate depacketizer to aggregate the packets, do error concealment, etc
    // HACK: to illustrate what should happen here. The *proper* implementation
    // should know which RTP session contains the correct depacketiser
    // Alt: maybe there should be a depacketiser module in the grouped RTP session manager?
    std::vector<MediaSample> vSamples = getRtpSession(getMid(0))->depacketise(node);
    boost::mutex::scoped_lock l(m_outgoingSampleLock);
    m_qOutgoing.push_back(vSamples);
//    m_qOutgoing.insert(m_qOutgoing.end(), vSamples.begin(), vSamples.end());
  }

  std::size_t uiRemoved = m_mPlayoutTimers.erase(pTimer);
  assert(uiRemoved == 1);
  delete pTimer;

  triggerNotification();
}

} // rtp_plus_plus
