#include "CorePch.h"
#include <rtp++/rfc2326/ServerMediaSession.h>
#include <rtp++/rfc2326/RtspMethod.h>
#include <rtp++/rfc2326/RtspUtil.h>
#include <rtp++/rfc2326/RtspServer.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

ServerMediaSession::ServerMediaSession(RtspServer& rtspServer, const std::string& sSession,
                                       const std::string& sSessionDescription, const std::string& sContentType)
  :m_rtspServer(rtspServer),
    m_sSession(sSession),
    m_sSessionDescription(sSessionDescription),
    m_sContentType(sContentType),
    //m_transport(transport),
    m_eState(ST_SETUP)
{
  VLOG(5) << "[" << this << "] Session created: " << m_sSession;
}

ServerMediaSession::~ServerMediaSession()
{
  VLOG(5) << "[" << this << "] Session destroyed: " << m_sSession;
}

bool ServerMediaSession::isExpired() const
{
  if (m_tTorndown.is_not_a_date_time())
  {
    // session has not been torn down
    return false;
  }
  else
  {
    // we say that a session is expired one second after it is torn down
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    return ((tNow - m_tTorndown).total_milliseconds() > 1000);      
  }
}

bool ServerMediaSession::livenessCheckFailed() const
{
  // check the session liveness
  return !isSessionLive();
}

void ServerMediaSession::deliver(const uint8_t uiPayloadType, const media::MediaSample& mediaSample)
{
  if (isPlaying())
  {
    doDeliver(uiPayloadType, mediaSample);
  }
  else
  {
    LOG(WARNING) << "Invalid state for deliver";
  }
}

void ServerMediaSession::deliver(const uint8_t uiPayloadType, const std::vector<media::MediaSample>& mediaSamples)
{
  if (isPlaying())
  {
    doDeliver(uiPayloadType, mediaSamples);
  }
  else
  {
    LOG(WARNING) << "Invalid state for deliver";
  }
}

void ServerMediaSession::shutdown()
{
  if (isPlaying())
  {
    doShutdown();
  }
  else
  {
    // This might occur if the session has already been shutdown
    VLOG(5) << "Invalid state for shutdown";
  }
  m_eState = ST_TORNDOWN;
  m_tTorndown = boost::posix_time::microsec_clock::universal_time();
}

ResponseCode ServerMediaSession::handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const
{
  // generic method - let server handle it
  return m_rtspServer.handleOptions(methods, vSupported);
}

ResponseCode ServerMediaSession::handleDescribe(const RtspMessage& describe, std::string& sSessionDescription,
                                                std::string& sContentType, const std::vector<std::string>& vSupported)
{
  // generic method - let server handle it
  return m_rtspServer.handleDescribe(describe, sSessionDescription, sContentType, vSupported);
}

ResponseCode ServerMediaSession::handleSetup(const RtspMessage& setup, std::string& sSession,
  Transport& transport, const std::vector<std::string>& vSupported)
{
  VLOG(2) << "Setup for session: " << sSession;

  // get server transport info
  RtspUri rtspUri(setup.getRtspUri());
  boost::system::error_code ec = m_rtspServer.getServerTransportInfo(rtspUri, sSession, transport);
  if (ec)
  {
    LOG(WARNING) << "Error getting server transport info: " << ec.message();
    return SERVICE_UNAVAILABLE;
  }

  return OK;
}

ResponseCode ServerMediaSession::handlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported)
{
  ResponseCode eCode = doHandlePlay(play, rtpInfo, vSupported);
  if (eCode == OK)
  {
    m_eState = ST_PLAYING;
  }
  else
  {
    m_eState = ST_ERROR;
  }
  return eCode;
}

bool ServerMediaSession::checkResource(const std::string& sResource)
{
  // generic method - let server handle it
  return m_rtspServer.checkResource(sResource);
}

boost::optional<Transport> ServerMediaSession::selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports)
{
  // generic method - let server handle it
  return m_rtspServer.selectTransport(rtspUri, vTransports);
}

} // rfc2326
} // rtp_plus_plus
