#include "CorePch.h"
#include <rtp++/rfc2326/RtspClientSession.h>
#include <cpputil/Conversion.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/ExistingConnectionAdapter.h>
#include <rtp++/rfc2326/Rfc2326.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc5285/Rfc5285.h>
#include <rtp++/rfc5761/Rfc5761.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

using boost::optional;

const uint32_t RtspClientSession::DEFAULT_RTP_PORT = 49170;

RtspClientSession::RtspClientSession(boost::asio::io_service& ioService, 
                                     const GenericParameters& applicationParameters, 
                                     const RtspUri& rtspUri, 
                                     const std::vector<std::string>& vLocalInterfaces)
  :m_ioService(ioService),
    m_timer(m_ioService),
    m_applicationParameters(applicationParameters),
    m_rtspUri(rtspUri),
    m_rtspClient(ioService, m_rtspUri.fullUri(), m_rtspUri.getServerIp(), m_rtspUri.getPort()),
    m_mediaSessionNetworkManager(ioService, vLocalInterfaces),
    m_uiSetupCount(0),
    m_bInterleaved(false),
    m_eState(STATE_INIT),
    m_bShuttingDown(false),
    m_uiDurationSeconds(0),
    m_uiRtpPort(DEFAULT_RTP_PORT),
    m_uiInterleaved(0),
    m_bEnableMpRtp(false),
    m_bDoesServerSupportMpRtp(false),
    m_bDoesServerSupportRtpRtcpMux(false)
{
  auto client_rtp_port = applicationParameters.getUintParameter(app::ApplicationParameters::client_rtp_port);
  if (client_rtp_port)
  {
    m_uiRtpPort = *client_rtp_port;
  }

  auto enable_mprtp = applicationParameters.getBoolParameter(app::ApplicationParameters::enable_mprtp);
  if (enable_mprtp)
  {
    m_bEnableMpRtp = *enable_mprtp;
    VLOG(2) << "MPRTP enabled: " << m_bEnableMpRtp;
  }

  auto rtcp_mux = applicationParameters.getBoolParameter(app::ApplicationParameters::rtcp_mux);
  if (rtcp_mux)
  {
    m_bRtcpMux = *rtcp_mux;
    VLOG(2) << "RTCP-mux enabled: " << m_bRtcpMux;
  }

  auto dest_mprtp_addr = applicationParameters.getStringsParameter(app::ApplicationParameters::dest_mprtp_addr);
  if (dest_mprtp_addr && !dest_mprtp_addr->empty())
  {
    m_vDestMpRtpAddr = *dest_mprtp_addr;
    m_bEnableMpRtp = true;
    VLOG(2) << "dest_mprtp_addr: " << ::toString(m_vDestMpRtpAddr);
  }

  auto force_dest_mprtp_addr = applicationParameters.getStringsParameter(app::ApplicationParameters::force_dest_mprtp_addr);
  if (force_dest_mprtp_addr && !force_dest_mprtp_addr->empty())
  {
    m_vForceDestMpRtpAddr = *force_dest_mprtp_addr;
    m_bEnableMpRtp = true;
    VLOG(2) << "Force dest_mprtp_addr: " << ::toString(m_vForceDestMpRtpAddr);
  }

  auto duration = applicationParameters.getUintParameter(app::ApplicationParameters::dur);
  if (duration)
  {
    m_uiDurationSeconds = *duration;
  }

  // configure handlers
  m_rtspClient.setOptionsHandler(boost::bind(&RtspClientSession::handleOptionsResponse, this, _1, _2, _3));
  m_rtspClient.setDescribeHandler(boost::bind(&RtspClientSession::handleDescribeResponse, this, _1, _2, _3));
  m_rtspClient.setSetupHandler(boost::bind(&RtspClientSession::handleSetupResponse, this, _1, _2, _3, _4));
  m_rtspClient.setPlayHandler(boost::bind(&RtspClientSession::handlePlayResponse, this, _1, _2));
  m_rtspClient.setTeardownHandler(boost::bind(&RtspClientSession::handleTeardownResponse, this, _1, _2));
}

