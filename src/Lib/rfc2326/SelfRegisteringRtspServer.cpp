#include "CorePch.h"
#include <rtp++/rfc2326/SelfRegisteringRtspServer.h>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <cpputil/Conversion.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/NetworkInterfaceUtil.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/LiveServerMediaSession.h>
#include <rtp++/rfc2326/Rfc2326.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc5285/Rfc5285.h>
#include <rtp++/rfc5761/Rfc5761.h>

namespace rtp_plus_plus
{

using namespace rfc2326;
using media::MediaSample;

const uint16_t STARTING_SERVER_MEDIA_PORT = 49170;
const uint32_t MAX_RETRY_SECONDS = 256;

SelfRegisteringRtspServer::SelfRegisteringRtspServer(boost::asio::io_service& ioService,
                                                     const GenericParameters& applicationParameters,
                                                     const std::string& sSdp,
                                                     const std::string& sRtspProxyOrClientAddressDescriptor, 
                                                     bool bReuseRegisterConnection, 
                                                     const std::vector<std::string>& vLocalInterfaces,
                                                     boost::system::error_code& ec,
                                                     unsigned short uiPort, 
                                                     const std::string& sStreamName,
                                                     const std::vector<std::string>& vSrcMpRtpAddr,
                                                     const std::vector<std::string>& vForceSrcMpRtpAddr,
                                                     bool bInterleavedTransport,
                                                     bool bRtcpMux, bool bEnableMpRtp, unsigned uiSessionTimeout)
  :RtspServer(ioService, applicationParameters, ec, uiPort, uiSessionTimeout),
    m_uiNextServerTransportPort(STARTING_SERVER_MEDIA_PORT),
    m_sSdp(sSdp),
    m_sRtspProxyOrClientAddressDescriptor(sRtspProxyOrClientAddressDescriptor),
    m_bReuseRegisterConnection(bReuseRegisterConnection),
    m_vLocalInterfaces(vLocalInterfaces),
    m_sStreamName(sStreamName),
    m_vSrcMpRtpAddr(vSrcMpRtpAddr),
    m_vForceSrcMpRtpAddr(vForceSrcMpRtpAddr),
    m_bInterleaved(bInterleavedTransport),
    m_bRtcpMux(bRtcpMux),
    m_bEnableMpRtp(bEnableMpRtp),
    m_registerTimer(getIoService()),
    m_uiLastSN(0),
    m_uiRetrySeconds(2),
    m_uiAudioPayloadType(0),
    m_uiVideoPayloadType(0)
{
  if (ec)
  {
    LOG(WARNING) << "Failed to initialise RTSP server";
    return;
  }

  // update local interfaces
  ec = m_mediaSessionNetworkManager.setLocalInterfaces(vLocalInterfaces);
  if (ec)
  {
    LOG(WARNING) << "Failed to update local interfaces";
    return;
  }

  // check SDP
  boost::optional<rfc4566::SessionDescription> sessionDescription = rfc4566::SdpParser::parse(m_sSdp);
  if (!sessionDescription)
  {
    LOG(WARNING) << "Failed to parse SDP";
    ec = boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    return;
  }
  else
  {
    m_sessionDescription = *sessionDescription;
    // determine audio and video payload types: very simplistic approach.
    // This would not work if there were multiple audio or video m-lines in the SDP.
    for (size_t i = 0; i < m_sessionDescription.getMediaDescriptionCount(); ++i)
    {
      if (m_sessionDescription.getMediaDescription(i).getMediaType() == rfc4566::AUDIO)
      {
        // also assuming that an m-line has a single format
        assert(m_sessionDescription.getMediaDescription(i).getFormats().size() == 1);
        std::string sFormat = m_sessionDescription.getMediaDescription(i).getFormat(0);
        bool bDummy;
        m_uiAudioPayloadType = convert<uint8_t>(sFormat, bDummy);
        assert(bDummy);
      }
      else if (m_sessionDescription.getMediaDescription(i).getMediaType() == rfc4566::VIDEO)
      {
        // also assuming that an m-line has a single format
        assert(m_sessionDescription.getMediaDescription(i).getFormats().size() == 1);
        std::string sFormat = m_sessionDescription.getMediaDescription(i).getFormat(0);
        bool bDummy;
        m_uiVideoPayloadType = convert<uint8_t>(sFormat, bDummy);
        assert(bDummy);
      }
      else
      {
        LOG(WARNING) << "Unsupported media type in SDP: " << m_sessionDescription.getMediaDescription(i).getMediaType();
      }
    }
  }

  for (const std::string& sInterface : m_vLocalInterfaces)
  {
    VLOG(2) << "RTSP server interface: " << sInterface;
  }

  // configure local URI
  m_rtspUri = RtspUri(sStreamName, "", m_vLocalInterfaces[0], uiPort);

  // validate src_mprtp_addr. If this parameter is specified
  // we try to bind to the ports immediately returning an 
  // error code if this is not possible
  ec = validateSrcMpRtpAddr();
  if (ec) return;

  // validate forced src_mprtp_addr
  ec = validateForcedSrcMpRtpAddr();
  if (ec) return;
}

boost::system::error_code SelfRegisteringRtspServer::validateSrcMpRtpAddr()
{
  // validate src_mprtp_addr
  if (!m_vSrcMpRtpAddr.empty())
  {
    // make sure the number of specified src_mprtp_addr fields matches the number of m-lines
    if (m_sessionDescription.getMediaDescriptionCount() != m_vSrcMpRtpAddr.size())
    {
      LOG(WARNING) << "Number of src_mprtp_addr does not match m-line count in SDP.";
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    }
    // parse header and make sure that the interfaces are valid
    for (const std::string& sSrcMprtpAddr : m_vSrcMpRtpAddr)
    {
      std::vector<std::string> vSrcMpRtpAddrInterfaces = StringTokenizer::tokenize(sSrcMprtpAddr, ";", true, true);
      if (vSrcMpRtpAddrInterfaces.empty())
      {
        LOG(WARNING) << "Invalid src_mprtp_addr header: " << sSrcMprtpAddr;
        return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
      }
      else
      {
        // This is a string of form "<counter> <address> <port>"
        for (const std::string& sInterface : vSrcMpRtpAddrInterfaces)
        {
          std::vector<std::string> vInterfaceInfo = StringTokenizer::tokenize(sInterface, " ", true, true);
          if (vInterfaceInfo.size() != 3)
          {
            LOG(WARNING) << "Invalid interface in src_mprtp_addr header: " << sInterface;
            return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
          }
          else
          {
            // check if interface is valid
            const std::string& sAddress = vInterfaceInfo[1];
            auto it = std::find_if(m_vLocalInterfaces.begin(), m_vLocalInterfaces.end(), [&sAddress](const std::string& sLocalInterface)
            {
              return sLocalInterface == sAddress;
            });
            if (it == m_vLocalInterfaces.end())
            {
              LOG(WARNING) << "Invalid interface: " << sInterface;
              return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
            }
            else
            {
              VLOG(2) << "src_mprtp_addr Interface validated: " << sInterface;
              // now try to bind to port
              bool bSuccess = true;
              uint16_t uiRtpPort = convert<uint16_t>(vInterfaceInfo[2], bSuccess);
              if (!bSuccess)
              {
                LOG(WARNING) << "Invalid port in interface: " << sInterface << ":" << uiRtpPort;
                return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
              }
            }
          }
        }
      }
    }
  }
  return boost::system::error_code();
}

boost::system::error_code SelfRegisteringRtspServer::validateForcedSrcMpRtpAddr()
{
  if (!m_vForceSrcMpRtpAddr.empty())
  {
    // make sure the number of specified src_mprtp_addr fields matches the number of m-lines
    if (m_sessionDescription.getMediaDescriptionCount() != m_vForceSrcMpRtpAddr.size())
    {
      LOG(WARNING) << "Number of forced src_mprtp_addr does not match m-line count in SDP.";
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    }
  }
  return boost::system::error_code();
}

boost::system::error_code SelfRegisteringRtspServer::doStart()
{
  boost::system::error_code ec;
  // if the connection is to be re-used, we don't start accepting
  // connections: we only support a single client!
  if (!m_bReuseRegisterConnection)
  {
    ec = RtspServer::doStart();
    if (ec)
    {
      return ec;
    }
  }

  connectToRtspProxyOrClient(m_sRtspProxyOrClientAddressDescriptor);
  return ec;
}

boost::system::error_code SelfRegisteringRtspServer::doShutdown()
{
  VLOG(2) << "Shutting down RTSP server";

  boost::system::error_code ec = RtspServer::doShutdown();
  if (ec)
  {
    LOG(WARNING) << "Error shutting down RTSP server: " << ec.message();
    return ec;
  }

  // stop register timer if running
  m_registerTimer.cancel();

  if (m_bReuseRegisterConnection)
  {
    if (m_pConnection)
      m_pConnection->close();
  }
  return ec;
}

void SelfRegisteringRtspServer::connectToRtspProxyOrClient(const std::string& sRtspProxyOrClientAddressDescriptor)
{
  VLOG(10) << "Creating RTSP connection for REGISTER request";
  RtspUri uri(sRtspProxyOrClientAddressDescriptor);
  SelfRegisteringRtspServerConnection::ptr pNewConnection = SelfRegisteringRtspServerConnection::create(getIoService(), uri.getServerIp(), uri.getPort());
  pNewConnection->setReadCompletionHandler(boost::bind(&SelfRegisteringRtspServer::readCompletionHandler, this, boost::asio::placeholders::error, _2, _3));
  pNewConnection->setWriteCompletionHandler(boost::bind(&SelfRegisteringRtspServer::writeCompletionHandler, this, boost::asio::placeholders::error, _2, _3, _4));
  pNewConnection->setCloseHandler(boost::bind(&SelfRegisteringRtspServer::closeCompletionHandler, this, _1));
  pNewConnection->setConnectErrorHandler(boost::bind(&SelfRegisteringRtspServer::connectErrorHandler, this, _1, _2));

  // start connect
  pNewConnection->connect();

  std::string sRegisterRequest = "REGISTER <<RtspUri>> RTSP/1.0\r\n"\
    "CSeq: <<CSeq>>\r\n"\
    "Transport: <<Transport>>\r\n\r\n";

  std::string sTransport = "<<reuse_connection>>preferred_delivery_protocol=<<preferred_delivery_protocol>>;proxy_url_suffix=<<proxy_url_suffix>>";
  boost::algorithm::replace_first(sTransport, "<<reuse_connection>>", m_bReuseRegisterConnection ? "reuse_connection;" : "");
  boost::algorithm::replace_first(sTransport, "<<preferred_delivery_protocol>>", m_bInterleaved ? "interleaved" : "udp");
  boost::algorithm::replace_first(sTransport, "<<proxy_url_suffix>>", m_rtspUri.getName());

  boost::algorithm::replace_first(sRegisterRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sRegisterRequest, "<<CSeq>>", ::toString(m_uiLastSN++));
  boost::algorithm::replace_first(sRegisterRequest, "<<Transport>>", sTransport);
  boost::algorithm::replace_first(sRegisterRequest, "<<UserAgent>>", getServerInfo());
  // this write will occur once a connection has been established
  pNewConnection->write(m_uiLastSN, sRegisterRequest);
  DLOG(INFO) << "TODO: store outgoing requests by SN so that we can can associate an incoming response with a previous request.";
}

std::unique_ptr<ServerMediaSession> SelfRegisteringRtspServer::createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType)
{
  return std::unique_ptr<ServerMediaSession>(new LiveServerMediaSession(*this, sSession, sSessionDescription, sContentType, m_uiSessionTimeout));
}

