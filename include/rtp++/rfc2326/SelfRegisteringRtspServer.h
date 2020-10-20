#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtspMessage.h>
#include <rtp++/rfc2326/RtspServer.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/SelfRegisteringRtspServerConnection.h>
#include <rtp++/rfc2326/Transport.h>
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{

/**
 * @brief SelfRegisteringRtspServer is an RTSP server implementation
 * that registers itself with an RTSP proxy or client.
 * It is being written for use with a live source such as in a DirectShow
 * context and wraps the SimpleMediaSession object which is used to deliver 
 * data to a single client: the one that the class registers itself with.
 * 
 * The use of SimpleMediaSession means that at most one audio and one video
 * RTP session are supported. This server is being written for the purpose 
 * of testing MPRTP RTSP.
 */
class SelfRegisteringRtspServer : public rfc2326::RtspServer
{
public:
  static const uint16_t DEFAULT_SERVER_MEDIA_PORT;

  /**
   * @brief SelfRegisteringRtspServer
   *
   * The RTSP server offers variable levels of configurability. The local interfaces can not be specified.
   * In this case the server auto detects the interfaces. Alternatively, the calling code may specify which
   * interfaces are to be used. In this case the server will validate the specified interfaces. For even
   * more control, the vSrcMpRtpAddr may contain the src_mprtp_addr fields to be used per media line.
   * This configures the server to only bind to the specified interface and port combinations.
   * Further, the vForceSrcMpRtpAddr can be specifed so that the server responds with the specified src_mprtp_addr
   * field set in the transport header of the SETUP response.
   * @param ioService[in] The boost asio io_service to be used for async io
   * @param applicationParameters[in] Contains application parameters that can be configured.
   * @param sSdp[in] The SDP describing the live session
   * @param sRtspProxyOrClientAddressDescriptor[in] The host address of the RTSP proxy or client. The RTSP REGISTER
   * request is sent to this address. The address should have the for of rtsp://xxx.xxx.xxx.xxx[:port]
   * @param bReuseRegisterConnection[in] Determines if the RTSP server sends the REGISTER request
   * with the reuse_connection parameter. If the Transport header contains reuse_connection the same TCP connection will be
   * used to listen for the RTSP requests coming from the proxy or client and no accept operation
   * will be started.
   * @param vLocalInterfaces[in] A vector specifying the local interfaces to be used. If these are invalid, an error
   * code will be returned in ec. This can be left empty in which case the server will auto-detect the available
   * network interfaces. This is useful when a machine has multiple interfaces and we want to limit which ones
   * are to be used by the RTSP server.
   * @param[out] ec is used to return error codes the RTSP server failed to initialise. This may be the case 
   * if parameters are incorrect.
   * @param uiPort[in] RTSP server port. If the server fails to bind to the specified port, an error code will be returned in ec.
   * @param sStreamName[in] The stream name to be used for the live RTSP session
   * @param vSrcMpRtpAddr[in] If specified, this forces the RTSP server to bind to the specified interfaces. 
   * If specified the number of items in the vector must match the number of m-lines in the SDP.
   * @param[in] vForceSrcMpRtpAddr the src_mprtp_addr transport header to be used in the SETUP response.
   * This is useful when inserting an RTP proxy between the RTSP server and client and all packets should be sent
   * via the proxy. This should be used with caution as multiple clients may cause different server ports to be allocated.
   * If specified the number of items in the vector must match the number of m-lines in the SDP.
   * @param bInterleavedTransport[in] Specifies if the REGISTER request will contain "interleaved" as the
   * preferred_delivery_protocol
   * @param bRtcpMux[in] Specifies if the server should accept rtcp-mux transport.
   * @param bEnableMpRtp[in] Specified if the server should enable MPRTP transport.
   * @param uiSessionTimeout[in] Time session timeout in seconds.
   */
  SelfRegisteringRtspServer(boost::asio::io_service& ioService, const GenericParameters& applicationParameters,
    const std::string& sSdp, const std::string& sRtspProxyOrClientAddressDescriptor, bool bReuseRegisterConnection, 
    const std::vector<std::string>& vLocalInterfaces, boost::system::error_code& ec, 
    unsigned short uiPort = 554, const std::string& sStreamName = "live", 
    const std::vector<std::string>& vSrcMpRtpAddr = std::vector<std::string>(),
    const std::vector<std::string>& vForceSrcMpRtpAddr = std::vector<std::string>(),
    bool bInterleavedTransport = false, bool bRtcpMux = false, bool bEnableMpRtp = false,
    unsigned uiSessionTimeout = DEFAULT_SESSION_TIMEOUT);
  /**
   * @brief sends a single video media sample.
   */
  bool sendVideo(const media::MediaSample& mediaSample);
  /**
   * @brief sends a vector of video media samples.
   *
   * This will typically be the case when sending a video sample comprised of multiple NAL units. Each NAL unit 
   * must be contained in it's own media sample.
   */
  bool sendVideo(const std::vector<media::MediaSample>& mediaSamples);
  /**
   * @brief sends a single audio media sample.
   */
  bool sendAudio(const media::MediaSample& mediaSample);
  /**
   * @brief sends a vector of audio media samples.
   */
  bool sendAudio(const std::vector<media::MediaSample>& mediaSamples);

protected:

  virtual boost::system::error_code doStart();
  virtual boost::system::error_code doShutdown();

  virtual rfc2326::ResponseCode handleOptions(std::set<rfc2326::RtspMethod>& methods, const std::vector<std::string>& vSupported) const;
  virtual rfc2326::ResponseCode handleSetup(const rfc2326::RtspMessage& setup, std::string& sSession,
                                            rfc2326::Transport& transport, const std::vector<std::string>& vSupported);
  virtual rfc2326::ResponseCode handleTeardown(const rfc2326::RtspMessage& teardown, const std::vector<std::string>& vSupported);

  virtual std::string getServerInfo() const { return std::string("rtp++ RTSP Server with REGISTER and MPRTP support") + RTP_PLUS_PLUS_VERSION_STRING + ":" +  __DATE__; }
  virtual bool checkResource(const std::string& sResource);
  virtual bool isOptionSupportedByServer(const std::string& sOptionOrFeature) const;
  virtual std::vector<std::string> getUnsupported(const rfc2326::RtspMessage& rtspRequest);
  virtual bool isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes);
  virtual bool getResourceDescription(const std::string& sResource, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported, const rfc2326::Transport& transport = rfc2326::Transport());
  virtual boost::optional<rfc2326::Transport> selectTransport(const rfc2326::RtspUri& rtspUri, const std::vector<std::string>& vTransports);
  virtual std::unique_ptr<rfc2326::ServerMediaSession> createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType);
  virtual boost::system::error_code getServerTransportInfo(const rfc2326::RtspUri& rtspUri, const std::string& sSession, rfc2326::Transport& transport);
  virtual void onShutdown();