RtspClientSession::~RtspClientSession()
{

}

boost::system::error_code RtspClientSession::start()
{
  // do OPTIONS
  boost::system::error_code ec = m_rtspClient.options(getSupported());
  return ec;
}

boost::system::error_code RtspClientSession::shutdown()
{
  m_bShuttingDown = true;
  switch (m_eState)
  {
    case STATE_INIT:
    {
      m_rtspClient.shutdown();
      break;
    }
    case STATE_PLAYING:
    {
      // the timer is only started once in the playing state
      m_timer.cancel();
      // fall through!
    }
    case STATE_SETUP:
    case STATE_PAUSED:
    {
      m_eState = STATE_TEARING_DOWN;
      boost::system::error_code ec = m_rtspClient.teardown();
      if (ec)
      {
        LOG(WARNING) << "Error in TEARDOWN: " << ec.message();
      }
      break;
    }
    case STATE_TEARING_DOWN:
    {
      // no need to do anything: teardown handler will end session
      break;
    }
  }
  return boost::system::error_code();
}

void RtspClientSession::endClientSession()
{
  VLOG(5) << "End of RTSP client session";
  if (m_onSessionEndHandler)
  {
    m_onSessionEndHandler();
  }
  else
  {
    LOG(WARNING) << "No session end handler configured";
  }
}

void RtspClientSession::verifyMpRtpAndRtpRtcpMuxSupportOnServer(const std::vector<std::string>& vSupported)
{
  for (const std::string& sSupported : vSupported)
  {
    if (sSupported == rfc2326::bis::SETUP_RTP_RTCP_MUX)
    {
      m_bDoesServerSupportRtpRtcpMux = true;
    }
    if (sSupported == mprtp::SETUP_MPRTP)
    {
      m_bDoesServerSupportMpRtp = true;
    }
  }
}

std::vector<std::string> RtspClientSession::getSupported()
{
  std::vector<std::string> vSupported;

  // check server for RTCP-mux support
  if (m_bRtcpMux)
  {
    vSupported.push_back(rfc2326::bis::SETUP_RTP_RTCP_MUX);
  }

  // check server for MPRTP support
  if (m_bEnableMpRtp)
  {
    vSupported.push_back(mprtp::SETUP_MPRTP);
  }
  return vSupported;
}

void RtspClientSession::handleOptionsResponse(const boost::system::error_code& ec, const std::vector<std::string>& vOptions, const std::vector<std::string>& vSupported)
{
  if (!ec)
  {
    LOG(INFO) << "RTSP SERVER OPTIONS: " << ::toString(vOptions) << " Supported: " << ::toString(vSupported, ',');
    verifyMpRtpAndRtpRtcpMuxSupportOnServer(vSupported);

    optional<bool> optionsOnly = m_applicationParameters.getBoolParameter(app::ApplicationParameters::options_only);

    if (!optionsOnly)
    {
      boost::system::error_code ec = m_rtspClient.describe(getSupported());
      if (ec)
      {
        LOG(WARNING) << "Error in describe: " << ec.message();
      }
    }
    else
    {
      // close RTSP client to end all outstanding asio handlers
      m_rtspClient.shutdown();
      endClientSession();
    }
  }
  else
  {
    LOG(WARNING) << "Error in OPTIONS: " << ec.message();
    endClientSession();
  }
}