void SelfRegisteringRtspServer::connectErrorHandler(const boost::system::error_code& ec, SelfRegisteringRtspServerConnection::ptr pConnection)
{
  // schedule another connect in x seconds
  pConnection->close();
  if (!isShuttingDown())
  {
    VLOG(2) << "Failed to connect, retrying in " << m_uiRetrySeconds << "s";
    m_registerTimer.expires_from_now(boost::posix_time::seconds(m_uiRetrySeconds));
    m_registerTimer.async_wait(boost::bind(&SelfRegisteringRtspServer::handleTimeout, this, _1));
    if (m_uiRetrySeconds < MAX_RETRY_SECONDS)
      m_uiRetrySeconds = m_uiRetrySeconds * 2;
  }
}

void SelfRegisteringRtspServer::handleTimeout(const boost::system::error_code& ec)
{
  if (!ec && !isShuttingDown())
  {
    // create new connection
    connectToRtspProxyOrClient(m_sRtspProxyOrClientAddressDescriptor);
  }
  else
  {
    VLOG(5) << "Shutdown imminent";
  }
}

bool SelfRegisteringRtspServer::isSessionSetup() const
{
  return activeSessionCount() >= 1;
}

bool SelfRegisteringRtspServer::sendVideo(const media::MediaSample& mediaSample)
{
  if (isSessionSetup())
  {
    m_ioService.post(boost::bind(&SelfRegisteringRtspServer::doSendVideo, this, mediaSample));
    return true;
  }
  else
    return false;
}

