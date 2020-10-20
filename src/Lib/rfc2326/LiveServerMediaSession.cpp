#include "CorePch.h"
#include <rtp++/rfc2326/LiveServerMediaSession.h>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <rtp++/RtpTime.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/rfc2326/LiveRtspServer.h>
#include <rtp++/rfc2326/Rfc2326.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc5761/Rfc5761.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

using media::MediaSample;

const uint32_t MAX_TIME_WITHOUT_LIVENESS_SECONDS = 10;

LiveServerMediaSession::LiveServerMediaSession(RtspServer& rtspServer, const std::string& sSession,
                                               const std::string& sSessionDescription, const std::string& sContentType,
                                               uint32_t uiMaxTimeWithoutLivenessSeconds)
  :ServerMediaSession(rtspServer, sSession, sSessionDescription, sContentType),
    m_uiMaxTimeWithoutLivenessSeconds(uiMaxTimeWithoutLivenessSeconds)
{
  VLOG(10) << "[" << this << "] LiveServerMediaSession Constructor: " << m_sSession;
  boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(sSessionDescription);
  assert(sdp);
  m_sdp = *sdp;
}

LiveServerMediaSession::~LiveServerMediaSession()
{
  VLOG(5) << "[" << this << "] LiveServerMediaSession Destructor: " << m_sSession;
}

void LiveServerMediaSession::updateLiveness()
{
  m_tLastLivenessIndicator = boost::posix_time::microsec_clock::universal_time();
}

void LiveServerMediaSession::onRr(const rfc3550::RtcpRr& rr, const EndPoint& ep)
{
  VLOG(5) << "Incoming RR: updating liveness";
  updateLiveness();
}

bool LiveServerMediaSession::isSessionLive() const
{
  if (m_uiMaxTimeWithoutLivenessSeconds != 0 && isPlaying() && !m_tLastLivenessIndicator.is_not_a_date_time())
  {
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    if ((tNow - m_tLastLivenessIndicator).total_seconds() > (int64_t)m_uiMaxTimeWithoutLivenessSeconds)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    // we're not in a state to worry about this yet
    return true;
  }
}

ResponseCode LiveServerMediaSession::handleSetup(const RtspMessage& setup, std::string& sSession,
                                                 Transport& transport, const std::vector<std::string>& vSupported)
{
#if 0
  // if the SETUP is handled inside the session, the session already exists
  sSession = setup.getSession();
  transport.setServerPortStart(((LiveRtspServer&)m_rtspServer).getNextServerRtpPort());
#else
  VLOG(2) << "Setup for session: " << sSession;

  // get server transport info by calling base class method
  ResponseCode eCode = ServerMediaSession::handleSetup(setup, sSession, transport, vSupported);
  
  if (eCode != OK) return eCode;

  // Update media lines to reflect the selected Transport parameters
  int index = -1;
  for (size_t i = 0; i < m_sdp.getMediaDescriptionCount(); ++i)
  {
    rfc4566::MediaDescription& media = m_sdp.getMediaDescription(i);
    // get the control line
    assert(media.hasAttribute(CONTROL));
    boost::optional<std::string> control = media.getAttributeValue(CONTROL);
    RtspUri uri(setup.getRtspUri());
    VLOG(2) << "Matching control: " << *control << " to URI " << setup.getRtspUri();
    // we need to check for relative and absolute URIs in CONTROL
    if ((setup.getRtspUri() == *control) ||
      (uri.getName() == *control))
    {
      index = i;
      break;
    }
  }

  if (index == -1)
  {
    VLOG(2) << "Unable to match control line" << setup.getRtspUri();
    return BAD_REQUEST;
  }
  //assert(sdp->getMediaDescriptionCount() == 1);
  // replace server transport port in SDP
  m_sdp.getMediaDescription(index).setPort(transport.getServerPortStart());
  // update profile according to selected Transport
  std::string sTransportSpecifier = m_sdp.getMediaDescription(index).getProtocol();
  if (sTransportSpecifier != transport.getSpecifier())
  {
    VLOG(2) << "Transport specifier accepted by server differs from initial specifier in SDP. Updating from "
      << sTransportSpecifier << " to " << transport.getSpecifier();
    m_sdp.getMediaDescription(index).setProtocol(transport.getSpecifier());
  }
  else
  {
    VLOG(5) << "Accepted transport specifier " << sTransportSpecifier;
  }

  // Now add interfaces
  if (transport.isUsingMpRtp())
  {
    // use media interface descriptors
    MediaSessionNetworkManager& networkManager = m_rtspServer.getMediaSessionNetworkManager();
    MediaInterfaceDescriptor_t& localMediaInterfaces = networkManager.getMediaDescriptor(sSession, index);

    // add the bound interfaces to the SDP which is used to setup the media session object
    for (size_t i = 0; i < localMediaInterfaces.size(); ++i)
    {
      AddressTuple_t& address = localMediaInterfaces[i];
      AddressDescriptor& rtp = std::get<0>(address);
      std::ostringstream mprtpInterfaceAttribute;
      mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << rtp.getAddress() << ":" << rtp.getPort();
      std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
      VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
      m_sdp.getMediaDescription(index).addAttribute(mprtp::MPRTP, sMpRtpInterface);
    }

    // get client addesses and ports also used to setup the media session object
    MediaInterfaceDescriptor_t mediaInterfaces = mprtp::getMpRtpInterfaceDescriptorFromRtspTransportString(transport.getDestMpRtpAddr(), transport.getRtcpMux());
    m_remoteInterfaces.push_back(mediaInterfaces);
  }
  else
  {
    uint16_t uiClientRtpPort = transport.getClientPortStart();
    uint16_t uiClientRtcpPort = (transport.getRtcpMux()) ? uiClientRtpPort : uiClientRtpPort + 1;
    // client network config
    AddressDescriptor rtpDescriptor(transport.getDestination(), uiClientRtpPort);
    AddressDescriptor rtcpDescriptor(transport.getDestination(), uiClientRtcpPort);
    AddressTuple_t tuple = std::tie(rtpDescriptor, rtcpDescriptor, "");
    MediaInterfaceDescriptor_t interface1;
    interface1.push_back(tuple);
    m_remoteInterfaces.push_back(interface1);
  }
#endif

  updateLiveness();
  return OK;
}