void RtspClientSession::handleDescribeResponse(const boost::system::error_code& ec, const MessageBody& messageBody, const std::vector<std::string>& vSupported)
{
  if (!ec)
  {
    verifyMpRtpAndRtpRtcpMuxSupportOnServer(vSupported);
    LOG(INFO) << "RTSP SERVER DESCRIBE: " << messageBody.getMessageBody();

    optional<bool> describeOnly = m_applicationParameters.getBoolParameter(app::ApplicationParameters::describe_only);

    optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(messageBody.getMessageBody());
    if (sdp)
    {
      // at this point we could trigger a callback so that the caller can modify the SDP i.e. remove unwanted subsessions
      m_sdp = sdp;

      if (describeOnly)
      {
        // close RTSP client to end all outstanding asio handlers
        m_rtspClient.shutdown();
        endClientSession();
        return;
      }

      // TODO: select which sessions should be set up.
      // For now just select all of them
      bool bFailedToSetup = false;

      for (size_t i = 0; i < m_sdp->getMediaDescriptionCount(); ++ i)
      {
        const rfc4566::MediaDescription& descrip = m_sdp->getMediaDescription(i);

        if (m_applicationParameters.getBoolParameter(app::ApplicationParameters::audio_only) && descrip.getMediaType() != rfc4566::AUDIO)
        {
          continue;
        }
        else if (m_applicationParameters.getBoolParameter(app::ApplicationParameters::video_only) && descrip.getMediaType() != rfc4566::VIDEO)
        {
          continue;
        }

        optional<Transport> transport = selectTransportForMediaDescription(i, m_sdp->getMediaDescription(i));
        if (transport)
        {
          m_vSelectedTransports.push_back(std::make_tuple(i, m_sdp->getMediaDescription(i), *transport));
        }
        else
        {
          VLOG(2) << "Failed to select transport for media description " << i << " in SDP";
          bFailedToSetup = true;
          break;
        }
      }

      if (!bFailedToSetup)
      {
        setupSelectedSessions();
      }
      else
      {
        endClientSession();
      }
    }
    else
    {
      LOG(WARNING) << "Failed to parse SDP ";
      endClientSession();
    }
  }
  else
  {
    LOG(WARNING) << "Error in DESCRIBE: " << ec.message();
    endClientSession();
  }
}

boost::optional<Transport> RtspClientSession::selectTransportForMediaDescription(uint32_t uiIndexInSdp, rfc4566::MediaDescription& mediaDescription)
{
  DLOG(INFO) << "TODO: returns a vector of possible transports so that we can offer e/g both AVP and AVPF";

  // in this method simply accept any media lines
  optional<bool> rtpOverRtsp = m_applicationParameters.getBoolParameter(app::ApplicationParameters::use_rtp_rtsp);
  if (rtpOverRtsp)
  {
    Transport transport;
    transport.setTransportSpecifier(rfc3550::RTP_AVP_TCP);
    transport.setUnicast();
    // bind and allocate port and populate the transport header accordingly
    transport.setInterleavedStart(m_uiInterleaved);
    m_uiInterleaved += 2;
    return optional<Transport>(transport);
  }
  else
  {
    Transport transport;
    transport.setUnicast();

    // bind and allocate port and populate the transport header accordingly
    uint16_t uiRtpPort = m_uiRtpPort;
    if (isSessionUsingMpRtp())
    {
      // in the case of non-MPRTP we're going to support AVPF by default for now
      transport.setTransportSpecifier(rfc4585::RTP_AVPF);
      // check if the dest_mprtp_addr was specified as a command line parameter
      // in the case that it was, the setting overrides the actual value and may not coincide
      // with the port allocated by the port manager.
      // If it was not specified, then we generate the correct value now.
      bool bOverrideDestMpRtpAddr = uiIndexInSdp < m_vDestMpRtpAddr.size();
      bool bForceDestMpRtpAddr = uiIndexInSdp < m_vForceDestMpRtpAddr.size();
      
      std::string sDestMpRtpAddr = bOverrideDestMpRtpAddr ? m_vDestMpRtpAddr[uiIndexInSdp] : "";

      boost::system::error_code ec = m_mediaSessionNetworkManager.allocatePortsForMediaSession(uiRtpPort, isSessionUsingRtcpMux(), true, sDestMpRtpAddr);
      if (ec)
      {
        LOG(WARNING) << "Failed to bind port for media session " << ec.message();
        return optional<Transport>();
      }

      transport.setRtcpMux(isSessionUsingRtcpMux());
      // get port from first address
      MediaInterfaceDescriptor_t& mi = m_mediaSessionNetworkManager.getMediaDescriptor(uiIndexInSdp);
#if 1
      AddressDescriptor& rtp = std::get<0>(mi[0]); // primary interface
      // sanity check
      assert(rtp.getPort() == uiRtpPort);
#endif
      transport.setClientPortStart(uiRtpPort);

      std::string sDestMpRtpAddrForTransport = mprtp::getMpRtpRtspTransportString(mi);
      VLOG(2) << "dest_mprtp_addr for media description at index " << uiIndexInSdp << ": " << sDestMpRtpAddrForTransport;
      transport.setDestMpRtpAddr(sDestMpRtpAddrForTransport);

      if (bForceDestMpRtpAddr)
      {
        std::string& sDestMpRtpAddrForTransport = m_vForceDestMpRtpAddr[uiIndexInSdp];
        VLOG(2) << "dest_mprtp_addr forced for media description at index " << uiIndexInSdp << ": " << sDestMpRtpAddrForTransport;
        transport.setDestMpRtpAddr(sDestMpRtpAddrForTransport);
      }
    }
    else
    {
      boost::system::error_code ec = m_mediaSessionNetworkManager.allocatePortsForMediaSession(uiRtpPort, isSessionUsingRtcpMux(), false);
      if (ec)
      {
        LOG(WARNING) << "Failed to bind port for media session " << ec.message();
        return optional<Transport>();
      }
      else
      {
        VLOG(5) << "Bound port to : " << uiRtpPort;
      }
      // in the case of non-MPRTP we're going to support AVP by default for now
      transport.setTransportSpecifier(rfc3550::RTP_AVP);
      transport.setRtcpMux(isSessionUsingRtcpMux());
      transport.setClientPortStart(uiRtpPort);
    }
    return optional<Transport>(transport);
  }
}