protected:
  bool isSessionSetup() const;
  void doSendVideo(const media::MediaSample& mediaSample);
  void doSendVideoSamples(const std::vector<media::MediaSample>& mediaSamples);
  void doSendAudio(const media::MediaSample& mediaSample);
  void doSendAudioSamples(const std::vector<media::MediaSample>& mediaSamples);
  void connectToRtspProxyOrClient(const std::string& sRtspProxyOrClientAddressDescriptor);

  /// @RtspServer
  virtual void handleResponse(const rfc2326::RtspMessage& rtspResponse, rfc2326::RtspServerConnection::ptr pConnection);

  virtual void connectErrorHandler(const boost::system::error_code& ec, SelfRegisteringRtspServerConnection::ptr pConnection);

  virtual void handleTimeout(const boost::system::error_code& ec);
  /**
   * @brief validates the src_mprtp_addr parameter if specified
   */
  boost::system::error_code validateSrcMpRtpAddr();
  /**
  * @brief validates the forced src_mprtp_addr parameter if specified
  */
  boost::system::error_code validateForcedSrcMpRtpAddr();
private:

  // storing used ports
  mutable uint16_t m_uiNextServerTransportPort;
  //! Local RTSP URI
  rfc2326::RtspUri m_rtspUri;
  //! SDP string
  std::string m_sSdp;
  //! Local session description
  rfc4566::SessionDescription m_sessionDescription;
  //! Address of procy or client
  std::string m_sRtspProxyOrClientAddressDescriptor;
  //! Flag to reuse register connection
  bool m_bReuseRegisterConnection;
  //! Local interfaces
  std::vector<std::string> m_vLocalInterfaces;
  //! Name of live stream
  std::string m_sStreamName;
  //! src_mprtp_addr field of MPRTP transport header
  std::vector<std::string> m_vSrcMpRtpAddr;
  //! forces src_mprtp_addr field of MPRTP transport header
  std::vector<std::string> m_vForceSrcMpRtpAddr;
  //! Interleaved transport flag
  bool m_bInterleaved;
  //! RTCP-mux flag
  bool m_bRtcpMux;
  //! MPRTP flag
  bool m_bEnableMpRtp;
  //! Timer for REGISTER requests
  boost::asio::deadline_timer m_registerTimer;
  //! SN for outgoing requests
  uint32_t m_uiLastSN;
  //! retry timeout
  uint32_t m_uiRetrySeconds;
  //! session ID of active RTSP session.
  std::string m_sSession;
  //! store connection when reusing connection
  SelfRegisteringRtspServerConnection::ptr m_pConnection;
  
  typedef std::vector<rfc2326::Transport> TransportList_t;
  /// server transports per session
  std::map<std::string, TransportList_t> m_mServerTransports;

  uint8_t m_uiAudioPayloadType;
  uint8_t m_uiVideoPayloadType;
};

} // rtp_plus_plus

