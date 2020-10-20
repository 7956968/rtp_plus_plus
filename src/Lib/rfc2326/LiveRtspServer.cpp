#include "CorePch.h"
#include <rtp++/rfc2326/LiveRtspServer.h>
#include <functional>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/LiveServerMediaSession.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

using media::MediaSample;

const uint16_t LiveRtspServer::DEFAULT_SERVER_MEDIA_PORT = 49170;

LiveRtspServer::LiveRtspServer(const std::string& sLiveStreamName,
                               std::shared_ptr<media::IVideoDevice> pVideoDevice,
                               boost::asio::io_service& ioService,
                               const GenericParameters& applicationParameters,
                               boost::system::error_code& ec,
                               unsigned short uiPort, unsigned short uiRtpPort, 
                               unsigned uiSessionTimeout)
  :RtspServer(ioService, applicationParameters, ec, uiPort, uiSessionTimeout),
    m_sLiveStreamName(sLiveStreamName),
    m_pVideoDevice(pVideoDevice),
    m_uiNextServerTransportPort(uiRtpPort)
{
  m_pVideoDevice->setReceiveAccessUnitCB(std::bind(&LiveRtspServer::accessUnitCB, this, std::placeholders::_1));

  std::ostringstream ostr;
  ostr << getRtspServerAddress() << "/" << m_sLiveStreamName;
  LOG(INFO) << "Stream can be accessed at " << ostr.str();

  m_pVideoDevice->start();
}

std::unique_ptr<ServerMediaSession> LiveRtspServer::createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType)
{
  return std::unique_ptr<ServerMediaSession>( new LiveServerMediaSession(*this, sSession, sSessionDescription, sContentType, m_uiSessionTimeout));
}

ResponseCode LiveRtspServer::handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const
{
  methods.insert(OPTIONS);
  methods.insert(DESCRIBE);
  methods.insert(SETUP);
  methods.insert(PLAY);
  methods.insert(TEARDOWN);
  return OK;
}

bool LiveRtspServer::checkResource(const std::string& sResource)
{
  // HACK for now: there is only one live resource
  // parse RTSP URI into components
  RtspUri rtspUri(sResource);

  VLOG(10) << "Checking RTSP URI: " << rtspUri.fullUri()
          << " Host URI: " << rtspUri.hostUri()
          << " Name: " << rtspUri.getName()
          << " Relative: " << rtspUri.getRelativePath()
          << " Server: " << rtspUri.getServerIp()
          << " Port: " << rtspUri.getPort();

  if ((rtspUri.getName() == m_sLiveStreamName) ||
      (rtspUri.getRelativePath() == m_sLiveStreamName && rtspUri.getName() == "track1")
     )
  {
    VLOG(10) << "Resource " << sResource << " ok";
    return true;
  }
  VLOG(10) << "Resource " << sResource << " NOT ok";
  return false;
}

bool LiveRtspServer::isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes)
{
  auto it = std::find_if(vAcceptedTypes.begin(), vAcceptedTypes.end(), [](const std::string& sAccept)
  {
    return sAccept == rfc4566::APPLICATION_SDP;
  });
  return (it != vAcceptedTypes.end());
}

