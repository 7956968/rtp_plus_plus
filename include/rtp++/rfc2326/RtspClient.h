#pragma once
#include <deque>
#include <map>
#include <boost/asio/io_service.hpp>
#include <rtp++/rfc2326/MessageBody.h>
#include <rtp++/rfc2326/RtspClientConnection.h>
#include <rtp++/rfc2326/RtspMessage.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/Transport.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/Version.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&, const std::vector<std::string>&)> OptionsHandler_t;
typedef boost::function<void(const boost::system::error_code&, const MessageBody&, const std::vector<std::string>&)> DescribeHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::string& sSession, const Transport&, const std::vector<std::string>&)> SetupHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&)> PlayHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&)> PauseHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&)> TeardownHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&)> GetParameterHandler_t;
typedef boost::function<void(const boost::system::error_code&, const std::vector<std::string>&)> SetParameterHandler_t;

/**
 * @brief The RtspClient class is intended to manage the RTSP session with one RTSP server.
 *
 * It uses ONE RTSP client connection object with which it manages the RTSP interaction.
 * It sends one request at a time
 */
class RtspClient
{
public:

  /**
   * @brief Constructor
   */
  RtspClient(boost::asio::io_service& ioService, const std::string& sRtspUri, const std::string& sIp, const uint16_t uiPort);

  // TEST TO SEE WHAT IS STORED: IP or host name?
  std::string getIpOrHost() const { return m_pConnection->getIpOrHost(); }
  std::string getServerIp() const { return m_pConnection->getServerIp(); }

  /**
   * @brief getRtspClientConnection Accessor to get handle to RtspClientConnection needed for interleaving RTP/RTCP over RTSP
   * @return RtspClientConnection if it has been initialised, otherwise null
   */
  RtspClientConnectionPtr getRtspClientConnection() const { return m_pConnection; }

  // handlers for asynchronous operation
  void setOptionsHandler(OptionsHandler_t handler) { m_optionsHandler = handler; }
  void setDescribeHandler(DescribeHandler_t handler) { m_describeHandler = handler; }
  void setSetupHandler(SetupHandler_t handler) { m_setupHandler = handler; }
  void setPlayHandler(PlayHandler_t handler) { m_playHandler = handler; }
  void setPauseHandler(PauseHandler_t handler) { m_pauseHandler = handler; }
  void setTeardownHandler(TeardownHandler_t handler) { m_teardownHandler = handler; }
  void setGetParameterHandler_t(GetParameterHandler_t handler) { m_getParameterHandler = handler; }
  void setSetParameterHandler_t(SetParameterHandler_t handler) { m_setParameterHandler = handler; }

  boost::system::error_code options(const std::vector<std::string>& vSupported = std::vector<std::string>());
  boost::system::error_code describe(const std::vector<std::string>& vSupported = std::vector<std::string>());
  boost::system::error_code setup(const rfc4566::SessionDescription& sessionDescription, const rfc4566::MediaDescription& media, const Transport& transport, const std::vector<std::string>& vSupported = std::vector<std::string>());
  boost::system::error_code play(double dStart = 0.0, double dEnd = -1.0, const std::vector<std::string>& vSupported = std::vector<std::string>());
//  boost::system::error_code  pause();
  boost::system::error_code teardown(const std::vector<std::string>& vSupported = std::vector<std::string>());
//  boost::system::error_code getParameter( const std::string& sUri, const std::string& sParameterName );
//  boost::system::error_code getParameter( const std::string& sUri, StringList_t& parameterMap );
//  boost::system::error_code setParameter( const std::string& sUri, const std::string& sParameterName, const std::string& sParameterValue );

  void shutdown();

protected:
  void readCompletionHandler( const boost::system::error_code& error, const std::string& sRtspMessage, RtspClientConnectionPtr pConnection );
  void writeCompletionHandler( const boost::system::error_code& error, uint32_t uiMessageId, const std::string& sRtspMessage, RtspClientConnectionPtr pConnection );
  void closeCompletionHandler( RtspClientConnectionPtr pConnection );
  void connectErrorHandler(const boost::system::error_code& error, RtspClientConnectionPtr pConnection );
  virtual void handleRequest(const RtspMessage& rtspRequest, RtspClientConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  virtual void handleResponse(const RtspMessage& rtspResponse, RtspClientConnectionPtr pConnection, const std::vector<std::string>& vSupported);

private:
  std::string getClientInfo() const { return std::string("rtp++ RTSP Client ") + RTP_PLUS_PLUS_VERSION_STRING; }
  void initialiseConnection();
  void notifyCallerOfError(const boost::system::error_code& error);
  boost::system::error_code sendRequest(RtspMethod eMethod, const std::string& sRtspRequest);

private:
  //! asio io service
  boost::asio::io_service& m_ioService;
  // shutdown flag set when shutdown is called
  bool m_bShuttingDown;
  std::string m_sIp;
  uint16_t m_uiPort;
  RtspClientConnectionPtr m_pConnection;
  RtspUri m_rtspUri;
  /// Session gets set on successful SETUP
  std::string m_sSession;
  /// session timeout in seconds
  uint32_t m_uiSessionTimeout;
  // SN state
  uint32_t m_uiSN;
  std::map<uint32_t, std::pair<RtspMethod, std::string> > m_mRequestMap;
  std::deque<RtspMethod> m_qOutstandingRequests;

  OptionsHandler_t m_optionsHandler;
  DescribeHandler_t m_describeHandler;
  SetupHandler_t m_setupHandler;
  PlayHandler_t m_playHandler;
  PauseHandler_t m_pauseHandler;
  TeardownHandler_t m_teardownHandler;
  GetParameterHandler_t m_getParameterHandler;
  SetParameterHandler_t m_setParameterHandler;
};

} // rfc2326
} // rtp_plus_plus
