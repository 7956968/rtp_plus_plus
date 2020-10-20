#pragma once
#include <map>
#include <set>
#include <vector>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <cpputil/Buffer.h>
#include <cpputil/GenericParameters.h>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc2326/IRtspAgent.h>
#include <rtp++/rfc2326/MessageBody.h>
#include <rtp++/rfc2326/RtspMessage.h>
#include <rtp++/rfc2326/RtspServerConnection.h>
#include <rtp++/rfc2326/ServerMediaSession.h>
#include <rtp++/rfc2326/SessionGenerator.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief RtspServer is the base class for RTSP servers.
 *
 * 
 */
class RtspServer : public IRtspAgent
{
  friend class RtspServerConnection;
  friend class RtspServerSession;
  friend class ServerMediaSession;

public:
  static const unsigned DEFAULT_SESSION_TIMEOUT;

  /**
   * @brief Constructor
   * @param[in] ioServer io service to be used for asynchronous operations.
   * @param[in] applicationParameters Parameters to configure the server.
   * @param[out] ec error code returns of the RTSP server was initialised correctly.
   * @param[in] uiPort Port to be used for RTSP.
   * @param[in] uiSessionTimeout Session timeout in seconds.
   */
  RtspServer(boost::asio::io_service& ioService, const GenericParameters& applicationParameters, 
    boost::system::error_code& ec, unsigned short uiPort = 554, 
    unsigned uiSessionTimeout = DEFAULT_SESSION_TIMEOUT);
  /**
   * @brief Destructor
   */
  virtual ~RtspServer();
  /**
   * @brief start() starts the RTSP server.
   */
  boost::system::error_code start();
  /**
   * @brief shutdown() stops the RTSP server.
   * This cancels all asynchronous operations started
   * by the RTSP server.
   */
  boost::system::error_code shutdown();
  /**
   * @brief Getter for RTSP port
   */
  unsigned short getPort() const { return m_uiPort; }
  /**
   * @brief Getter for RTSP server address
   */
  std::string getRtspServerAddress() const;
  /**
   * @brief returns if the RTSP server is shutting down
   */
  bool isShuttingDown() const { return m_bShuttingDown; }
  /**
   * @brief Getter for io service.
   */
  boost::asio::io_service& getIoService() { return m_ioService; }
  /**
   * @brief Getter for media session network manager
   */
  MediaSessionNetworkManager& getMediaSessionNetworkManager() { return m_mediaSessionNetworkManager; }

protected:

  /**
   * @brief doStart() can be overridden to change the start-up behaviour of the RTSP server
   * The default behaviour is to start accepting connections
   */
  virtual boost::system::error_code doStart();
  /**
   * @brief doShutdown() can be overridden to change the stopping behaviour of the RTSP server
   * The default behaviour is to close the acceptors and stop accepting connections
   */
  virtual boost::system::error_code doShutdown();
  /**
   * @brief returns a string containing the server info. This string will form part of RTSP responses
   * allowing the client to identify the type of RTSP server.
   */
  virtual std::string getServerInfo() const { return std::string("rtp++ RTSP Server ") + RTP_PLUS_PLUS_VERSION_STRING; }
  /**
   * @brief
   *
   */
  virtual ResponseCode handleDescribe(const RtspMessage& describe, std::string& sSessionDescription, 
    std::string& sContentType, const std::vector<std::string>& vSupported);
  /**
   * @brief
   *
   */
  virtual ResponseCode handleSetup(const RtspMessage& setup, std::string& sSession,
                                   Transport& transport, const std::vector<std::string>& vSupported);
  /**
   * @brief
   *
   */
  virtual ResponseCode handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported);
  /**
   * @brief returns if one of the requested content types is supported by this server
   *
   */
  virtual bool isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes) = 0;
  /**
   * @brief returns if the specified option is supported by the server
   * @param[in] sOptionOrFeature The option or feature that is being checked.
   * @return true of the feature is supported by the RTSP server, false otherwise.
   */
  virtual bool isOptionSupportedByServer(const std::string& sOptionOrFeature) const { return false; }
  /**
   * @brief Returns which options are unsupported by the RTSP server
   * @return A vector of unsupported options.
   *
   * The implementation should examine the "Require" header of rtspRequest to see what 
   * options are required, and then return a vector containing those options not supported
   * by the server.
   */
  virtual std::vector<std::string> getUnsupported(const RtspMessage& rtspRequest) = 0;
  /**
   * @brief retrieves a description of the requested resource, as well as the content type of this description.
   *
   * The description may vary based on the supported features of the client, or based on the selected transport.
   * E.g. If the client supports rtcp-mux and this is indicated either in the vSupported or transport, the server
   * can adjust the session description.
   * @param[in] sResource The name of the resource to be described. This will typically be the name of the media
   * file or the live stream.
   * @param[out] sSessionDescription The description of the requested resource
   * @param sContentType The content type of the resource description e.g. application/sdp.
   * @param vSupported Optionally the client can specify supported features allowing the server to customise 
   * the resource description
   * @param transport The transport is an optional parameter allowing the server to create the appropriate 
   * resource description. This is necessary so that the description returned by describe and setup match.
   */
  virtual bool getResourceDescription(const std::string& sResource, std::string& sSessionDescription, 
    std::string& sContentType, const std::vector<std::string>& vSupported, const Transport& transport = Transport()) = 0;
  /**
   * @brief Creates the actual ServerMediaSession object. 
   *
   * This method can be overridden to create specialised server media session objects.
   */
  virtual std::unique_ptr<ServerMediaSession> createServerMediaSession(const std::string& sSession, 
    const std::string& sSessionDescription, const std::string& sContentType);
  /**
   * @brief This method gets the server parameters of the transport.
   *
   * @param[in] rtspUri The RTSP URI that is being set up. The various media lines can be identified by
   * the last part of the URI.
   * @param[in] sSession The session of the RTSP session that is being set up.
   * @param[in,out] transport The RTSP transport provided by the client. The implementation must at the very
   * least set the server ports. 
   * At the very least it must set the server_port member of the Transport header using the setServerPortStart()
   * method.
   */
  virtual boost::system::error_code getServerTransportInfo(const RtspUri& rtspUri, const std::string& sSession, Transport& transport) = 0;
  /**
   * @brief This method will be called by the RtspServer base class on shutdown allowing subclasses to perform any cleanup.
   *
   */
  virtual void onShutdown() = 0;
  // TODO