void RtspClientSession::handleSetupResponse(const boost::system::error_code& ec, const std::string& sSession, const Transport& transport, const std::vector<std::string>& vSupported)
{
  if (!ec)
  {
    verifyMpRtpAndRtpRtcpMuxSupportOnServer(vSupported);

    m_eState = STATE_SETUP;
    VLOG(2) << "Session setup: " << sSession << " Transport: " << transport.toString();
    if (m_sSession.empty())
      m_sSession = sSession;

    // update selected transport
    auto& mediaTransport = m_vSelectedTransports[m_uiSetupCount];
    auto updatedMediaTransport = std::make_tuple(std::get<0>(mediaTransport), std::get<1>(mediaTransport), transport);
    m_vSelectedTransports[m_uiSetupCount] = updatedMediaTransport;

    ++m_uiSetupCount;
    if (!setupSelectedSessions())
    {
      VLOG(2) << "All media subsessions setup";
      // all sessions have been setup

      bool bOk = onSessionSetup(m_vSelectedTransports);
      if (bOk)
      {
        boost::system::error_code ec = m_rtspClient.play();
        if (ec)
        {
          LOG(WARNING) << "Error in PLAY: " << ec.message();
          shutdown();
        }
      }
      else
      {
        shutdown();
      }
    }
  }
  else
  {
    LOG(WARNING) << "Error in SETUP: " << ec.message();
    endClientSession();
  }
}

