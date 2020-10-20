#pragma once
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc2326/RtspClient.h>
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief class RtspClientSession object manages an entire RTSP session.
 *
 * The RTSP client session uses the boost asio io_service for asynchronous I/O
 * and event handling. The session should be started by calling start() and stopped
 * by calling shutdown(). The setOnSessionEndHandler() callback should be configured
 * to automatically stop the application once the RTSP client session completes, either
 * due to error, or due to the session being complete.
 */
class RtspClientSession
{
  typedef boost::function<void(const boost::system::error_code&)> OnSessionPlayHandler_t;
  typedef boost::function<void()> OnSessionEndHandler_t;
  typedef std::tuple<size_t, rfc4566::MediaDescription, Transport> MediaTransportDescription_t;

public:

  /**
   * @brief Constructor for RtspClientSession object.
   *
   * @param[in] ioService boost asio I/O service
   * @param[in] applicationParameters contains configuration parameters that can be used to setup the RTSP session.\n
   * \li use-rtp-rtsp,t: configures the client to request RTP over RTSP/TCP transport
   * \li client-rtp-port,r: desired starting port for RTP
   * \li enable-mprtp: configures the client to use MPRTP transport if the server supports it.
   * \li rtcp-mux: configures the client to request rtcp-mux Transport if the server supports it.
   * \li dest-mprtp-addr: overrides the Transport header dest_mprtp_addr field. This can be used to override the
   * local interfaces allowing the use of RTP proxies.
   * \li dur,d: duration of media session in seconds.
   * \li options-only,O: Stops after completing the OPTIONS request
   * \li describe-only,D: Stops after completing the DESCRIBE request
   * \li audio-only,A: only sets up audio session if available
   * \li video-only,V: only sets up video session if available
   * \li external-rtp,x: should be used in connection with client-rtp-port. In this case an external application will be
   * used to receive the RTP/RTCP packets.
   * @param[in] rtspUri An RtspUri object representing the RTSP URI that the RTSP session will use.
   * @param[in] vLocalInterfaces A vector of string representations of local interface addresses. If these are specified, 
   * this results in the said interfaces being used to setup the local ports. This can be useful to force the use of an
   * interface, or to specify multiple interfaces for MPRTP sessions. In the case of specifying multiple interfaces, the 
   * primary interface should be first.
   */
  RtspClientSession(boost::asio::io_service& ioService, const GenericParameters& applicationParameters, 
                    const RtspUri& rtspUri, 
                    const std::vector<std::string>& vLocalInterfaces = std::vector<std::string>());
  
  /**
   * @brief Destructor
   */
  virtual ~RtspClientSession();
  /**
   * @brief start() method starts the RTSP client session chain of events
   */
  boost::system::error_code start();
  /**
   * @brief stop() method stops the RTSP client session chain of events. 
   *
   * Once the RTSP session is complete, m_onSessionEndHandler is called if configured.
   */
  boost::system::error_code shutdown();
  /**
   * @brief Getter for SDP
   */
  const boost::optional<rfc4566::SessionDescription> getSessionDescription() const { return m_sdp; }
  /**
   * @brief Getter for media session
   */
  boost::shared_ptr<SimpleMediaSessionV2> getMediaSession() { return m_pMediaSession; }
  /**
   * @brief configures the callback to be called once the session starts.
   */
  void setOnSessionStartHandler(OnSessionPlayHandler_t handler) { m_onSessionStartHandler = handler; }
  /**
   * @brief configures the callback to be called once the session ends.
   */
  void setOnSessionEndHandler(OnSessionEndHandler_t handler) { m_onSessionEndHandler = handler; }
  /**
   * @brief Register callbacks for incoming audio samples
   */
  boost::system::error_code registerAudioConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingAudio);
  /**
   * @brief Register callbacks for incoming video samples
   */
  boost::system::error_code registerVideoConsumer(media::IMediaSampleSource::MediaCallback_t onIncomingVideo);