//  virtual ServerMediaSubsession::ptr getMediaSubsession(const std::string& sUri) = 0;
//  virtual bool checkRange(const std::string& sRequestUri, const Range& range) const = 0;
//  virtual bool playServerMediaSession(RtspServerSession::ptr pRtspSession, const Range& range) = 0;

protected:
  /**
   * @brief Returns a list of options that are supported by the client, and the server.
   */
  std::vector<std::string> getSupported(const RtspMessage& rtspRequest);
  /**
   * @brief tears down all active media sessions
   */
  void teardownAllSessions();

  //////////////////////////////////////////////////////////////////////////
  /// SET_PARAMETER specific methods
//  virtual bool isParameterKnown(const std::string& sParameter, RtspServerSession::ptr pSession) const
//  {
//    return false;
//  }
//  virtual bool isParameterReadOnly(const std::string& sParameter, RtspServerSession::ptr pSession) const
//  {
//    return true;
//  }
//  virtual bool setParameter(const std::string& sParamName, const std::string& sParamValue, RtspServerSession::ptr pSession)
//  {
//    return false;
//  }

protected:

  // Timeout task
  void checkConnectionsTask(const boost::system::error_code& error);
  void checkRtspSessions();

  // I/O related
  void startAccept();
  void acceptHandler(const boost::system::error_code& error, RtspServerConnectionPtr pNewConnection);
  void readCompletionHandler( const boost::system::error_code& error, const std::string& sRtspMessage, RtspServerConnectionPtr pConnection );
  void writeCompletionHandler( const boost::system::error_code& error, uint32_t uiMessageId, const std::string& sRtspMessage, RtspServerConnectionPtr pConnection );
  void closeCompletionHandler( RtspServerConnectionPtr pConnection );
  virtual void handleRequest(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  virtual void handleResponse(const RtspMessage& rtspResponse, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);

  //////////////////////////////////////////////////////////////////////////
  /// SET_PARAMETER
  // void setParameters(const StringMap_t& mParams, StringMap_t& mNotUnderstoodParameters, StringMap_t& mReadOnlyParameters, RtspServerSession::ptr pSession);

//  RtspResponsePtr setParameters( RtspServerSession::ptr pSession, RtspServerConnectionPtr pConnection, RtspRequestPtr pRequest );
  bool sessionExists(const std::string& sSession) const;
  uint32_t activeSessionCount() const;

  std::map<std::string, std::string> parseSetParameterMessageBody(const std::string& sMessageBody);

protected:

  GenericParameters m_applicationParameters;
  unsigned short m_uiPort;
  unsigned m_uiTimerTimeout;
  unsigned m_uiSessionTimeout;

  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer;
  boost::asio::ip::tcp::acceptor m_acceptor;
  /// Media session network manager
  MediaSessionNetworkManager m_mediaSessionNetworkManager;
  mutable boost::mutex m_sessionLock;
  std::unordered_map<std::string, std::unique_ptr<ServerMediaSession> > m_mRtspSessions;
  // TODO: store in map with address as accessor so that we can remove them on close?
  std::vector<RtspServerConnectionPtr> m_vConnections;
  
  //! m_sessionGenerator Used for generation of RTSP session identifiers
  SessionGenerator m_sessionGenerator;

  uint32_t m_uiRequestNumber;
  //! Shutdown flag
  bool m_bShuttingDown;
};

} // rfc2326
} // rtp_plus_plus