bool RtspClientSession::setupSelectedSessions()
{
  if (m_uiSetupCount < m_vSelectedTransports.size())
  {
    boost::system::error_code ec = m_rtspClient.setup(*m_sdp,
                                                      std::get<1>(m_vSelectedTransports.at(m_uiSetupCount)),
                                                      std::get<2>(m_vSelectedTransports.at(m_uiSetupCount)));
    if (ec)
    {
      LOG(WARNING) << "Error in SETUP: " << ec.message();
      return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    return false;
  }
}

bool RtspClientSession::isInterleavedMediaSession(const std::vector<MediaTransportDescription_t>& vSelectedTransports)
{
  // assume all transport is the same, checking first one suffices!
  return std::get<2>(vSelectedTransports[0]).isInterleaved();
}

bool RtspClientSession::setupRtpOverUdpSession(std::vector<MediaTransportDescription_t>& vSelectedTransports)
{
  // check if the user is using a 3rd party application for RTP/RTCP reception
  if (!m_applicationParameters.getBoolParameter(app::ApplicationParameters::external_rtp))
  {
    // create copy of SDP containing only setup media lines
    rfc4566::SessionDescription updatedSessionDescription = *m_sdp;
    // clear existing lines: we will re-add them
    updatedSessionDescription.clearMediaDescriptions();
    // There is one selected transport per m-line
    for (size_t i = 0; i < vSelectedTransports.size(); ++i)
    {
      MediaTransportDescription_t& rTransportDescription = vSelectedTransports[i];
      rfc4566::MediaDescription& mediaDescription = std::get<1>(rTransportDescription);
      Transport& transport = std::get<2>(rTransportDescription);

      LOG(INFO) << "Checking addresses: source: " << transport.getSource()
                << " Connection getIp var: " << m_rtspClient.getIpOrHost()
                << " Connection getServerIp var: " << m_rtspClient.getServerIp();

      mediaDescription.setConnection(rfc4566::ConnectionData(m_rtspClient.getServerIp()));
      // update ports in SDP: since we are using RTSP the server ports are in the transport
      mediaDescription.setPort(transport.getServerPortStart());

      // The rtcp-mux and mprtp might not be in the initial SDP from the server
      // we have to add them based on the server's SETUP response
      if (transport.getRtcpMux())
      {
        mediaDescription.addAttribute(rfc5761::RTCP_MUX, "");
      }

      if (transport.isUsingMpRtp())
      {
        // in SDP the server shouldn't provide the MPRTP interfaces since these will differ for each client
        // if the server were to provide them, this code would have to be updated.
        MediaInterfaceDescriptor_t mediaInterfaces = mprtp::getMpRtpInterfaceDescriptorFromRtspTransportString(transport.getSrcMpRtpAddr(), transport.getRtcpMux());
        for (const AddressTuple_t& addresses : mediaInterfaces)
        {
          const AddressDescriptor& rtp = std::get<0>(addresses);
          std::ostringstream mprtpInterfaceAttribute;
          mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << rtp.getAddress() << ":" << rtp.getPort();
          std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
          VLOG(2) << "Adding server MPRTP interface to SDP: " << sMpRtpInterface;
          mediaDescription.addAttribute(mprtp::MPRTP, sMpRtpInterface);
        }
      }
      else
      {
        // NON-MPRTP case: either standard RTSP, or RTP, but no interfaces have been specified
        VLOG(2) << "Setting up RTP over UDP MPRTP: " << m_bDoesServerSupportMpRtp
          << " Selected Transports: " << vSelectedTransports.size();
        // The port has already been set above.
      }

      // Note: again, if SDP already has extmaps: these may be added doubly?
      if (transport.hasExtmaps())
      {
        // transform each extmap into an "a=extmap:id name" line
        std::string sExtmaps = transport.getExtmaps();
        std::vector<std::string> vExtmaps = StringTokenizer::tokenize(sExtmaps, ";", true, true);
        for (const std::string& sExtmap : vExtmaps)
        {
          VLOG(2) << "Adding extmap to session " << sExtmap;
          mediaDescription.addAttribute(rfc5285::EXTMAP, sExtmap);
        }
      }

      updatedSessionDescription.addMediaDescription(mediaDescription);
    }

    InterfaceDescriptions_t localInterfaces = m_mediaSessionNetworkManager.getInterfaceDescriptors();
    MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), updatedSessionDescription, localInterfaces, false);
    SimpleMediaSessionFactory factory;
    // create adapter to port manager: this will be used to retrieve allocated UDP sockets
    PortAllocationManager& portManager = m_mediaSessionNetworkManager.getPortAllocationManager();
    ExistingConnectionAdapter adapter(&portManager);
    m_pMediaSession = factory.create(m_ioService, adapter, mediaSessionDescription, m_applicationParameters);
    if (!m_pMediaSession)
    {
      LOG(WARNING) << "Failed to create media session";
      return false;
    }

    // Moving out
#if 0
    if (m_pMediaSession->hasAudio())
    {
      boost::system::error_code ec = m_multimediaService.createAudioConsumer(m_pMediaSession->getAudioSource(), "AUDIO_",
                                                                             m_pMediaSession->getAudioSessionManager()->getPrimaryRtpSessionParameters());
      if (ec)
      {
        LOG(WARNING) << "Error creating audio consumer: " << ec.message();
        return false;
      }
    }

    if (m_pMediaSession->hasVideo())
    {
      boost::system::error_code ec = m_multimediaService.createVideoConsumer(m_pMediaSession->getVideoSource(), "VIDEO_",
                                                                             m_pMediaSession->getVideoSessionManager()->getPrimaryRtpSessionParameters());
      if (ec)
      {
        LOG(WARNING) << "Error creating video consumer: " << ec.message();
        return false;
      }
    }
    m_multimediaService.startServices();
#endif
    if (m_onSessionStartHandler)
      m_onSessionStartHandler(boost::system::error_code());

    m_pMediaSession->start();
    return true;
  }
  else
  {
    VLOG(5) << "external_rtp configured, not setting up media session: using 3rd party application for RTP/RTCP reception";
    return true;
  }
}