bool SelfRegisteringRtspServer::sendVideo(const std::vector<media::MediaSample>& mediaSamples)
{
  if (isSessionSetup())
  {
    m_ioService.post(boost::bind(&SelfRegisteringRtspServer::doSendVideoSamples, this, mediaSamples));
    return true;
  }
  else
    return false;
}

bool SelfRegisteringRtspServer::sendAudio(const media::MediaSample& mediaSample)
{
  if (isSessionSetup())
  {
    m_ioService.post(boost::bind(&SelfRegisteringRtspServer::doSendAudio, this, mediaSample));
    return true;
  }
  else
    return false;
}

bool SelfRegisteringRtspServer::sendAudio(const std::vector<media::MediaSample>& mediaSamples)
{
  if (isSessionSetup())
  {
    m_ioService.post(boost::bind(&SelfRegisteringRtspServer::doSendAudioSamples, this, mediaSamples));
    return true;
  }
  else
    return false;
}

void SelfRegisteringRtspServer::doSendVideo(const media::MediaSample& mediaSample)
{
  assert(m_uiVideoPayloadType != 0);
  // get session and send video in session
  boost::mutex::scoped_lock l(m_sessionLock);
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    if (it->second->isPlaying())
      it->second->deliver(m_uiVideoPayloadType, mediaSample);
  }
}