private:

  /**
   * @brief selectTransportForMediaDescription The implementation should select a transport for the passed in media description
   *
   * TODO: returns a vector of possible transports so that we can offer e/g both AVP and AVPF
   * @param mediaDescription
   * @return This method should return a null pointer if the media subsession is not supported and should not be setup
   */
  virtual boost::optional<Transport> selectTransportForMediaDescription(uint32_t uiIndexInSdp, rfc4566::MediaDescription& mediaDescription);
  /**
   * @brief onSessionSetup The subclass can override this to open UDP ports, start receive operations, etc.
   * @param vSelectedTransports The transport parameters for the media that have been issued by the RTSP server.
   * @return true of the subclass is able to perform the required session initialisation and false if not. If
   * true is returned, an RTSP PLAY is issued, if false, an RTSP TEARDOWN
   */
  virtual bool onSessionSetup(std::vector<MediaTransportDescription_t>& vSelectedTransports);
  /**
   * @brief onSessionTeardown
   */
  virtual void onSessionTeardown();
  /**
   * @brief setupSelectedSessions Attempts to setup outstanding media subsessions
   * @return true if a new setup has been issued
   */
  bool setupSelectedSessions();
  /**
   * @brief Callback called on OPTIONS completion
   */
  void handleOptionsResponse(const boost::system::error_code& ec, const std::vector<std::string>& vOptions, const std::vector<std::string>& vSupported);
  /**
   * @brief Callback called on DESCRIBE completion
   */
  void handleDescribeResponse(const boost::system::error_code& ec, const MessageBody& messageBody, const std::vector<std::string>& vSupported);
  /**
   * @brief Callback called on SETUP completion
   */
  void handleSetupResponse(const boost::system::error_code& ec, const std::string& sSession, const Transport& transport, const std::vector<std::string>& vSupported);
  /**
   * @brief Callback called on PLAY completion
   */
  void handlePlayResponse(const boost::system::error_code& ec, const std::vector<std::string>& vSupported);
  /**
   * @brief Callback called on TEARDOWN completion
   */
  void handleTeardownResponse(const boost::system::error_code& ec, const std::vector<std::string>& vSupported);
  /**
   * @brief Callback called once media sessions is complete
   */
  void onMediaSessionComplete();
  /**
   * @brief Callback called once desired session duration has been reached if this has been specified
   */
  void onTimeout(const boost::system::error_code& ec);

  bool isInterleavedMediaSession(const std::vector<MediaTransportDescription_t>& vSelectedTransports);
  bool setupRtpOverUdpSession(std::vector<MediaTransportDescription_t>& vSelectedTransports);
  bool setupRtpOverTcpSession(std::vector<MediaTransportDescription_t>& vSelectedTransports);

private:
  bool isSessionUsingMpRtp() const { return m_bEnableMpRtp && m_bDoesServerSupportMpRtp; }
  bool isSessionUsingRtcpMux() const { return m_bRtcpMux && m_bDoesServerSupportRtpRtcpMux; }
  void endClientSession();
  void verifyMpRtpAndRtpRtcpMuxSupportOnServer(const std::vector<std::string>& vSupported);
  std::vector<std::string> getSupported();

  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer;
  GenericParameters m_applicationParameters;
  RtspUri m_rtspUri;
  RtspClient m_rtspClient;
  MediaSessionNetworkManager m_mediaSessionNetworkManager;

  //media::SimpleMultimediaServiceV2 m_multimediaService;
  boost::shared_ptr<SimpleMediaSessionV2> m_pMediaSession;

  boost::optional<rfc4566::SessionDescription> m_sdp;
  std::string m_sSession;

  std::vector<MediaTransportDescription_t> m_vSelectedTransports;

  uint32_t m_uiSetupCount;
  bool m_bInterleaved;

  /// RTSP session state
  enum State
  {
    STATE_INIT,
    STATE_SETUP,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_TEARING_DOWN
  };
  State m_eState;
  /// Component state
  bool m_bShuttingDown;

  uint32_t m_uiDurationSeconds;

  static const uint32_t DEFAULT_RTP_PORT;

  //! preferred RTP port
  uint32_t m_uiRtpPort;
  //! used for RTP over RTSP. Should be incremented by 2 for each subsession
  uint32_t m_uiInterleaved;
  //! is client configured to use MPRTP
  bool m_bEnableMpRtp;
  //! is client configured to use rtcp-mux
  bool m_bRtcpMux;
  //! dest_mprtp_addr is specified on the command line
  std::vector<std::string> m_vDestMpRtpAddr;
  //! forces dest_mprtp_addr if specified. This does not have to reflect the actual addresses/ports used.
  std::vector<std::string> m_vForceDestMpRtpAddr;
  /// Addresses of interfaces for dst_mprtp_addr parameter if specified
  InterfaceDescriptions_t m_interfaceDescriptions;
  //! does server support MPRTP
  bool m_bDoesServerSupportMpRtp;
  //! does server support rtcp-mux
  bool m_bDoesServerSupportRtpRtcpMux;
  /// Callback to be called on session play
  OnSessionPlayHandler_t m_onSessionStartHandler;
  /// Callback to be called on session end
  OnSessionEndHandler_t m_onSessionEndHandler;
};

} // rfc2326
} // rtp_plus_plus