bool RtspClientSession::setupRtpOverTcpSession(std::vector<MediaTransportDescription_t>& vSelectedTransports)
{
  // create copy of SDP containing only setup media lines
  rfc4566::SessionDescription updatedSessionDescription = *m_sdp;
  // clear existing lines: we will re-add them
  updatedSessionDescription.clearMediaDescriptions();
  std::vector<Transport> transports;
  for (size_t i = 0; i < vSelectedTransports.size(); ++i)
  {
    MediaTransportDescription_t& rTransportDescription = vSelectedTransports[i];
    rfc4566::MediaDescription& mediaDescription = std::get<1>(rTransportDescription);
    Transport& transport = std::get<2>(rTransportDescription);
    assert(transport.isInterleaved());
    updatedSessionDescription.addMediaDescription(mediaDescription);
    transports.push_back(transport);
  }

  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), updatedSessionDescription, transports, false);
  SimpleMediaSessionFactory factory;

  // need adapter for existing connection
  ExistingConnectionAdapter adapter;
  adapter.setRtspClientConnection(m_rtspClient.getRtspClientConnection());

  m_pMediaSession = factory.create(m_ioService, adapter, mediaSessionDescription, m_applicationParameters);

  // Moving out
#if 0
  if (m_pMediaSession->hasAudio())
  {
    boost::system::error_code ec = m_multimediaService.createAudioConsumer(m_pMediaSession->getAudioSource(), "AUDIO_",
                                                                           m_pMediaSession->getAudioSessionManager()->getPrimaryRtpSessionParameters());
    if (ec)
    {
      LOG(WARNING) << "Error creating audio consumer: " << ec.message();
      return false;
    }
  }

  if (m_pMediaSession->hasVideo())
  {
    boost::system::error_code ec = m_multimediaService.createVideoConsumer(m_pMediaSession->getVideoSource(), "VIDEO_",
                                                                           m_pMediaSession->getVideoSessionManager()->getPrimaryRtpSessionParameters());
    if (ec)
    {
      LOG(WARNING) << "Error creating video consumer: " << ec.message();
      return false;
    }
  }
  m_multimediaService.startServices();