void SelfRegisteringRtspServer::doSendVideoSamples(const std::vector<media::MediaSample>& mediaSamples)
{
  assert(m_uiVideoPayloadType != 0);
  // get session and send video in session
  boost::mutex::scoped_lock l(m_sessionLock);
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    if (it->second->isPlaying())
      it->second->deliver(m_uiVideoPayloadType, mediaSamples);
  }
}

void SelfRegisteringRtspServer::doSendAudio(const media::MediaSample& mediaSample)
{
  assert(m_uiAudioPayloadType != 0);
  // get session and send audio in session
  boost::mutex::scoped_lock l(m_sessionLock);
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    if (it->second->isPlaying())
      it->second->deliver(m_uiAudioPayloadType, mediaSample);
  }
}

void SelfRegisteringRtspServer::doSendAudioSamples(const std::vector<media::MediaSample>& mediaSamples)
{
  assert(m_uiAudioPayloadType != 0);
  // get session and send audio in session
  boost::mutex::scoped_lock l(m_sessionLock);
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    if (it->second->isPlaying())
      it->second->deliver(m_uiAudioPayloadType, mediaSamples);
  }
}

void SelfRegisteringRtspServer::handleResponse(const RtspMessage& rtspResponse, RtspServerConnectionPtr pConnection)
{
  DLOG(INFO) << "TODO: match to REGISTER and check result of REGISTER request";
  DLOG(INFO) << "TODO: if an error occurred, schedule later retry?";

  if (!m_bReuseRegisterConnection)
  {
    VLOG(2) << "Got response from RTSP proxy or client: " << rtspResponse.getResponseCode() << ". Closing RTSP connection used for REGISTER request";
    pConnection->close();
  }
  else
  {
    VLOG(2) << "Got response from RTSP proxy or client: " << rtspResponse.getResponseCode() << ". Reusing REGISTER connection. NOT closing";
    // store connection so that we can close it on shutdown
    m_pConnection = boost::static_pointer_cast<SelfRegisteringRtspServerConnection>(pConnection);
  }
}