ResponseCode LiveServerMediaSession::doHandlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported)
{
  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
    m_sdp, m_remoteInterfaces, true);
  // create RTP media session
  SimpleMediaSessionFactory factory;
  PortAllocationManager& portManager = m_rtspServer.getMediaSessionNetworkManager().getPortAllocationManager();
  // create adapter to port manager: this will be used to retrieve allocated UDP sockets
  ExistingConnectionAdapter adapter(&portManager);
  // dummy for now
  GenericParameters applicationParameters;
  m_pMediaSession = factory.create(m_rtspServer.getIoService(), adapter, mediaSessionDescription, applicationParameters);
  if (m_pMediaSession)
  {
    VLOG(5) << "Media session V2 created";
    // we're only handling max 1 audio + 1 video
    assert(m_sdp.getMediaDescriptionCount() <= 2);

    updateLiveness();
  }
  else
  {
    LOG(WARNING) << "Failed to create media session";
    return INTERNAL_SERVER_ERROR;
  }

  // only dealing with video or audio for now
  // TODO: how to populate RtpInfo struct if session has video and audio? Both URI's need to be added with
  // own TS and SN space!

  DLOG(INFO) << "TODO: add correct RtpInfo header if there are both audio and video";

  if (m_pMediaSession->hasVideo())
  {
    IRtpSessionManager& rtpSessionManager = *(m_pMediaSession->getVideoSessionManager().get());

    rtpSessionManager.setRrNotifier(boost::bind(&LiveServerMediaSession::onRr, this, _1, _2));

    const RtpSession& rtpSession = rtpSessionManager.getPrimaryRtpSession();
    const RtpSessionParameters& rtpSessionParameters = rtpSessionManager.getPrimaryRtpSessionParameters();
    const RtpSessionState& rtpSessionState = rtpSession.getRtpSessionState();

    // remember payload type: this will be used to demux the incoming media
    m_uiVideoPayload = rtpSessionParameters.getPayloadType();
    VLOG(2) << "Video payload type: " << (int)m_uiVideoPayload;

    rtpInfo.setRtspUri(play.getRtspUri());
    // get info from RTP session
    uint32_t uiTimestampFrequency = rtpSessionParameters.getRtpTimestampFrequency();
    uint32_t uiTimestampBase = rtpSessionState.getRtpTimestampBase();
    uint32_t uiRtpNow = RtpTime::currentTimeToRtpTimestamp(uiTimestampFrequency, uiTimestampBase);

    rtpInfo.setStartSN(rtpSessionState.getCurrentSequenceNumber());
    rtpInfo.setRtpTs(uiRtpNow);
    VLOG(2) << "Media session RTP Info - SN: " << rtpSessionState.getCurrentSequenceNumber()
      << " RTP TS: " << uiRtpNow;
  }
  
  if (m_pMediaSession->hasAudio())
  {
    IRtpSessionManager& rtpSessionManager = *(m_pMediaSession->getAudioSessionManager().get());

    // hook into the receiver reports received functionality so that we can check the session liveness
    rtpSessionManager.setRrNotifier(boost::bind(&LiveServerMediaSession::onRr, this, _1, _2));

    const RtpSession& rtpSession = rtpSessionManager.getPrimaryRtpSession();
    const RtpSessionParameters& rtpSessionParameters = rtpSessionManager.getPrimaryRtpSessionParameters();
    const RtpSessionState& rtpSessionState = rtpSession.getRtpSessionState();

    // remember payload type: this will be used to demux the incoming media
    m_uiAudioPayload = rtpSessionParameters.getPayloadType();
    VLOG(2) << "Audio payload type: " << (int)m_uiAudioPayload;

    rtpInfo.setRtspUri(play.getRtspUri());
    // get info from RTP session
    uint32_t uiTimestampFrequency = rtpSessionParameters.getRtpTimestampFrequency();
    uint32_t uiTimestampBase = rtpSessionState.getRtpTimestampBase();
    uint32_t uiRtpNow = RtpTime::currentTimeToRtpTimestamp(uiTimestampFrequency, uiTimestampBase);

    rtpInfo.setStartSN(rtpSessionState.getCurrentSequenceNumber());
    rtpInfo.setRtpTs(uiRtpNow);

    VLOG(2) << "Media session RTP Info - SN: " << rtpSessionState.getCurrentSequenceNumber()
      << " RTP TS: " << uiRtpNow;
  }

  // start media session
  VLOG(2) << "Starting media session " << m_sSession;
  boost::system::error_code ec = m_pMediaSession->start();

  if (!ec)
  {
    return OK;
  }
  else
  {
    LOG(WARNING) << "Failed to start media session: " << ec.message();
    return INTERNAL_SERVER_ERROR;
  }
}

