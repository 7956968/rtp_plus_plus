#include "CorePch.h"
#include <rtp++/rfc3261/SipAgent.h>
#include <boost/bind.hpp>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#if 1
#include <rtp++/mprtp/MpRtp.h>
#endif
#include <rtp++/network/ExistingConnectionAdapter.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipErrorCategory.h>
#include <rtp++/rfc3264/Rfc3264.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc4566/SdpParser.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

SipAgent::SipAgent(boost::system::error_code& ec,
                   boost::asio::io_service& ioService,
                   const GenericParameters &applicationParameters,
                   const SipContext& sipContext,
                   std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswer,
                   std::shared_ptr<media::IVideoDevice> pVideoDevice,
                   std::shared_ptr<media::IVideoDevice> pAudioDevice)
  :m_eState(SA_INIT),
  m_ioService(ioService),
  m_applicationParameters(applicationParameters),
  m_sipContext(sipContext),
  m_pOfferAnswerModel(pOfferAnswer),
  m_pVideoDevice(pVideoDevice),
  m_pAudioDevice(pAudioDevice),
  m_userAgent(ioService, sipContext, ec),
  m_bUseSipProxy(false),
  m_bRegisterWithProxy(false),
  m_bIsRegistered(false),
  m_bIsCaller(false),
  m_uiTxId(0)
{
  if (ec)
  {
    VLOG(5) << "Error initialising user agent";
    m_eState = SA_ERROR;
    return;
  }
  else
  {
    m_userAgent.setOnInviteHandler(boost::bind(&SipAgent::onInvite, this, _1, _2));
    m_userAgent.setInviteResponseHandler(boost::bind(&SipAgent::onInviteResponse, this, _1, _2, _3));
    m_userAgent.setRegisterResponseHandler(boost::bind(&SipAgent::onRegisterResponse, this, _1, _2, _3));
    m_userAgent.setOnByeHandler(boost::bind(&SipAgent::onBye, this, _1, _2));
    m_userAgent.setByeResponseHandler(boost::bind(&SipAgent::onByeResponse, this, _1, _2, _3));

    auto publicIp = applicationParameters.getStringParameter(app::ApplicationParameters::public_ip);
    if (publicIp)
    {
      m_sPublicIp = *publicIp;
      VLOG(2) << "Using public IP: " << m_sPublicIp;
    }
  }

  // relaxing this restriction so that it can be set later.
#if 0
  if (!m_pVideoDevice)
  {
    VLOG(5) << "Invalid video device";
    ec = boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    return;
  }
  else
  {
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromVideoDevice, this, _1));
  }
#else
  if (m_pVideoDevice)
  {
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromVideoDevice, this, _1));
  }
  if (m_pAudioDevice)
  {
    m_pAudioDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromAudioDevice, this, _1));
  }
#endif
}

void SipAgent::setDevices(std::shared_ptr<media::IVideoDevice> pVideoDevice,
  std::shared_ptr<media::IVideoDevice> pAudioDevice)
{
  if (pVideoDevice)
  {
    m_pVideoDevice = pVideoDevice;
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromVideoDevice, this, _1));
  }
  if (pAudioDevice)
  {
    m_pAudioDevice = pAudioDevice;
    m_pAudioDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromAudioDevice, this, _1));
  }
}

void SipAgent::setVideoDevice(std::shared_ptr<media::IVideoDevice> pVideoDevice)
{
  if (pVideoDevice)
  {
    m_pVideoDevice = pVideoDevice;
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromVideoDevice, this, _1));
  }
}

void SipAgent::setAudioDevice(std::shared_ptr<media::IVideoDevice> pAudioDevice)
{
  if (pAudioDevice)
  {
    m_pAudioDevice = pAudioDevice;
    m_pAudioDevice->setReceiveAccessUnitCB(boost::bind(&SipAgent::getAccessUnitsFromAudioDevice, this, _1));
  }
}

boost::system::error_code SipAgent::getAccessUnitsFromVideoDevice(const std::vector<media::MediaSample>& accessUnit)
{
  VLOG(12) << "Received frame from video device: AUs: " << accessUnit.size();
  if (m_pMediaSession)
  {
    m_pMediaSession->sendVideo(accessUnit);
  }
  return boost::system::error_code();
}

boost::system::error_code SipAgent::getAccessUnitsFromAudioDevice(const std::vector<media::MediaSample>& accessUnit)
{
  VLOG(12) << "Received frame from audio device: AUs: " << accessUnit.size();
  if (m_pMediaSession)
  {
    m_pMediaSession->sendAudio(accessUnit);
  }
  return boost::system::error_code();
}