rfc2326::ResponseCode SelfRegisteringRtspServer::handleOptions(std::set<rfc2326::RtspMethod>& methods, const std::vector<std::string>& vSupported) const
{
  methods.insert(OPTIONS);
  methods.insert(DESCRIBE);
  methods.insert(SETUP);
  methods.insert(PLAY);
  methods.insert(TEARDOWN);
  return OK;
}

rfc2326::ResponseCode SelfRegisteringRtspServer::handleSetup(const rfc2326::RtspMessage& setup, std::string& sSession,
  rfc2326::Transport& transport, const std::vector<std::string>& vSupported)
{
  // we only allow one RTSP session at a time
  if (isSessionSetup())
  {
    return SERVICE_UNAVAILABLE;
  }
  rfc2326::ResponseCode code = RtspServer::handleSetup(setup, sSession, transport, vSupported);
  // store session id: we need this to access the single RTSP session
  DLOG(INFO) << "TODO: what happens when more than one session is registered?!?";
  m_sSession = sSession;
  return code;
}

rfc2326::ResponseCode SelfRegisteringRtspServer::handleTeardown(const rfc2326::RtspMessage& teardown, const std::vector<std::string>& vSupported)
{
  ResponseCode eCode = RtspServer::handleTeardown(teardown, vSupported);
  // reset session ID
  m_sSession = "";
  return eCode;
}

bool SelfRegisteringRtspServer::checkResource(const std::string& sResource)
{
  // check if resource name matches name of RTSP URI
  RtspUri resourceUri(sResource);
  if (resourceUri.getName() == m_rtspUri.getName())
    return true;
  else if (resourceUri.getRelativePath() == m_rtspUri.getName())
    return true;
  else
    return false;
}

bool SelfRegisteringRtspServer::isOptionSupportedByServer(const std::string& sOptionOrFeature) const
{
  if ((sOptionOrFeature == mprtp::SETUP_MPRTP && m_bEnableMpRtp) ||
    (sOptionOrFeature == rfc2326::bis::SETUP_RTP_RTCP_MUX && m_bRtcpMux))
    return true;
  return false;
}

std::vector<std::string> SelfRegisteringRtspServer::getUnsupported(const rfc2326::RtspMessage& rtspRequest)
{
  std::vector<std::string> vUnsupported;
  // we only support mprtp and rtcp-mux
  boost::optional<std::string> require = rtspRequest.getHeaderField(rfc2326::REQUIRE);
  if (require)
  {
    std::vector<std::string> vRequired = StringTokenizer::tokenize(*require, ",", true, true);
    for (size_t i = 0; i < vRequired.size(); ++i)
    {
      if (vRequired[i] == mprtp::SETUP_MPRTP)
      {
        if (!m_bEnableMpRtp)
        {
          vUnsupported.push_back(vRequired[i]);
        }
      }
      else if (vRequired[i] == rfc2326::bis::SETUP_RTP_RTCP_MUX)
      {
        if (!m_bRtcpMux)
        {
          vUnsupported.push_back(vRequired[i]);
        }
      }
      else
      {
        vUnsupported.push_back(vRequired[i]);
      }
    }
  }
  return vUnsupported;
}

