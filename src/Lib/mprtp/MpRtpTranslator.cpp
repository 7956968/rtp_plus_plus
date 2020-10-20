#include "CorePch.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpTranslator.h>
#include <rtp++/mprtp/MpRtpTranslatorSessionFactory.h>
#include <rtp++/media/SimpleMultimediaService.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>

namespace rtp_plus_plus {
namespace mprtp {

boost::shared_ptr<MpRtpTranslator> MpRtpTranslator::create(const GenericParameters& applicationParameters,
                                                           const rfc4566::SessionDescription& sdp1, const rfc4566::SessionDescription& sdp2,
                                                           const InterfaceDescriptions_t& netConf1, const InterfaceDescriptions_t& netConf2)
{
  // At least one of the SDPs must be MPRTP capable
  if (isMpRtpSession(sdp1) || isMpRtpSession(sdp2))
  {
    return boost::make_shared<MpRtpTranslator>(applicationParameters, sdp1, sdp2, netConf1, netConf2);
  }
  else
  {
    LOG(ERROR) << "At least one of the sessions must be an MPRTP sessions";
    return boost::shared_ptr<MpRtpTranslator>();
  }
}

MpRtpTranslator::MpRtpTranslator(const GenericParameters &applicationParameters,
                                 const rfc4566::SessionDescription &sdp1, const rfc4566::SessionDescription &sdp2,
                                 const InterfaceDescriptions_t &netConf1, const InterfaceDescriptions_t &netConf2)
  :m_bFirstMpRtpAudio(false),
    m_bFirstMpRtpVideo(false),
    m_bFirstRtpAudio(false),
    m_bFirstRtpVideo(false)
{
  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription1 = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
                                                                             sdp1, netConf1, false);

  MediaSessionDescription mediaSessionDescription2 = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
                                                                             sdp2, netConf2, false);

  // check which session is the MPRTP session
  if (isMpRtpSession(sdp1))
  {
    mprtp::MpRtpTranslatorSessionFactory factory(boost::bind(&MpRtpTranslator::handleIncomingVideoMpRtp, this, _1, _2),
                                                 boost::bind(&MpRtpTranslator::handleIncomingAudioMpRtp, this, _1, _2));

    m_pMpRtpMediaSession = factory.create(mediaSessionDescription1, applicationParameters);


    mprtp::MpRtpTranslatorSessionFactory factory2(boost::bind(&MpRtpTranslator::handleIncomingVideoRtp, this, _1, _2),
                                                  boost::bind(&MpRtpTranslator::handleIncomingAudioRtp, this, _1, _2));
    m_pRtpMediaSession = factory2.create(mediaSessionDescription2, applicationParameters);
  }
  else
  {
    mprtp::MpRtpTranslatorSessionFactory factory(boost::bind(&MpRtpTranslator::handleIncomingVideoMpRtp, this, _1, _2),
                                                 boost::bind(&MpRtpTranslator::handleIncomingAudioMpRtp, this, _1, _2));

    m_pMpRtpMediaSession = factory.create(mediaSessionDescription2, applicationParameters);


    mprtp::MpRtpTranslatorSessionFactory factory2(boost::bind(&MpRtpTranslator::handleIncomingVideoRtp, this, _1, _2),
                                                  boost::bind(&MpRtpTranslator::handleIncomingAudioRtp, this, _1, _2));
    m_pRtpMediaSession = factory2.create(mediaSessionDescription1, applicationParameters);
  }
}

boost::system::error_code MpRtpTranslator::doStart()
{
  // Each service has to be started in it's own thread since start blocks
  VLOG(2) << "Starting MPRTP session thread";
  m_pMpRtpThread = boost::shared_ptr<boost::thread>(new boost::thread(&SimpleMediaSession::start, m_pMpRtpMediaSession));
  VLOG(2) << "Starting RTP session thread";
  m_pRtpThread = boost::shared_ptr<boost::thread>(new boost::thread(&SimpleMediaSession::start, m_pRtpMediaSession));
//  m_pMpRtpMediaSession->start();
//  m_pRtpMediaSession->start();
  return boost::system::error_code();
}

boost::system::error_code MpRtpTranslator::doStop()
{
  VLOG(2) << "Stopping MPRTP session thread";
  m_pMpRtpMediaSession->stop();
  VLOG(2) << "Stopping RTP session thread";
  m_pRtpMediaSession->stop();
  m_pMpRtpThread->join();
  m_pRtpThread->join();
  VLOG(2) << "Done";
  return boost::system::error_code();
}