boost::system::error_code SipAgent::setRegisterServer(const std::string& sSipRegisterServer)
{
  boost::optional<SipUri> proxy = SipUri::parse(sSipRegisterServer);
  if (!proxy)
  {
    VLOG(2) << "Invalid SIP Proxy/REGISTER server: " << sSipRegisterServer;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  else
  {
    VLOG(2) << "SIP Proxy/REGISTER server: " << sSipRegisterServer;
  }
  m_pSipProxy = proxy;
  return boost::system::error_code();
}

boost::system::error_code SipAgent::start()
{
  boost::system::error_code ec;
  ec = m_userAgent.start();
  if (ec)
  {
    LOG(WARNING) << "Error starting UA" << ec.message();
    return ec;
  }
  else
  {
    if (m_pSipProxy && m_bRegisterWithProxy)
    {
      uint32_t uiTxId;
      uint32_t uiExpires = 7200; 
      ec = m_userAgent.registerWithServer(*m_pSipProxy, m_sipContext.getUserSipUri(), m_sipContext.getUserSipUri(), uiExpires, uiTxId);
    }
  }
  return ec;
}

boost::system::error_code SipAgent::stop()
{
  VLOG(6) << "SipAgent::stop()";
  boost::system::error_code ec;

  if (m_bIsRegistered)
  {
    VLOG(2) << "DEREGISTER?";
    uint32_t uiTxId;
    uint32_t uiExpires = 0; // a value of zero causes degistration
    ec = m_userAgent.registerWithServer(*m_pSipProxy, m_sipContext.getUserSipUri(), m_sipContext.getUserSipUri(), uiExpires, uiTxId);
  }

  switch (m_eState)
  {
  case SA_DIALING_OR_RINGING:
  {
    if (m_bIsCaller)
    {
      VLOG(2) << "TODO: cancel call";
    }
    else
    {
      VLOG(2) << "TODO: reject call, need to send signal to stop ringing?";
    }
    break;
  }
  case SA_CALL_IN_PROGRESS:
  {
    VLOG(2) << "Call in progress, hanging up";
    ec = hangup();
    if (ec)
    {
      VLOG(2) << "Error hanging up: " << ec.message();
    }
    break;
  }
  case SA_INIT:
  case SA_ERROR:
  default:
  {
    break;
  }
  }

  ec = m_userAgent.stop();
  if (ec)
  {
    LOG(WARNING) << "Error stopping UA" << ec.message();
  }

  if (m_onSessionEndHandler) m_onSessionEndHandler(ec);

  return ec;
}

boost::system::error_code SipAgent::call(const std::string& sCallee)
{
  boost::optional<SipUri> callee = SipUri::parse(sCallee);
  if (!callee)
  {
    VLOG(5) << "Invalid callee: " << sCallee;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  m_pSipCallee = callee;

  assert(!m_sipContext.FQDN.empty());
  // construct local URI
  std::string sHost = m_sipContext.FQDN;
  std::ostringstream user;
  user << SIP << ":" << m_sipContext.User << "@" << sHost;
  uint32_t uiTxId = 0;

  // get session description
#if 0
  boost::optional<std::string> sessionDescription = m_mediaManager.getOffer(m_sipContext.User, m_bEnableMpRtp, m_bRtcpMux);
  if (sessionDescription)
  {
    // store for local SDP
    boost::optional < rfc4566::SessionDescription > localSdp = rfc4566::SdpParser::parse(*sessionDescription);
    assert(localSdp);
    m_localSdp = localSdp;
    MessageBody sdp(*sessionDescription, rfc4566::APPLICATION_SDP);
#else
  boost::optional<rfc4566::SessionDescription> sessionDescription = m_pOfferAnswerModel->generateOffer();
  if (sessionDescription)
  {
    // store for local SDP
    m_localSdp = sessionDescription;
    if (!m_sPublicIp.empty())
    {
      VLOG(2) << "Overriding public IP in SDP offer: " << m_sPublicIp;
      sessionDescription->updateAllConnectionAddresses(m_sPublicIp);
    }
    MessageBody sdp(sessionDescription->toString(), rfc4566::APPLICATION_SDP);
#endif
    std::string sProxy = m_bUseSipProxy ? *m_pSipProxy->toString() : "";
    // generate request
    boost::system::error_code ec = m_userAgent.initiateSession(*m_pSipCallee->toString(), user.str(), sHost, uiTxId, sdp, sProxy);
    if (ec)
    {
      LOG(WARNING) << " UA error on INVITE" << ec.message();
    }
    else
    {
      m_bIsCaller = true;
      updateState(SA_DIALING_OR_RINGING);
    }
    return ec;
  }
  else
  {
    SipErrorCategory cat;
    return boost::system::error_code(FAILED_TO_GET_SESSION_DESCRIPTION, cat);
  }
}

boost::system::error_code SipAgent::hangup()
{
  // This would have to change once a SIP phone can register with a proxy.
  // In that case hangup should not stop the UA
  VLOG(6) << "SipAgent::hangup()";
  boost::system::error_code ec;

  //  The UAC MUST
  //  consider the session terminated(and therefore stop sending or
  //  listening for media) as soon as the BYE request is passed to the
  //  client transaction.  
  m_userAgent.terminateSession(m_uiTxId);

  ec = stopMediaSession();

  return ec;
}

boost::system::error_code SipAgent::stopMediaSession()
{
  boost::system::error_code ec;

  VLOG(6) << "TODO: When should this media session be stopped";
  if (m_pMediaSession)
  {
    ec = m_pMediaSession->stop();
    assert(!ec);
  }

  if (m_pVideoDevice)
  {
    VLOG(6) << "TODO: When should this video device be stopped";
    ec = m_pVideoDevice->stop();
    assert(!ec);
  }

  if (m_pAudioDevice)
  {
    VLOG(6) << "TODO: When should this audio device be stopped";
    ec = m_pAudioDevice->stop();
    assert(!ec);
  }

  return ec;
}

boost::system::error_code SipAgent::accept(const uint32_t uiTxId)
{
  VLOG(6) << "SipAgent::accept()";
  boost::system::error_code ec;
#if 1
  assert(m_remoteSdp);
  boost::optional<rfc4566::SessionDescription> answer = m_pOfferAnswerModel->generateAnswer(*m_remoteSdp);
  assert(answer);
  MessageBody sdp(answer->toString(), rfc4566::APPLICATION_SDP);
  ec = m_userAgent.accept(uiTxId, sdp);
  if (ec)
  {
    updateState(SA_INIT);
    LOG(WARNING) << "Error accepting call" << ec.message();
    return ec;
  }

  // parse local SDP
#ifdef DEBUG
#define VERBOSE true
#else
#define VERBOSE false
#endif
  m_localSdp = answer;
  // This might change if we allow the OFFER to be in the 200 OK and the ANSWER should be in the ACK
  // but for now INVITE must contain OFFER and 200 OK ANSWER

  ec = startMediaSession();

  m_uiTxId = uiTxId;
  updateState(SA_CALL_IN_PROGRESS);
  return ec;
#else
  // get session description
  boost::optional<std::string> sessionDescription = m_mediaManager.getSessionDescription();
  if (sessionDescription)
  {
    MessageBody sdp(*sessionDescription, rfc4566::APPLICATION_SDP);
    ec = m_userAgent.accept(uiTxId, sdp);
    if (ec)
    {
      updateState(SA_INIT);
      LOG(WARNING) << "Error accepting call" << ec.message();
      return ec;
    }

    // parse local SDP
#ifdef DEBUG
    #define VERBOSE true
#else
    #define VERBOSE false
#endif
    m_localSdp = rfc4566::SdpParser::parse(*sessionDescription, VERBOSE);
    // This might change if we allow the OFFER to be in the 200 OK and the ANSWER should be in the ACK
    // but for now INVITE must contain OFFER and 200 OK ANSWER

    ec = startMediaSession();

    m_uiTxId = uiTxId;
    updateState(SA_CALL_IN_PROGRESS);
    return ec;
  }
  else
  {
    LOG(WARNING) << "Unable to get local session description - rejecting call";
    // something went wrong getting a local SDP: reject for now
    // TODO: pass in error code to reject call with Internal Server Error
    updateState(SA_INIT);
    ec = m_userAgent.reject(uiTxId);
    if (ec)
    {
      LOG(WARNING) << "Error accepting call" << ec.message();
    }
    return ec;
  }
#endif
}

boost::system::error_code SipAgent::registerAudioConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingAudio)
{
  if (m_pMediaSession->hasAudio())
  {
    m_pMediaSession->getAudioSessionManager()->registerMediaCallback(onIncomingAudio);
    return boost::system::error_code();
  }
  else
  {
    return boost::system::error_code(boost::system::errc::no_such_file_or_directory, boost::system::get_generic_category());
  }
}

boost::system::error_code SipAgent::registerVideoConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingVideo)
{
  if (m_pMediaSession->hasVideo())
  {
    m_pMediaSession->getVideoSessionManager()->registerMediaCallback(onIncomingVideo);
    return boost::system::error_code();
  }
  else
  {
    return boost::system::error_code(boost::system::errc::no_such_file_or_directory, boost::system::get_generic_category());
  }
}

boost::system::error_code SipAgent::startMediaSession()
{
  boost::system::error_code ec;
  assert(m_remoteSdp && m_localSdp);
  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), *m_localSdp, *m_remoteSdp);
  SimpleMediaSessionFactory factory;
  MediaSessionNetworkManager& networkManager = m_pOfferAnswerModel->getMediaSessionNetworkManager();
  PortAllocationManager& portManager = networkManager.getPortAllocationManager();
  ExistingConnectionAdapter adapter(&portManager);
  m_pMediaSession = factory.create(m_ioService, adapter, mediaSessionDescription, m_applicationParameters);
  if (!m_pMediaSession)
  {
    updateState(SA_INIT);
    LOG(WARNING) << "Failed to create media session";
    SipErrorCategory sipCategory;
    ec = boost::system::error_code(FAILED_TO_CREATE_MEDIA_SESSION, sipCategory);
    if (m_onSessionStartHandler)
      m_onSessionStartHandler(ec);
    return ec;
  }
  else
  {
    if (m_onSessionStartHandler)
      m_onSessionStartHandler(boost::system::error_code());
  }


  VLOG(6) << "TODO: When should this media session be stopped";
  ec = m_pMediaSession->start();
  assert(!ec);

  if (m_pVideoDevice)
  {
    VLOG(6) << "TODO: When should this video device be stopped";
    VLOG(6) << "TODO: this video device should only be started if the answer or accepted offer is active (port non-zero)";
    ec = m_pVideoDevice->start();
    assert(!ec);
  }
  if (m_pAudioDevice)
  {
    VLOG(6) << "TODO: When should this video device be stopped";
    VLOG(6) << "TODO: this audio device should only be started if the answer or accepted offer is active (port non-zero)";
    ec = m_pAudioDevice->start();
    assert(!ec);
  }
  return ec;
}