bool SelfRegisteringRtspServer::isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes)
{
  auto it = std::find_if(vAcceptedTypes.begin(), vAcceptedTypes.end(), [](const std::string& sAccept)
  {
    return sAccept == rfc4566::APPLICATION_SDP;
  });
  return (it != vAcceptedTypes.end());
}

bool SelfRegisteringRtspServer::getResourceDescription(const std::string& sResource, std::string& sSessionDescription, 
  std::string& sContentType, const std::vector<std::string>& vSupported, const Transport& transport)
{
  VLOG(5) << "Getting resource description for " << sResource;
  RtspUri resourceUri(sResource);
  assert(resourceUri.getName() == m_rtspUri.getName() || resourceUri.getRelativePath() == m_rtspUri.getName());

  // we need to update the SDP if the client requests MPRTP and/or RTCP-mux
  boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(m_sSdp);
  assert(sdp);
  // add additional options based on what the client supports
  // check for rtcp-mux support
  auto it = std::find_if(vSupported.begin(), vSupported.end(), [](const std::string& sSupported)
  {
    return sSupported == bis::SETUP_RTP_RTCP_MUX;
  });
  if (it != vSupported.end() || transport.getRtcpMux())
  {
    VLOG(2) << "Client supports rtcp-mux, adding rtcp-mux to SDP";
    // add rtcp-mux to SDP
    for (size_t i = 0; i < sdp->getMediaDescriptionCount(); ++i)
    {
      sdp->getMediaDescription(i).addAttribute(rfc5761::RTCP_MUX, "");
    }
  }

  it = std::find_if(vSupported.begin(), vSupported.end(), [](const std::string& sSupported)
  {
    return sSupported == mprtp::SETUP_MPRTP;
  });
  if (it != vSupported.end() || transport.isUsingMpRtp())
  {
    VLOG(2) << "Client supports MPRTP, adding MPRTP to SDP";
    // add mprtp to SDP
    for (size_t i = 0; i < sdp->getMediaDescriptionCount(); ++i)
    {
      sdp->getMediaDescription(i).addAttribute(mprtp::MPRTP, "");
      // add extmap
      const uint32_t MPRTP_EXT_MAP_ID = 1;
      std::string sExtmapValue = ::toString(MPRTP_EXT_MAP_ID) + " " + mprtp::EXTENSION_NAME_MPRTP;
      sdp->getMediaDescription(0).addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }
  }

  sSessionDescription = sdp->toString();
  sContentType = rfc4566::APPLICATION_SDP;

  return true;
}

boost::optional<rfc2326::Transport> SelfRegisteringRtspServer::selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports)
{
  // This server only supports unicast, RTP/AVP and AVPF
  for (size_t i = 0; i < vTransports.size(); ++i)
  {
    rfc2326::Transport transport(vTransports[i]);
    if (transport.isValid())
    {
      if ((transport.getSpecifier() == "RTP/AVP") ||
        (transport.getSpecifier() == "RTP/AVPF") ||
        (transport.getSpecifier() == "RTP/AVP/UDP") ||
        (transport.getSpecifier() == "RTP/AVPF/UDP"))
      {
        if (!transport.isUnicast())
        {
          VLOG(2) << "Multicast unsupported";
          return boost::optional<Transport>();
        }

        if (transport.isInterleaved())
        {
          VLOG(2) << "TODO: Interleaving unsupported";
          return boost::optional<Transport>();
        }

        if (transport.getRtcpMux())
        {
          if (!m_bRtcpMux)
          {
            VLOG(2) << "RTCP-mux unsupported";
            return boost::optional<Transport>();
          }
        }

        if (transport.isUsingMpRtp())
        {
          if (!m_bEnableMpRtp)
          {
            VLOG(2) << "MPRTP unsupported";
            return boost::optional<Transport>();
          }
        }
        return boost::optional<Transport>(transport);
      }
      else
      {
        VLOG(2) << "Unsupported transport specifier " << transport.getSpecifier();
      }
    }
  }
  return boost::optional<Transport>();
}

