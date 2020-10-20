#include "CorePch.h"
#include <rtp++/rfc2326/RtspServer.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/RtspUtil.h>

using boost::asio::ip::tcp;

namespace rtp_plus_plus
{
namespace rfc2326
{

const unsigned RtspServer::DEFAULT_SESSION_TIMEOUT = 60;

RtspServer::RtspServer( boost::asio::io_service& ioService, const GenericParameters& applicationParameters, 
  boost::system::error_code& ec, unsigned short uiPort, unsigned uiSessionTimeout)
  :m_applicationParameters(applicationParameters),
    m_uiPort(uiPort),
    m_uiTimerTimeout(2),
    m_uiSessionTimeout(uiSessionTimeout),
    m_ioService(ioService),
    m_timer(ioService, boost::posix_time::seconds(m_uiTimerTimeout)),
    m_acceptor(ioService),
    //m_portManager(m_ioService),
    m_mediaSessionNetworkManager(ioService),
    m_uiRequestNumber(0),
    m_bShuttingDown(false)
{
  // bind to port
  boost::asio::ip::tcp::endpoint endpoint = tcp::endpoint(tcp::v4(), m_uiPort);
  m_acceptor.open(endpoint.protocol(), ec);
  if (ec) return;
  m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec) return;
  m_acceptor.bind(endpoint, ec);
  if (ec) return;
  m_acceptor.listen();
}

RtspServer::~RtspServer()
{

}

boost::system::error_code RtspServer::doStart()
{
  m_timer.async_wait(boost::bind(&RtspServer::checkConnectionsTask, this, boost::asio::placeholders::error));
  startAccept();
  return boost::system::error_code();
}

boost::system::error_code RtspServer::start()
{
  VLOG(5) << "Starting RTSP server: " << getRtspServerAddress();
  return doStart();
}

boost::system::error_code RtspServer::doShutdown()
{
  m_bShuttingDown = true;
  boost::system::error_code ec;
  VLOG(5) << "Shutting down RTSP server. Closing acceptor";
  m_acceptor.close(ec);
  VLOG(5) << "Shutting down RTSP sessions";
  // teardown RTSP sessions
  teardownAllSessions();
  VLOG(5) << "Calling on shutdown";
  // give subclass opportunity for cleanup
  onShutdown();
  VLOG(5) << "Shutdown complete";
  return ec;
}

boost::system::error_code RtspServer::shutdown()
{
  // close all connections
  for (size_t i = 0; i < m_vConnections.size(); ++i)
  {
    if (!m_vConnections[i]->isClosed())
    {
      VLOG(2) << "Closing connection: " << m_vConnections[i]->getId();
      m_vConnections[i]->close();
    }
    else
    {
      VLOG(2) << "Connection " << m_vConnections[i]->getId() << " closed already";
    }
  }
  return doShutdown();
}

std::string RtspServer::getRtspServerAddress() const
{
  std::ostringstream ostr;
  ostr << "rtsp://";
  std::string sHostName = boost::asio::ip::host_name();
  boost::asio::ip::tcp::resolver resolver(m_ioService);
  boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
  boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
  boost::asio::ip::tcp::resolver::iterator end;;
  while(iter != end)
  {
    if (iter->endpoint().protocol() == tcp::v4())
    {
      // HACK: temporary fix to get IP: will NOT work if there are multiple IP addresses/NICs
      sHostName = iter->endpoint().address().to_string();
      break;
    }
    ++iter;
  }

  ostr << sHostName << ":" << m_uiPort;
  return ostr.str();
}

void RtspServer::startAccept()
{
  VLOG(10) << "RTSP server listening for connections on port " << m_uiPort;

  RtspServerConnectionPtr pNewConnection = RtspServerConnection::create(m_acceptor.get_io_service());
  pNewConnection->setReadCompletionHandler(boost::bind(&RtspServer::readCompletionHandler, this, boost::asio::placeholders::error, _2, _3) );
  pNewConnection->setWriteCompletionHandler(boost::bind(&RtspServer::writeCompletionHandler, this, boost::asio::placeholders::error, _2, _3, _4) );
  pNewConnection->setCloseHandler(boost::bind(&RtspServer::closeCompletionHandler, this, _1) );

  m_acceptor.async_accept(pNewConnection->socket(),
                          boost::bind(&RtspServer::acceptHandler, this,
                                      boost::asio::placeholders::error, pNewConnection));
}

void RtspServer::acceptHandler( const boost::system::error_code& error, RtspServerConnectionPtr pNewConnection )
{
  if (!error)
  {
    VLOG(2) << "Starting new connection: " << pNewConnection->getId();
    // handle new connection
    pNewConnection->start();
    // store pointer to connection so that we can close them all on shutdown
    m_vConnections.push_back(pNewConnection);

    // listen for new connections
    startAccept();
  }
  else
  {
    // The operation will be aborted on close.
    if (error!= boost::asio::error::operation_aborted)
      LOG(WARNING) << "Error in accept: " << error.message();
  }
}

void RtspServer::closeCompletionHandler( RtspServerConnectionPtr pConnection )
{
  VLOG(10) << "RtspServer::closeCompletionHandler: " << pConnection->getId();
  // TODO: remove all requests related to connection
  //  m_connectionRequestDatabase.removeEntries( pConnection->getId() );

  //  m_connectionManager.removeConnection( pConnection );
}

void RtspServer::readCompletionHandler( const boost::system::error_code& error, const std::string& sRtspMessage, RtspServerConnectionPtr pConnection )
{
  if (!error)
  {
    VLOG(2) << "Read RTSP message (" << sRtspMessage.length() << "):\r\n" << sRtspMessage;
    boost::optional<RtspMessage> pRtspMessage = RtspMessage::parse(sRtspMessage);
    if (!pRtspMessage)
    {
      // Invalid RTSP message received
      // 400 BAD REQUEST
      VLOG(5) << "Bad RTSP request";
      pConnection->write(m_uiRequestNumber++, createErrorResponse(BAD_REQUEST, 0));
      return;
    }
    else
    {
      std::vector<std::string> vSupported = getSupported(*pRtspMessage);
      if (pRtspMessage->isRequest())
      {
        handleRequest(*pRtspMessage, pConnection, vSupported);
      }
      else if (pRtspMessage->isResponse())
      {
        handleResponse(*pRtspMessage, pConnection, vSupported);
      }
      else
      {
        LOG(WARNING) << "Not request or response!!";
        assert(false);
      }
    }
  }
  else
  {
    if ((error != boost::asio::error::eof) &&
      (error != boost::asio::error::connection_aborted)) // is called on windows when closing the socket
    {
      LOG(WARNING) << "Error in read: " << error.message();
    }
    else
    {
      VLOG(5) << "Error in read: " << error.message();
    }
  }
}

void RtspServer::writeCompletionHandler( const boost::system::error_code& error, uint32_t uiMessageId, const std::string& sRtspMessage, RtspServerConnectionPtr pConnection )
{
  if (!error)
  {
    VLOG(2) << "[S->C] Wrote RTSP message (" << sRtspMessage.length() << "):\r\n" << sRtspMessage;
  }
  else
  {
    LOG(WARNING) << "Error in write: " << error.message();
  }
}

bool RtspServer::sessionExists(const std::string& sSession) const
{
  boost::mutex::scoped_lock l(m_sessionLock);
  auto it = m_mRtspSessions.find(sSession);
  return (it != m_mRtspSessions.end());
}

uint32_t RtspServer::activeSessionCount() const
{
  boost::mutex::scoped_lock l(m_sessionLock);
  return m_mRtspSessions.size();
}

std::vector<std::string> RtspServer::getSupported(const RtspMessage& rtspRequest)
{
  std::vector<std::string> vSupportedByServer;
  // default method: nothing is supported
  boost::optional<std::string> supportedByClient = rtspRequest.getHeaderField(SUPPORTED);
  if (supportedByClient)
  {
    std::vector<std::string> vSupportedByClient = StringTokenizer::tokenize(*supportedByClient, ",", true, true);
    for (size_t i = 0; i < vSupportedByClient.size(); ++i)
    {
      if (isOptionSupportedByServer(vSupportedByClient[i]))
        vSupportedByServer.push_back(vSupportedByClient[i]);
    }
  }
  return vSupportedByServer;
}

void RtspServer::handleRequest(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  assert(rtspRequest.isRequest());

  // handle unsupported options here by returning the appropriate error code on Require
  std::vector<std::string> vUnsupported = getUnsupported(rtspRequest);
  if (!vUnsupported.empty())
  {
    pConnection->write(m_uiRequestNumber++, createErrorResponse(OPTION_NOT_SUPPORTED, rtspRequest.getCSeq(), vSupported, vUnsupported));
    return;
  }

  std::string sSession = rtspRequest.getSession();

  if (sSession != "")
  {
    // handle request in session if possible
    if (!sessionExists(sSession))
    {
      // NOT FOUND
      pConnection->write(m_uiRequestNumber++, createErrorResponse(SESSION_NOT_FOUND, rtspRequest.getCSeq(), vSupported, vUnsupported));
      return;
    }
    else
    {
      switch(rtspRequest.getMethod())
      {
        // we want to handle teardowns at the server level
        case TEARDOWN:
        {
          IRtspAgent::handleRequest(rtspRequest, pConnection, vSupported);
          break;
        }
        // other requests with sessions are handled at the session level
        default:
        {
          boost::mutex::scoped_lock l(m_sessionLock);
          m_mRtspSessions[sSession]->handleRequest(rtspRequest, pConnection, vSupported);
        }
      }
    }
  }
  else
  {
    IRtspAgent::handleRequest(rtspRequest, pConnection, vSupported);
  }
}

void RtspServer::handleResponse(const RtspMessage& rtspResponse, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // TODO: we are not currently sending requests and therefore do not handle responses
  LOG(WARNING) << "TODO: we are not currently sending requests and therefore do not handle responses";
}

void RtspServer::checkConnectionsTask( const boost::system::error_code& ec)
{
  checkRtspSessions();

  // Check connections every 5 seconds
  if (!m_bShuttingDown)
  {
    m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(m_uiTimerTimeout));
    m_timer.async_wait(boost::bind(&RtspServer::checkConnectionsTask, this, boost::asio::placeholders::error ));
  }
  else
  {
    boost::mutex::scoped_lock l(m_sessionLock);
    // if we're shutting down we have to wait until all sessions have been removed
    if (!m_mRtspSessions.empty())
    {
      VLOG(2) << "Waiting for " << m_mRtspSessions.size() << " RTSP sessions to be removed";
      m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(m_uiTimerTimeout));
      m_timer.async_wait(boost::bind(&RtspServer::checkConnectionsTask, this, boost::asio::placeholders::error ));
    }
    else
    {
      VLOG(5) << "All RTSP sessions removed";
    }
  }
}

