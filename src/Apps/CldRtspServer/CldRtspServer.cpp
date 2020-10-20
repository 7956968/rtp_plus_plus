#include "CorePch.h"
#include "CldRtspServer.h"
#include <functional>
#include <boost/exception/all.hpp>
#include <cpputil/Conversion.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/LiveServerMediaSession.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

using namespace rfc2326;
using media::MediaSample;

const uint16_t CldRtspServer::DEFAULT_SERVER_MEDIA_PORT = 49170;
uint32_t CldRtspServer::SourceId = 0;

CldRtspServer::CldRtspServer(boost::asio::io_service& ioService,
                               const GenericParameters& applicationParameters,
                               boost::system::error_code& ec,
                               unsigned short uiPort, unsigned uiSessionTimeout)
  :RtspServer(ioService, applicationParameters, ec, uiPort, uiSessionTimeout),
    m_uiNextServerTransportPort(DEFAULT_SERVER_MEDIA_PORT)
{

}

std::unique_ptr<ServerMediaSession> CldRtspServer::createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType)
{
  return std::unique_ptr<ServerMediaSession>(new LiveServerMediaSession(*this, sSession, sSessionDescription, sContentType, m_uiSessionTimeout));
}

ResponseCode CldRtspServer::handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const
{
  methods.insert(OPTIONS);
  methods.insert(DESCRIBE);
  methods.insert(SETUP);
  methods.insert(PLAY);
  methods.insert(TEARDOWN);
  return OK;
}

boost::system::error_code CldRtspServer::addLiveSource(const rfc4566::SessionDescription& sdp, const InterfaceDescriptions_t& networkConfig)
{
  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), sdp, networkConfig, false);
  SimpleMediaSessionFactory factory;
  // TODO: need to return error when ports could not be bound
  boost::shared_ptr<SimpleMediaSessionV2> pMediaSession = factory.create(getIoService(), mediaSessionDescription, m_applicationParameters);
  // in this case we know there is only one video session for now
  pMediaSession->getVideoSessionManager()->registerMediaCallback(boost::bind(&CldRtspServer::accessUnitCB, this, _1, SourceId));
  m_mSources.insert(std::make_pair(SourceId, std::make_tuple(sdp, networkConfig, pMediaSession)));

  // TODO: double check that session name is unique!!!
  m_mSourceIdSessionNameMap.insert(bm_type::value_type(SourceId, sdp.getSessionName()));

  std::ostringstream ostr;
  ostr << getRtspServerAddress() << "/" << sdp.getSessionName();
  LOG(INFO) << "Stream can be accessed at " << ostr.str();

  SourceId++;
  pMediaSession->start();
  return boost::system::error_code();
}

bool CldRtspServer::checkResource(const std::string& sResource)
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

//  if ((rtspUri.getName() == m_sLiveStreamName) ||
//      (rtspUri.getRelativePath() == m_sLiveStreamName && rtspUri.getName() == "track1")
//     )
//  {
//    VLOG(10) << "Resource " << sResource << " ok";
//    return true;
//  }

  for (auto it : m_mSources)
  {
    auto& tup = it.second;
    std::string sSessionName = std::get<0>(tup).getSessionName();
    if ((rtspUri.getName() == sSessionName) ||
        (rtspUri.getRelativePath() == sSessionName && rtspUri.getName() == "track1")
        )
    {
      VLOG(10) << "Resource " << sResource << " ok";
      return true;
    }
  }
  VLOG(10) << "Resource " << sResource << " NOT ok";
  return false;
}

std::vector<std::string> CldRtspServer::getUnsupported(const RtspMessage& rtspRequest)
{
  // default method: just add all options as unsupported for now
  boost::optional<std::string> require = rtspRequest.getHeaderField("Require");
  if (require)
  {
    std::vector<std::string> vRequired = StringTokenizer::tokenize(*require, ",", true, true);
    return vRequired;
  }
  return std::vector<std::string>();
}

bool CldRtspServer::isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes)
{
  auto it = std::find_if(vAcceptedTypes.begin(), vAcceptedTypes.end(), [](const std::string& sAccept)
  {
    return sAccept == rfc4566::APPLICATION_SDP;
  });
  return (it != vAcceptedTypes.end());
}