#endif

  if (m_onSessionStartHandler)
    m_onSessionStartHandler(boost::system::error_code());

  m_pMediaSession->setMediaSessionCompleteHandler(boost::bind(&RtspClientSession::onMediaSessionComplete, this));
  m_pMediaSession->start();
  return true;
}

bool RtspClientSession::onSessionSetup(std::vector<MediaTransportDescription_t>& vSelectedTransports)
{
  if (isInterleavedMediaSession(vSelectedTransports))
  {
    m_bInterleaved = true;
    // setup RTP over RTSP
    VLOG(5) << "Setting up RTP session (interleaved transport)";
    return setupRtpOverTcpSession(vSelectedTransports);
  }
  else
  {
    m_bInterleaved = false;
    // setup RTP over UDP
    VLOG(5) << "Setting up RTP session (standard transport)";
    return setupRtpOverUdpSession(vSelectedTransports);
  }
}

void RtspClientSession::onTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    if (m_eState == STATE_PLAYING)
    {
      // check if we're still in the playing state, if so teardown
      VLOG(2) << "Session duration reached: " << m_uiDurationSeconds << "s";
      endClientSession();
    }
    else
    {
      VLOG(2) << "Session duration reached (" << m_uiDurationSeconds << "s) but not playing anymore " << m_eState;
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Error on timeout: " << ec.message();
    }
  }
}

boost::system::error_code RtspClientSession::registerAudioConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingAudio)
{
  if (m_pMediaSession && m_pMediaSession->hasAudio())
  {
    m_pMediaSession->getAudioSessionManager()->registerMediaCallback(onIncomingAudio);
    return boost::system::error_code();
  }
  else
  {
    return boost::system::error_code(boost::system::errc::no_such_file_or_directory, boost::system::get_generic_category());
  }
}

boost::system::error_code RtspClientSession::registerVideoConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingVideo)
{
  if (m_pMediaSession && m_pMediaSession->hasVideo())
  {
    m_pMediaSession->getVideoSessionManager()->registerMediaCallback(onIncomingVideo);
    return boost::system::error_code();
  }
  else
  {
    return boost::system::error_code(boost::system::errc::no_such_file_or_directory, boost::system::get_generic_category());
  }
}

void RtspClientSession::handlePlayResponse(const boost::system::error_code& ec, const std::vector<std::string>& vSupported)
{
  if (!ec)
  {
    m_eState = STATE_PLAYING;
    LOG(INFO) << "Session " << m_sSession << " playing";
    // check if the session duration has been set
    if (m_uiDurationSeconds > 0)
    {
      m_timer.expires_from_now(boost::posix_time::seconds(m_uiDurationSeconds));
      m_timer.async_wait(boost::bind(&RtspClientSession::onTimeout, this, _1));
    }
  }
  else
  {
    LOG(WARNING) << "Error in PLAY: " << ec.message();
    shutdown();
  }
}

void RtspClientSession::handleTeardownResponse(const boost::system::error_code& ec, const std::vector<std::string>& vSupported)
{
  VLOG(2) << "Session " << m_sSession << " torndown";
  // Note: should teardown always shutdow the connection
  m_eState = STATE_INIT;

  // allow subclasses to process event
  onSessionTeardown();

  // TODO: if RTP over RTSP interleaved wait for RTCP to be sent before closing socket
  if (m_bShuttingDown && !m_bInterleaved)
  {
    m_rtspClient.shutdown();
  }

  if (ec)
  {
    LOG(WARNING) << "Error in TEARDOWN: " << ec.message();
  }
}

void RtspClientSession::onSessionTeardown()
{
  VLOG(5) << "Stopping multimedia services";
  if (m_pMediaSession)
    m_pMediaSession->stop();
  //m_multimediaService.stopServices();
}

void RtspClientSession::onMediaSessionComplete()
{
  VLOG(5) << "onMediaSessionComplete";
  m_rtspClient.shutdown();
  // end client session
  endClientSession();
}

} // rfc2326
} // rtp_plus_plus