void RtspServer::teardownAllSessions()
{
  boost::mutex::scoped_lock l(m_sessionLock);
  VLOG(10) << "teardownAllSessions";
  // Check session to see if they have timed out
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); ++it)
  {
    it->second->shutdown();
  }
}

void RtspServer::checkRtspSessions()
{
  boost::mutex::scoped_lock l(m_sessionLock);
  // Check session to see if they have timed out
  for (auto it = m_mRtspSessions.begin(); it != m_mRtspSessions.end(); )
  {
    //it->second->updateState();

    if (it->second->isExpired())
    {
      VLOG(2) << "Session " << it->second->getSession() << " expired. Removing.";
      it = m_mRtspSessions.erase(it);
    }
    else
    {
#if 1
      // check session liveness
      if (it->second->livenessCheckFailed())
      {
        VLOG(2) << "Liveness check failed. Shutting down session";
        it->second->shutdown();
      }
#endif
      ++it;
    }
  }
}

std::map<std::string, std::string> RtspServer::parseSetParameterMessageBody( const std::string& sMessageBody )
{
  std::map<std::string, std::string> mParams;
  char szBuffer[256];
  std::istringstream istr(sMessageBody);
  while (!istr.eof())
  {
    istr.getline(szBuffer, 256);
    std::string sLine(szBuffer);
    size_t pos = sLine.find(":");
    if (pos == std::string::npos)
    {
      // Invalid param
      continue;
    }
    std::string sName = sLine.substr(0, pos);
    std::string sValue = sLine.substr(pos + 1);
    boost::trim(sValue);
    mParams[sName] = sValue;
  }
  return mParams;
}