boost::system::error_code SipAgent::ringing(const uint32_t uiTxId)
{
  VLOG(6) << "SipAgent::ringing()";
  boost::system::error_code ec;
  ec = m_userAgent.ringing(uiTxId);
  if (ec)
  {
    LOG(WARNING) << "Error rejecting call" << ec.message();
  }

  return ec;
}

boost::system::error_code SipAgent::reject(const uint32_t uiTxId)
{
  VLOG(6) << "SipAgent::reject()";
  boost::system::error_code ec;
  ec = m_userAgent.reject(uiTxId);
  updateState(SA_INIT);
  if (ec)
  {
    LOG(WARNING) << "Error rejecting call" << ec.message();
  }

  return ec;
}

void SipAgent::onInvite(const uint32_t uiTxId, const SipMessage& invite)
{
  VLOG(6) << "SipAgent::onInvite";
  if (m_onInvite)
  {
    m_bIsCaller = false;
    VLOG(10) << "Message body size: " << invite.getMessageBody().getContentLength();
    // TODO: is this the right place to do this
    if (invite.getMessageBody().getContentType() == rfc4566::APPLICATION_SDP)
    {
      VLOG(6) << "SipAgent::onInvite parsing SDP";
      boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(invite.getMessageBody().getMessageBody(), VERBOSE);
      VLOG(6) << "SipAgent::onInvite parsing SDP Done";
      if (!sdp)
      {
        LOG(WARNING) << "Failed to parse OFFER SDP - rejecting call";
        m_userAgent.reject(uiTxId);
      }
      else
      {
        VLOG(6) << "Parsed OFFER SDP";
        // remember SDP for subsequent session setup
        m_remoteSdp = sdp;
      }
    }
    else
    {
      VLOG(2) << "INVITE has no SDP";
    }
    m_onInvite(uiTxId, invite);
  }
  else
  {
    LOG(WARNING) << "No INVITE callback set - rejecting call";
    m_userAgent.reject(uiTxId);
    updateState(SA_INIT);
  }
}