bool CldRtspServer::getResourceDescription(const std::string& sResource, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported, const Transport& transport)
{
  // TODO: create SDP for live media session with video and audio
  VLOG(5) << "Getting resource description for " << sResource;

#if 0
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
#else
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
      "a=fmtp:96 packetization-mode=1\r\n"\
      "a=control:track1\r\n";
#endif

  RtspUri rtspUri(sResource);
  auto it = std::find_if(m_mSources.begin(), m_mSources.end(), [sResource, rtspUri](const std::pair<uint32_t,  SourceInfo_t>& item)
  {
    std::string sSessionName = std::get<0>(item.second).getSessionName();
    return ((rtspUri.getName() == sSessionName) ||
        (rtspUri.getRelativePath() == sSessionName && rtspUri.getName() == "track1"));
  });

  std::string sStreamName = std::get<0>(it->second).getSessionName();
  // TODO: extract parameter sets from item?

//  std::string sProfileLevelId, sSPropParameterSets;
//  boost::system::error_code ec = m_pVideoDevice->getParameter(rfc6184::PROFILE_LEVEL_ID, sProfileLevelId);
//  if (ec)
//  {
//    return false;
//  }

//  ec = m_pVideoDevice->getParameter(rfc6184::SPROP_PARAMETER_SETS, sSPropParameterSets);
//  if (ec)
//  {
//    return false;
//  }

  uint64_t uiNtpTime = RtpTime::getNTPTimeStamp();
  boost::algorithm::replace_first(sSessionDescription, "<<NtpTime>>", ::toString(uiNtpTime));
  boost::algorithm::replace_first(sSessionDescription, "<<Ip>>", rtspUri.getServerIp());
  boost::algorithm::replace_first(sSessionDescription, "<<StreamName>>", sStreamName);
  boost::algorithm::replace_first(sSessionDescription, "<<Software>>", getServerInfo());
//  boost::algorithm::replace_first(sSessionDescription, "<<ProfileLevelId>>", sProfileLevelId);
//  boost::algorithm::replace_first(sSessionDescription, "<<SPropParameterSets>>", sSPropParameterSets);

  sContentType = "application/sdp";
  return true;
}

boost::optional<Transport> CldRtspServer::selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports)
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

boost::system::error_code CldRtspServer::getServerTransportInfo(const RtspUri& rtspUri, const std::string& sSession, Transport& transport)
{
  DLOG(INFO) << "TODO: find/bind ports here";
  transport.setServerPortStart(m_uiNextServerTransportPort);
  m_uiNextServerTransportPort += 2;
  return boost::system::error_code();
}

void CldRtspServer::accessUnitCB(const std::vector<MediaSample>& mediaSamples, uint32_t uiSourceId)
{
//  VLOG(10) << "Received " << mediaSamples.size() << " media samples in AU";
  // TODO: based onsource id: only deliver media to RTSP sessions for that specific resource
 #if 1
  auto iterPair = m_mSourceRtspSessionMap.equal_range(uiSourceId);
  for (auto it = iterPair.first; it != iterPair.second; ++it)
  {
//    VLOG(10) << "Checking session " << it->first << " Session: " << it->second;
    if (m_mRtspSessions[it->second]->isPlaying())
    {
      m_mRtspSessions[it->second]->deliver(96, mediaSamples);
    }
    else
    {
      VLOG(10) << "Session " << it->first << " not playing";
    }
  }

#else
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    // session will only be removed once expired:
    // only deliver while session is playing
    if (it->second->isPlaying())
    {
      VLOG(10) << "Sent media to session " << it->first;
      it->second->deliver(mediaSamples);
    }
    else
    {
      VLOG(10) << "Session " << it->first << " not playing";
    }
  }
#endif
}

ResponseCode CldRtspServer::handleSetup(const RtspMessage& setup, std::string& sSession,
                                         Transport& transport, const std::vector<std::string>& vSupported)
{
  VLOG(2) << "Inserting " << sSession << " into multimap";
  RtspUri rtspUri = setup.getRtspUri();

  if (rtspUri.getName() == "track1")
  {
    std::string sSessionName  = rtspUri.getRelativePath();
    VLOG(2) << "Searching for source " << sSessionName;

    auto it = m_mSourceIdSessionNameMap.right.find(sSessionName);
    if ( it != m_mSourceIdSessionNameMap.right.end())
    {      
      // check if this session exists?
      ResponseCode eCode = RtspServer::handleSetup(setup, sSession, transport, vSupported);
      if (eCode == OK)
      {
        // insert mapping from RTSP session to live source session
        uint32_t uiSourceId = it->second;
        m_mSourceRtspSessionMap.insert(std::make_pair(uiSourceId, sSession));
        return eCode;
      }
      else
      {
        // TODO: undo map insertions
        LOG(WARNING) << "Failed to setup session";
      }
    }
    else
    {
      LOG(WARNING) << "Couldn't find live source session";
      return NOT_FOUND;
    }
  }
  else
  {
    LOG(ERROR) << "TODO: Invalid subsession?!?" << rtspUri.getName();
  }

  return INTERNAL_SERVER_ERROR;
}

ResponseCode CldRtspServer::handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported)
{
  ResponseCode eCode = RtspServer::handleTeardown(teardown, vSupported);

  RtspUri rtspUri = teardown.getRtspUri();
  std::string sSessionName  = rtspUri.getName();

  auto it = m_mSourceIdSessionNameMap.right.find(sSessionName);
  if ( it != m_mSourceIdSessionNameMap.right.end())
  {
    uint32_t uiSourceId = it->second;
    // remove specific multimap value
    auto iterPair = m_mSourceRtspSessionMap.equal_range(uiSourceId);
    for (auto it = iterPair.first; it != iterPair.second; ++it)
    {
      if (it->second == teardown.getSession())
      {
        VLOG(2) << "Removing " << teardown.getSession() << " from multimap";
        m_mSourceRtspSessionMap.erase(it);
        break;
      }
    }
  }

  return eCode;
}

void CldRtspServer::onShutdown()
{
  VLOG(5) << "Shutting down live stream";

  for (auto it : m_mSources)
  {
    auto& tup = it.second;
    std::string sSessionName = std::get<0>(tup).getSessionName();
    VLOG(2) << "Stopping " << sSessionName;
    std::get<2>(tup)->stop();
  }

}

}