boost::system::error_code SelfRegisteringRtspServer::getServerTransportInfo(const RtspUri& rtspUri, const std::string& sSession, rfc2326::Transport& transport)
{
  uint16_t uiRtpPort = m_uiNextServerTransportPort;

  bool bUseMpRtp = m_bEnableMpRtp && transport.isUsingMpRtp();
  bool bUseRtcpMux = m_bRtcpMux && transport.getRtcpMux();

  if (bUseMpRtp)
  {
    // TODO: remove extmap from here?!? We ar esetting it in getResourceDescription based on Supported and Transport
    // we also have to set the MPRTP extmap
    DLOG(INFO) << "TODO: we should get the MPRTP ext map ID from the MPRTP session";
    const uint32_t MPRTP_EXT_MAP_ID = 1;
    transport.setExtmap(MPRTP_EXT_MAP_ID, mprtp::EXTENSION_NAME_MPRTP);

    // HACK FOR NOW: there will only be one m-line per SDP
    // if there are audio and vide at a later stage then we
    // need to determine the index in the SDP of the current uri and
    // then retrieve the correct index in the vector m_vForceSrcMpRtpAddr
    assert(m_vForceSrcMpRtpAddr.size() <= 1);
    assert(m_vSrcMpRtpAddr.size() <= 1);

    bool bOverrideSrcMpRtpAddr = !m_vSrcMpRtpAddr.empty();
    bool bForceSrcMpRtpAddr = !m_vForceSrcMpRtpAddr.empty();
    // Note: this only works for a single session setup for now
    const uint32_t uiIndex = 0;

    std::string sSrcMpRtpAddr = bOverrideSrcMpRtpAddr ? m_vSrcMpRtpAddr[uiIndex] : "";

    boost::system::error_code ec = m_mediaSessionNetworkManager.allocatePortsForMediaSession(sSession, uiRtpPort, bUseRtcpMux, true, sSrcMpRtpAddr);
    if (ec)
    {
      LOG(WARNING) << "Failed to bind port for media session " << ec.message();
      return ec;
    }
    else
    {
      VLOG(5) << "Bound ports starting at " << uiRtpPort;
    }

    transport.setRtcpMux(bUseRtcpMux);
    // bound port has been set
    transport.setServerPortStart(uiRtpPort);

    MediaInterfaceDescriptor_t& mi = m_mediaSessionNetworkManager.getMediaDescriptor(sSession, uiIndex);
    std::string sSrcMpRtpAddrForTransport = mprtp::getMpRtpRtspTransportString(mi);
    VLOG(5) << "src_mprtp_addr for media description at index " << uiIndex << ": " << sSrcMpRtpAddrForTransport;
    transport.setSrcMpRtpAddr(sSrcMpRtpAddrForTransport);

    if (bForceSrcMpRtpAddr)
    {
      std::string& sSrcMpRtpAddrForTransport = m_vForceSrcMpRtpAddr[uiIndex];
      VLOG(2) << "src_mprtp_addr forced for media description at index " << uiIndex << ": " << sSrcMpRtpAddrForTransport;
      transport.setSrcMpRtpAddr(sSrcMpRtpAddrForTransport);
    }
    // store server transport
    m_mServerTransports[sSession].push_back(transport);
  }
  else // single path transport
  {
    boost::system::error_code ec = m_mediaSessionNetworkManager.allocatePortsForMediaSession(sSession, uiRtpPort, bUseRtcpMux, false);
    if (ec)
    {
      LOG(WARNING) << "Failed to bind port for single path media session " << ec.message();
      return ec;
    }
    else
    {
      VLOG(5) << "Bound ports starting at " << uiRtpPort;
    }

    // in the case of non-MPRTP we're going to support AVP by default for now
    transport.setTransportSpecifier(rfc3550::RTP_AVP);
    transport.setRtcpMux(bUseRtcpMux);
    transport.setServerPortStart(uiRtpPort);
    m_mServerTransports[sSession].push_back(transport);
  } // single path transport
  return boost::system::error_code();
}

void SelfRegisteringRtspServer::onShutdown()
{
  // TODO
}

} // rtp_plus_plus