void LiveServerMediaSession::doDeliver(const uint8_t uiPayloadType, const MediaSample& mediaSample)
{
  if (m_pMediaSession)
  {
    // TODO: only play if state is playing! Base class needs to manage state?!?
#if 0
    // TODO: should suffice for now since we only have video.
    // FIXME: will have to rethink for audio and video e.g. add ID

    // HACK FOR NOW
    if (m_pMediaSession->hasVideo())
      m_pMediaSession->sendVideo(mediaSample);
    else if (m_pMediaSession->hasAudio())
      m_pMediaSession->sendAudio(mediaSample);
#else
    if (uiPayloadType == m_uiVideoPayload)
    {
      assert(m_pMediaSession->hasVideo());
      m_pMediaSession->sendVideo(mediaSample);
    }
    else if (uiPayloadType == m_uiAudioPayload)
    {
      assert(m_pMediaSession->hasAudio());
      m_pMediaSession->sendAudio(mediaSample);
    }
    else
    {
      LOG(WARNING) << "Unknown payload type: " << (int)uiPayloadType;
    }
#endif
  }
  else
  {
    LOG_FIRST_N(ERROR, 1) << "Can't deliver media: no media session has been created";
  }
}

void LiveServerMediaSession::doDeliver(const uint8_t uiPayloadType, const std::vector<MediaSample>& mediaSamples)
{
  if (m_pMediaSession)
  {
    // TODO: only play if state is playing! Base class needs to manage state?!?
#if 0
    // TODO: should suffice for now since we only have video.
    // FIXME: will have to rethink for audio and video e.g. add ID
    m_pMediaSession->sendVideo(mediaSamples);
#else
    if (uiPayloadType == m_uiVideoPayload)
    {
      assert(m_pMediaSession->hasVideo());
      m_pMediaSession->sendVideo(mediaSamples);
    }
    else if (uiPayloadType == m_uiAudioPayload)
    {
      assert(m_pMediaSession->hasAudio());
      m_pMediaSession->sendAudio(mediaSamples);
    }
    else
    {
      LOG(WARNING) << "Unknown payload type: " << uiPayloadType;
    }
#endif
  }
  else
  {
    LOG_FIRST_N(ERROR, 1) << "Can't deliver media: no media session has been created";
  }
}

void LiveServerMediaSession::doShutdown()
{
  VLOG(2) << "[" << this << "] LiveServerMediaSession:doShutdown: " << m_sSession;
  // stop RTP session
  // start media session
  m_pMediaSession->stop();
}

} // rfc2326
} // rtp_plus_plus

