#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc3261/Registrar.h>
#include <rtp++/rfc3261/UserAgent.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

class SipProxy
{
public:
  typedef boost::function<void(boost::system::error_code)> OnSessionEndHandler_t;
  /**
   * @brief Constructor
   */
  SipProxy(boost::asio::io_service& ioService, const SipContext& sipContext, boost::system::error_code& ec);
  /**
   * @brief starts the SIP proxy
   */
  boost::system::error_code start();
  /**
   * @brief stops the SIP proxy
   */
  boost::system::error_code stop();
  /**
   * @brief Setter for REGISTER response callback
   */
  void setRegisterHandler(SynchronousSipMessageNotificationHandler_t onRegister) { m_onRegister = onRegister; }
  /**
   * @brief Setter for callback on INVITE.
   */
  void setOnInviteHandler(InviteNotificationHandler_t onInvite) { m_onInvite = onInvite; }
  /**
   * @brief Setter for INVITE response callback
   */
  void setInviteResponseHandler(SipResponseHandler_t onInviteResponse) { m_onInviteResponse = onInviteResponse;  }
  /**
   * @brief configures the callback to be called once the session ends.
   */
  void setOnSessionEndHandler(OnSessionEndHandler_t handler) { m_onSessionEndHandler = handler; }
  /**
   * @brief Handler for incoming REGISTER messages. Invoked by UAS. 
   */
  void onRegister(const uint32_t uiTxId, const SipMessage& invite, SipMessage& sipResponse);
  /**
   * @brief Handler for incoming INVITE messages. Invoked by UAS. This allows the application to start a ringing tone, display
   * a UI for the user to accept or reject the call, etc.
   */
  void onInvite(const uint32_t uiTxId, const SipMessage& invite);
  /**
   * @brief This is invoked by the UserAgentClient when a response to an INVITE is received.
   */
  void onInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);

private:
  /// Callback to be called on session end
  OnSessionEndHandler_t m_onSessionEndHandler;
  /// io service
  boost::asio::io_service& m_ioService;
  /// registrar
  Registrar m_registrar;
  /// network manager
  MediaSessionNetworkManager m_mediaSessionNetworkManager;
  /// SIP UA
  UserAgent m_userAgent;
  /// REGISTER callback
  SynchronousSipMessageNotificationHandler_t m_onRegister;
  /// Callback for INVITEs
  InviteNotificationHandler_t m_onInvite;
  /// INVITE response callback
  SipResponseHandler_t m_onInviteResponse;

};

} // rfc3261
} // rtp_plus_plus