void SipAgent::onBye(const uint32_t uiTxId, const SipMessage& bye)
{
  updateState(SA_INIT);
  VLOG(6) << "TODO: stop media session if playing!";
  boost::system::error_code ec = stopMediaSession();
  if (ec)
  {
    VLOG(2) << "Error stopping media session: " << ec.message();
  }

  if (m_onBye)
  {
    VLOG(6) << "Bye received";
    m_onBye(uiTxId, bye);
  }
  else
  {
    LOG(WARNING) << "No BYE callback set";
  }
}

void SipAgent::onRegisterResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  assert(m_onRegisterResponse);
  if (!ec)
  {
    m_bIsRegistered = true;
    m_onRegisterResponse(ec, uiTxId, sipResponse);
  }
  else
  {
    m_bIsRegistered = false;
    SipErrorCategory cat;
    ResponseCode eCode = sipResponse.getResponseCode();
    boost::system::error_code ec(responseCodeToErrorCode(eCode), cat);
    m_onRegisterResponse(ec, uiTxId, sipResponse);
  }
}

void SipAgent::onInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (!ec)
  {
    ResponseCode eCode = sipResponse.getResponseCode();
    LOG(INFO) << responseCodeString(eCode);
    if (isResponseProvisional(eCode))
    {
    }
    else if (isFinalNonSuccessfulResponse(eCode))
    {
      SipErrorCategory cat;
      boost::system::error_code ec(responseCodeToErrorCode(eCode), cat);
      // m_onSessionEndHandler(ec);
      m_onInviteResponse(ec, uiTxId, sipResponse);
      updateState(SA_INIT);
    }
    else if (isResponseSuccessfull(eCode))
    {
      m_onInviteResponse(ec, uiTxId, sipResponse);

      VLOG(5) << "200 OK has SDP ANSWER";
      boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(sipResponse.getMessageBody().getMessageBody(), VERBOSE);
      if (!sdp)
      {
        LOG(WARNING) << "TODO: Failed to parse ANSWER SDP - send BYE";
        // TODO: what state do we transition to now?!?
      }
      else
      {
        VLOG(6) << "TODO: if answer is not acceptable- send BYE!";
        // remember SDP for subsequent session setup
        m_remoteSdp = sdp;
        processAnswer();
        VLOG(5) << "INVITE successful - starting media session";
        boost::system::error_code ec = startMediaSession();
        assert(!ec);
        m_uiTxId = uiTxId;
        updateState(SA_CALL_IN_PROGRESS);
      }
    }
  }
  else
  {
    m_onInviteResponse(ec, uiTxId, sipResponse);
    updateState(SA_INIT);
  }
}

