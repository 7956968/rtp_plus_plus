#pragma once
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/media/IVideoDevice.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipUri.h>
#include <rtp++/rfc3261/UserAgent.h>
#include <rtp++/rfc3264/OfferAnswerModel.h>
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief The SipAgent class is used to create a SIP session with the specified SIP URI.
 *
 * @TODO: add state machine that checks which methods can be called. i.e. in CALLING state,
 * you can only hand-up and not reject.
 */
class SipAgent
{
public:
  typedef boost::function<void(boost::system::error_code)> OnSessionStartHandler_t;
  typedef boost::function<void(boost::system::error_code)> OnSessionEndHandler_t;
  enum SipAgentState
  {
    SA_INIT,               // initial state
    SA_DIALING_OR_RINGING, // dialing or ringing
    SA_CALL_IN_PROGRESS,   // in progress
    SA_ERROR
  };

  /**
   * @brief Constructor
   */
  SipAgent(boost::system::error_code& ec,
           boost::asio::io_service& ioService,
           const GenericParameters &applicationParameters,
           const SipContext& sipContext,
           std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswer,
           std::shared_ptr<media::IVideoDevice> pVideoDevice = std::shared_ptr<media::IVideoDevice>(),
           std::shared_ptr<media::IVideoDevice> pAudioDevice = std::shared_ptr<media::IVideoDevice>()
           );
  /**
   * @brief sets the SIP REGISTER server
   * @return An error code if the URI could not be parsed.
   */
  boost::system::error_code setRegisterServer(const std::string& sSipRegisterServer);
  /**
   * @brief Setter for if proxy should be used for outgoing calls
   */
  void setUseSipProxy(bool bUseSipProxy)
  {
    m_bUseSipProxy = bUseSipProxy;
  }
  /**
   * @brief Setter for if agent should register with registrar
   */
  void setRegisterProxy(bool bRegisterWithProxy)
  {
    m_bRegisterWithProxy = bRegisterWithProxy;
  }
  /**
   * @brief Setter for REGISTER response callback
   */
  void setRegisterResponseHandler(SipResponseHandler_t onRegisterResponse) { m_onRegisterResponse = onRegisterResponse; }
  /**
   * @brief Setter for callback on INVITE.
   */
  void setOnInviteHandler(InviteNotificationHandler_t onInvite) { m_onInvite = onInvite; }
  /**
   * @brief Setter for INVITE response callback
   */
  void setInviteResponseHandler(SipResponseHandler_t onInviteResponse) { m_onInviteResponse = onInviteResponse;  }
  /**
   * @brief configures the callback to be called before the session starts.
   */
  void setOnSessionStartHandler(OnSessionStartHandler_t handler) { m_onSessionStartHandler = handler; }
  /**
   * @brief configures the callback to be called once the session ends.
   */
  void setOnSessionEndHandler(OnSessionEndHandler_t handler) { m_onSessionEndHandler = handler; }
  /**
   * @brief Allow the public IP address to be set. This will be used in the offer/answer exchange overriding
   * the SipContext FQDN. This parameter is also extracted from the application parameters if present.
   */
  void setPublicIpAddress(const std::string& sPublicIp) { m_sPublicIp = sPublicIp; }
  /**
   * @brief starts the SIP agent
   */
  boost::system::error_code start();
  /**
   * @brief stops the SIP agent
   */
  boost::system::error_code stop();
  /**
   * @brief starts the SIP call
   */
  boost::system::error_code call(const std::string& sCallee);
  /**
   * @brief informs the SIP agent that the local side is ringing
   */
  boost::system::error_code ringing(const uint32_t uiTxId);
  /**
   * @brief hangs up the active call
   */
  boost::system::error_code hangup();
  /**
   * @brief accept the SIP call
   */
  boost::system::error_code accept(const uint32_t uiTxId);
  /**
   * @brief reject the SIP call
   */
  boost::system::error_code reject(const uint32_t uiTxId);
  /**
   * @brief This is invoked by the UserAgentClient when a response to an REGISTER is received.
   */
  void onRegisterResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);
  /**
   * @brief Handler for incoming INVITE messages. Invoked by UAS. This allows the application to start a ringing tone, display
   * a UI for the user to accept or reject the call, etc.
   */
  void onInvite(const uint32_t uiTxId, const SipMessage& invite);
  /**
   * @brief This is invoked by the UserAgentClient when a response to an INVITE is received.
   */
  void onInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);
  /**
   * @brief Handler for incoming BYE messages. Invoked by UAS.
   */
  void onBye(const uint32_t uiTxId, const SipMessage& bye);
  /**
   * @brief This is invoked by the UserAgentClient when a response to a BYE is received.
   */
  void onByeResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);

