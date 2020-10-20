#pragma once
#include <set>
#include <rtp++/Version.h>
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtpInfo.h>
#include <rtp++/rfc2326/RtspMessage.h>
#include <rtp++/rfc2326/RtspServerConnection.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/Transport.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief The IRtspAgent class is the base class for the RtspClient and RtspServer classes, 
 * as well as ServerMediaSession classes.
 *
 * The virtual methods must be overridden by the subclasses to implement the RTSP functionality.
 */
class IRtspAgent
{
public:
  /**
   * @brief Constructor
   */
  IRtspAgent();
  /**
   * @brief Destructor
   */
  virtual ~IRtspAgent();
  /**
   * @brief handles the incoming request.
   * @param rtspRequest The RTSP request to be handled.
   * @param pConnection The RtspServerConnection the request was received on.
   * @param vSupported The Supported header in the request.
   *
   * The implementation should write the RTSP response on the passed in RTSP connection.
   * The base class implementation simply calls the appropriate method on the object i.e. 
   * options(), setup(), play(), etc. based on the RTSP request type.
   */
  virtual void handleRequest(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported = std::vector<std::string>());
  /**
   * @brief handles the incoming response.
   * @param rtspRequest The RTSP response to be handled.
   * @param pConnection The RtspServerConnection the request was received on.
   * @param vSupported The Supported header in the request.
   *
   * The base class implementation is a NOOP.
   */
  virtual void handleResponse(const RtspMessage& rtspResponse, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);

protected:
  /**
   * @brief RTSP options implementation. Subclasses can override by the behaviour by implementing handleOptions.
   */
  void options(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP describe implementation. Subclasses can override by the behaviour by implementing handleDescribe.
   */
  void describe(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP setup implementation. Subclasses can override by the behaviour by implementing handleSetup.
   */
  void setup(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP play implementation. Subclasses can override by the behaviour by implementing handlePlay.
   */
  void play(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP pause implementation. Subclasses can override by the behaviour by implementing handlePause.
   */
  void pause(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP teardown implementation. Subclasses can override by the behaviour by implementing handleTeardown.
   */
  void teardown(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP getParameter implementation. Subclasses can override by the behaviour by implementing handleGetParameter.
   */
  void getParameter(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief RTSP setParameter implementation. Subclasses can override by the behaviour by implementing handleSetParameter.
   */
  void setParameter(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported);
  /**
   * @brief This method implements the experimental REGISTER request from live555.com. 
   * Subclasses can override by the behaviour by implementing handleRegister.
   * @TODO This is not implemented yet. Place holder code.
   */
  void registerRtspServer(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection);

protected:
  /**
   * @brief Subclasses can implement this to control OPTION request behaviour.
   * @param[out] methods A set that should be populated with the RTSP methods that this RTSP agent understands.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const;
  /**
   * @brief Subclasses can implement this to control DESCRIBE request behaviour.
   * @param[in] describe The RTSP DESCRIBE message.
   * @param[out] sSessionDescription A description of the media session. This is usually "application/sdp".
   * @param[out] sContentType The content type of the session description.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handleDescribe(const RtspMessage& describe, std::string& sSessionDescription,
                                      std::string& sContentType, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control SETUP request behaviour.
   * @param[in] setup The RTSP SETUP message.
   * @param[in, out] sSession The RTSP session. The session should be generated when the initial RTSP SETUP
   * is received. Subsequent requests should store the previously generated value.
   * @param[out] transport. The transport selected by the server. This behaviour can be overridden by implementing selectTransport().
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   *
   * The setup method selects a transport from the options proposed by the client.
   */
  virtual ResponseCode handleSetup(const RtspMessage& setup, std::string& sSession,
                                   Transport& transport, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control PLAY request behaviour.
   * @param[in] setup The RTSP PLAY message.
   * @param[out] rtpInfo The RTP info header of the PLAY response. This contains the sequence number and timestamp info.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   *
   * The setup method selects a transport from the options proposed by the client.
   */
  virtual ResponseCode handlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control PAUSE request behaviour.
   * @param[in] pause The RTSP PAUSE message.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handlePause(const RtspMessage& pause, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control TEARDOWN request behaviour.
   * @param[in] teardown The RTSP TEARDOWN message.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control GET_PARAMETER request behaviour.
   * @param[in] getParameter The RTSP GET_PARAMETER message.
   * @param[in] sContentType The RTSP content type of the content.
   * @param[in] sContent The content of the GET_PARAMETER message body.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handleGetParameter(const RtspMessage& getParameter, std::string& sContentType,
                                          std::string& sContent, const std::vector<std::string>& vSupported);
  /**
   * @brief Subclasses can implement this to control SET_PARAMETER request behaviour.
   * @param[in] setParameter The RTSP SET_PARAMETER message.
   * @param[in] vSupported A vector containing the Supported header of the RTSP request.
   */
  virtual ResponseCode handleSetParameter(const RtspMessage& setParameter, const std::vector<std::string>& vSupported);
  /**
   * @brief Method to retrieve the type and version of the RTSP agent.
   */
  virtual std::string getServerInfo() const { return std::string("rtp++ RTSP Agent ") + RTP_PLUS_PLUS_VERSION_STRING; }
  /**
   * @brief checks if the specified resource is valid in the context of the RTSP agent.
   */
  virtual bool checkResource(const std::string& sResource);
  /**
   * @brief Selects a transport from the vector of proposed transport options.
   */
  virtual boost::optional<Transport> selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports);
  /**
   * @brief Returns if this RTSP agent supports the experimental live555 REGISTER request.
   */
  virtual bool supportsRegisterRequest() const;
};

} // rfc2326
} // rtp_plus_plus