void SipAgent::processAnswer()
{
  // go through all media descriptions and remove formats which were excluded from the answer
  assert(m_localSdp->getMediaDescriptionCount() == m_remoteSdp->getMediaDescriptionCount());
  for (size_t i = 0; i < m_localSdp->getMediaDescriptionCount(); ++i)
  {
    rtp_plus_plus::rfc4566::MediaDescription& localMedia = m_localSdp->getMediaDescription(i);
    const rtp_plus_plus::rfc4566::MediaDescription& remoteMedia = m_remoteSdp->getMediaDescription(i);
    const std::vector<std::string>& localFormats = localMedia.getFormats();
    const std::vector<std::string>& remoteFormats = remoteMedia.getFormats();
    for (const std::string& sFormat : localFormats)
    {
      auto it = std::find(remoteFormats.begin(), remoteFormats.end(), sFormat);
      if (it == remoteFormats.end())
      {
        VLOG(2) << "Didn't find payload type " << sFormat << " in answer, removing from local SDP";
        localMedia.removeFormat(sFormat);
      }
    }
  }

#if 0
  // HACK to force MPRTP on the offerer without specifying it in the SDP which crashes the linphone.
  m_localSdp->addAttribute(mprtp::MPRTP, "");
#endif
}

void SipAgent::onByeResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (!ec)
  {
    VLOG(5) << "SipAgent::onByeResponse: " << sipResponse.getResponseCode();
  }
  else
  {
    m_onInviteResponse(ec, uiTxId, sipResponse);
  }
  updateState(SA_INIT);
}

std::string SipAgent::stateToString(SipAgentState eState)
{
  switch (eState)
  {
  case SA_INIT:
    return "SA_INIT";
  case SA_DIALING_OR_RINGING:
    return "SA_DIALING_OR_RINGING";
  case SA_CALL_IN_PROGRESS:
    return "SA_CALL_IN_PROGRESS";
  case SA_ERROR:
    return "SA_ERROR";
  default:
    assert(false);
  }
  return "Unknown";
}

void SipAgent::updateState(SipAgentState eState)
{
  VLOG(2) << "State update [" << stateToString(eState) << "]";
  m_eState = eState;
}

} // rfc3261
} // rtp_plus_plus