public:
  /**
   * @brief Getter for local SDP. May be offer or answer depending on who initiated the call.
   */
  boost::optional<rfc4566::SessionDescription> getLocalSdp() const { return m_localSdp; }
  /**
   * @brief Getter for remote SDP. May be offer or answer depending on who initiated the call.
   */
  boost::optional<rfc4566::SessionDescription> getRemoteSdp() const { return m_remoteSdp; }

  /**
   * @brief configures the video device
   */
  void setVideoDevice(std::shared_ptr<media::IVideoDevice> pVideoDevice);
  /**
   * @brief configures the audio device
   */
  void setAudioDevice(std::shared_ptr<media::IVideoDevice> pAudioDevice);
  /**
   * @brief configures the audio and video devices
   */
  void setDevices(std::shared_ptr<media::IVideoDevice> pVideoDevice = std::shared_ptr<media::IVideoDevice>(),
                  std::shared_ptr<media::IVideoDevice> pAudioDevice = std::shared_ptr<media::IVideoDevice>());
  /**
   * @brief Getter for MediaSession
   */
  boost::shared_ptr<SimpleMediaSessionV2> getMediaSession() { return m_pMediaSession; }
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
   * @brief Callback to receive frames from video device
   */
  boost::system::error_code getAccessUnitsFromVideoDevice(const std::vector<media::MediaSample>& accessUnit);
  /**
   * @brief Callback to receive frames from audio device
   */
  boost::system::error_code getAccessUnitsFromAudioDevice(const std::vector<media::MediaSample>& accessUnit);

private:
  static std::string stateToString(SipAgent::SipAgentState eState);
  void updateState(SipAgentState eState);

  /**
   * @brief constructs a media session using the local and remote session descriptions
   */
  boost::system::error_code startMediaSession();
  /**
   * @brief stops media session
   */
  boost::system::error_code stopMediaSession();
  /**
   * @brief processes the answer sent by the remote host
   */
  void processAnswer();

private:
  /// State
  SipAgentState m_eState;
  /// Callback for INVITEs
  InviteNotificationHandler_t m_onInvite;
  /// INVITE response callback
  SipResponseHandler_t m_onInviteResponse;
  /// REGISTER response callback
  SipResponseHandler_t m_onRegisterResponse;
  /// Callback for BYEs
  InviteNotificationHandler_t m_onBye;
  /// BYE response callback
  SipResponseHandler_t m_onByeResponse;
  /// Callback to be called on session start
  OnSessionStartHandler_t m_onSessionStartHandler;
  /// Callback to be called on session end
  OnSessionEndHandler_t m_onSessionEndHandler;
  /// io service
  boost::asio::io_service& m_ioService;
  /// app parameters
  GenericParameters m_applicationParameters;
  /// SIP context
  SipContext m_sipContext;
  /// Offer/answer
  std::shared_ptr<rfc3264::OfferAnswerModel> m_pOfferAnswerModel;
  /// video device
  std::shared_ptr<media::IVideoDevice> m_pVideoDevice;
  /// audio device
  std::shared_ptr<media::IVideoDevice> m_pAudioDevice;
  /// SIP UA
  UserAgent m_userAgent;
  /// sip proxy/REGISTER server
  boost::optional<SipUri> m_pSipProxy;
  /// Use proxy for outgoing calls
  bool m_bUseSipProxy;
  /// Register with proxy
  bool m_bRegisterWithProxy;
  /// is registered with registrar
  bool m_bIsRegistered;
  /// is caller or callee: only valid during call initiation and call
  bool m_bIsCaller;
  /// var to store active Tx ID
  uint32_t m_uiTxId;
  /// remote sip client
  boost::optional<SipUri> m_pSipCallee;
  /// MediaSession
  boost::shared_ptr<SimpleMediaSessionV2> m_pMediaSession;
  /// Local SDP used for session setup
  boost::optional<rfc4566::SessionDescription> m_localSdp;
  /// Remote SDP used for session setup
  boost::optional<rfc4566::SessionDescription> m_remoteSdp;
  /// public IP address
  std::string m_sPublicIp;
};

} // rfc3261
} // rtp_plus_plus