bool LiveRtspServer::getResourceDescription(const std::string& sResource, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported, const Transport& transport)
{
  // TODO: create SDP for live media session with video and audio
  VLOG(5) << "Getting resource description for " << sResource;

  sSessionDescription = "v=0\r\n"\
      "o=- <<NtpTime>> 1 IN IP4 <<Ip>>\r\n"\
      "s=Live media streamed by rtp++\r\n"\
      "i=<<StreamName>>\r\n"\
      "t=0 0\r\n"\
      "a=tool:<<Software>>\r\n"\
      "a=control:*\r\n"\
      "a=range:npt=now-\r\n"\
      "m=video 0 RTP/AVP 96\r\n"\
      "c=IN IP4 0.0.0.0\r\n"\
      "b=AS:96\r\n"\
      "a=rtpmap:96 H264/90000\r\n"\
      "a=fmtp:96 profile-level-id=<<ProfileLevelId>>;packetization-mode=1;sprop-parameter-sets=<<SPropParameterSets>>\r\n"\
      "a=control:track1\r\n";

  uint64_t uiNtpTime = RtpTime::getNTPTimeStamp();

  std::string sProfileLevelId, sSPropParameterSets;
  boost::system::error_code ec = m_pVideoDevice->getParameter(rfc6184::PROFILE_LEVEL_ID, sProfileLevelId);
  if (ec)
  {
    return false;
  }

  ec = m_pVideoDevice->getParameter(rfc6184::SPROP_PARAMETER_SETS, sSPropParameterSets);
  if (ec)
  {
    return false;
  }

  RtspUri rtspUri(sResource);
  boost::algorithm::replace_first(sSessionDescription, "<<NtpTime>>", ::toString(uiNtpTime));
  boost::algorithm::replace_first(sSessionDescription, "<<Ip>>", rtspUri.getServerIp());
  boost::algorithm::replace_first(sSessionDescription, "<<StreamName>>", m_sLiveStreamName);
  boost::algorithm::replace_first(sSessionDescription, "<<Software>>", getServerInfo());
  boost::algorithm::replace_first(sSessionDescription, "<<ProfileLevelId>>", sProfileLevelId);
  boost::algorithm::replace_first(sSessionDescription, "<<SPropParameterSets>>", sSPropParameterSets);

  sContentType = rfc4566::APPLICATION_SDP;
  return true;
}

std::vector<std::string> LiveRtspServer::getUnsupported(const RtspMessage& rtspRequest)
{
  // default method: just add all options as unsupported for now
  boost::optional<std::string> require = rtspRequest.getHeaderField(REQUIRE);
  if (require)
  {
    std::vector<std::string> vRequired = StringTokenizer::tokenize(*require, ",", true, true);
    return vRequired;
  }
  return std::vector<std::string>();
}

boost::optional<Transport> LiveRtspServer::selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports)
{
  // This server only supports unicast, RTP/AVP
  for (size_t i = 0; i < vTransports.size(); ++i)
  {
    rfc2326::Transport transport(vTransports[i]);
    if (transport.isValid())
    {
      if (((transport.getSpecifier() == "RTP/AVP") || (transport.getSpecifier() == "RTP/AVP/UDP")) &&
          (transport.isUnicast()) &&
          (!transport.isInterleaved())
          )
      {
        return boost::optional<Transport>(transport);
      }
    }
  }
  return boost::optional<Transport>();
}

boost::system::error_code LiveRtspServer::getServerTransportInfo(const RtspUri& rtspUri, const std::string& sSession, Transport& transport)
{
  DLOG(INFO) << "TODO: find/bind ports here";
  transport.setServerPortStart(getNextServerRtpPort());
  return boost::system::error_code();
}

uint16_t LiveRtspServer::getNextServerRtpPort() const
{
  uint16_t uiPort = m_uiNextServerTransportPort;
  m_uiNextServerTransportPort += 2;
  return uiPort;
}

boost::system::error_code LiveRtspServer::accessUnitCB(const std::vector<MediaSample>& mediaSamples)
{
  VLOG(10) << "Received " << mediaSamples.size() << " AUs";
  boost::mutex::scoped_lock l(m_sessionLock);
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    // Hard-code payload type as SDP is also hard-coded
    if (it->second->isPlaying())
      it->second->deliver(96, mediaSamples);
  }
  return boost::system::error_code();
}

ResponseCode LiveRtspServer::handleSetup(const RtspMessage& setup, std::string& sSession,
                                         Transport& transport, const std::vector<std::string>& vSupported)
{
//  if (m_mRtspSessions.empty())
//  {
//    VLOG(2) << "Starting video device.";
//    m_pVideoDevice->start();
//  }
  return RtspServer::handleSetup(setup, sSession, transport, vSupported);
}

ResponseCode LiveRtspServer::handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported)
{
  ResponseCode eCode = RtspServer::handleTeardown(teardown, vSupported);

//  if (m_mRtspSessions.empty())
//  {
//    VLOG(2) << "Stopping video device.";
//    m_pVideoDevice->stop();
//  }
  return eCode;
}

void LiveRtspServer::onShutdown()
{
  VLOG(5) << "Shutting down live stream";
  if (m_pVideoDevice->isRunning())
  {
    VLOG(5) << "Shutting down - Stopping video device.";
    m_pVideoDevice->stop();
  }
  else
  {
    VLOG(2) << "Shutting down - video device not running.";
  }
  // teardown all sessions
}

} // rfc2326
} // rtp_plus_plus