ResponseCode RtspServer::handleDescribe(const RtspMessage& describe, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported)
{
  boost::optional<std::string> accept = describe.getHeaderField(ACCEPT);
  if (accept)
  {
    // check if SDP is listed
    std::vector<std::string> vAccepted = StringTokenizer::tokenize(*accept, ",", true, true);
    if (!isContentTypeAccepted(vAccepted))
    {
      // we can't satisfy this request
      return NOT_ACCEPTABLE;
    }
  }

  if (getResourceDescription(describe.getRtspUri(), sSessionDescription, sContentType, vSupported))
  {
    return OK;
  }
  else
  {
    return INTERNAL_SERVER_ERROR;
  }
}

ResponseCode RtspServer::handleSetup(const RtspMessage& setup, std::string& sSession,
                                     Transport& transport, const std::vector<std::string>& vSupported)
{
  VLOG(2) << "Request for " << setup.getRtspUri();
  // if the SETUP is handled in the server, we need to create the session
  // Otherwise the request would have been handled in the session
  sSession = m_sessionGenerator.generateSession();

  boost::mutex::scoped_lock l(m_sessionLock);
  assert(m_mRtspSessions.find(sSession) == m_mRtspSessions.end());

  std::string sSessionDescription;
  std::string sContentType;
  if (getResourceDescription(setup.getRtspUri(), sSessionDescription, sContentType, vSupported, transport))
  {
    // create new session
    m_mRtspSessions[sSession] = createServerMediaSession(sSession, sSessionDescription, sContentType);
    // handle setup of first media line
    return m_mRtspSessions[sSession]->handleSetup(setup, sSession, transport, vSupported);
  }
  else
  {
    return INTERNAL_SERVER_ERROR;
  }
}

ResponseCode RtspServer::handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported)
{
  boost::mutex::scoped_lock l(m_sessionLock);
  auto it = m_mRtspSessions.find(teardown.getSession());
  if (it != m_mRtspSessions.end())
  {
    m_mRtspSessions[teardown.getSession()]->shutdown();
    return OK;
  }
  else
  {
    return NOT_FOUND;
  }
}

std::unique_ptr<ServerMediaSession> RtspServer::createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType)
{
  return std::unique_ptr<ServerMediaSession>(new ServerMediaSession(*this, sSession, sSessionDescription, sContentType));
}

} // rfc2326
} // rtp_plus_plus