void MpRtpTranslator::handleIncomingAudioRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  RtpSession& rtpSession = m_pMpRtpMediaSession->getAudioSessionManager()->getPrimaryRtpSession();
  // update RTP packet
  if (!m_bFirstRtpAudio)
  {
    m_bFirstRtpAudio = true;
    // update SSRC of audio MPRTP session
    RtpSessionState& state = rtpSession.getRtpSessionState();
    state.updateSSRC(rtpPacket.getSSRC());
  }

  // now translate to MPRTP and send in other session: at this point we could do buffering or jitter compensation, etc.
  // Alternatively, if the RTP session manager supports it, that could be done in there
  RtpPacket mprtpPacket = rtpPacket;

  // TODO: investigate of we still need to add the extension here with the new API?
  LOG(WARNING) << "TODO: investigate of we still need to add the extension here with the new API?";
#if 0
  rtpSession.addMpRtpExtensionHeader(mprtpPacket); // we could specify the flow here
#endif

  // old API
  // rtpSession.sendRtpPackets(std::vector<RtpPacket>(1, mprtpPacket));

  // new API
  uint32_t uiFlowId = 0; // we could specify the flow here
  rtpSession.sendRtpPacket(mprtpPacket, uiFlowId);
}

void MpRtpTranslator::handleIncomingVideoRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  RtpSession& rtpSession = m_pMpRtpMediaSession->getVideoSessionManager()->getPrimaryRtpSession();
  // update RTP packet
  if (!m_bFirstRtpVideo)
  {
    m_bFirstRtpVideo = true;
    RtpSessionState& state = rtpSession.getRtpSessionState();
    // update SSRC of video MPRTP session
    state.updateSSRC(rtpPacket.getSSRC());
  }

  // now translate to MPRTP and send in other session: at this point we could do buffering or jitter compensation, etc.
  // Alternatively, if the RTP session manager supports it, that could be done in there
  RtpPacket mprtpPacket = rtpPacket;
  // TODO: investigate of we still need to add the extension here with the new API?
  LOG(WARNING) << "TODO: investigate of we still need to add the extension here with the new API?";
#if 0
  rtpSession.addMpRtpExtensionHeader(mprtpPacket); // we could specify the flow here
#endif

  // old API
  // rtpSession.sendRtpPackets(std::vector<RtpPacket>(1, mprtpPacket));

  // new API
  uint32_t uiFlowId = 0; // we could specify the flow here
  rtpSession.sendRtpPacket(mprtpPacket, uiFlowId);
}

void MpRtpTranslator::handleIncomingAudioMpRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  RtpSession& rtpSession = m_pRtpMediaSession->getAudioSessionManager()->getPrimaryRtpSession();
  // update RTP packet
  if (!m_bFirstMpRtpAudio)
  {
    m_bFirstMpRtpAudio = true;
    // update SSRC of audio MPRTP session
    RtpSessionState& state = rtpSession.getRtpSessionState();
    // update SSRC of audio MPRTP session
    state.updateSSRC(rtpPacket.getSSRC());
  }

  // remove MPRTP extension header
  RtpPacket rtpPacketCopy = rtpPacket;
  // FIXME: this code DOES NOT check if it is actually an MPRTP header extension
  //rtpPacketCopy.getHeader().setRtpHeaderExtension(rfc5285::RtpHeaderExtension::ptr());
  // TODO: this code should remove the MPRTP header extension.
  rtpPacketCopy.getHeader().getHeaderExtension().clear();
  rtpSession.sendRtpPackets(std::vector<RtpPacket>(1, rtpPacketCopy));
}

void MpRtpTranslator::handleIncomingVideoMpRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
{
  RtpSession& rtpSession = m_pRtpMediaSession->getVideoSessionManager()->getPrimaryRtpSession();
  // update RTP packet
  if (!m_bFirstMpRtpVideo)
  {
    m_bFirstMpRtpVideo = true;
    RtpSessionState& state = rtpSession.getRtpSessionState();
    // update SSRC of video MPRTP session
    state.updateSSRC(rtpPacket.getSSRC());
  }

  // remove MPRTP extension header
  RtpPacket rtpPacketCopy = rtpPacket;
  // FIXME: this code DOES NOT check if it is actually an MPRTP header extension
  //rtpPacketCopy.getHeader().setRtpHeaderExtension(rfc5285::RtpHeaderExtension::ptr());
  // TODO: this code should remove the MPRTP header extension.
  rtpPacketCopy.getHeader().getHeaderExtension().clear();

  rtpSession.sendRtpPackets(std::vector<RtpPacket>(1, rtpPacketCopy));
}

} // mprtp
} // rtp_plus_plus
