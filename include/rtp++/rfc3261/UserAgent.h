#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/ClientTransaction.h>
#include <rtp++/rfc3261/DialogManager.h>
#include <rtp++/rfc3261/SipDialog.h>
#include <rtp++/rfc3261/TransactionUser.h>
#include <rtp++/rfc3261/TransportLayer.h>
#include <rtp++/rfc3261/UserAgentBase.h>
#include <rtp++/rfc3261/UserAgentClient.h>
#include <rtp++/rfc3261/UserAgentServer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief The UserAgent class can be used to send SIP messages to a UAS,
 * as well as receive SIP messages from a UAC.
 *
 */
class UserAgent : public UserAgentBase
{
  friend class UserAgentClient;
  friend class UserAgentServer;
public:
  /**
   * @brief Constructor
   */
  UserAgent(boost::asio::io_service& ioService, const SipContext& sipContext, boost::system::error_code& ec);
  /**
   * @brief registers with REGISTER server
   */
  boost::system::error_code registerWithServer(const SipUri& sipServer, const std::string& to, const std::string& from, uint32_t uiExpiresSeconds, uint32_t& uiTxId);
  /**
   * @brief starts a session by sending an INVITE using the UAC
   */
  boost::system::error_code initiateSession(const std::string& to, const std::string& from, const std::string& sDomain, uint32_t& uiTxId,
                                            const MessageBody& sessionDescription, const std::string& sProxy = "");
  /**
   * @brief terminates a session
   */
  boost::system::error_code terminateSession(const uint32_t uiTxId);
  /**
   * @brief informs the user agent that the local side is ringing
   */
  boost::system::error_code ringing(const uint32_t uiTxId);
  /**
   * @brief accept the SIP call
   */
  boost::system::error_code accept(const uint32_t uiTxId, const MessageBody& sessionDescription);
  /**
   * @brief rejects an incoming call
   */
  boost::system::error_code reject(const uint32_t uiTxId);

public:
  /**
   * @brief Setter for callback on INVITE.
   */
  void setOnInviteHandler(InviteNotificationHandler_t onInvite) { m_onInvite = onInvite; }
  /**
   * @brief Setter for invite response callback
   */
  void setInviteResponseHandler(SipResponseHandler_t onInviteResponse) { m_onInviteResponse = onInviteResponse;  }
  /**
   * @brief Setter for callback on REGISTER.
   */
  void setRegisterHandler(SynchronousSipMessageNotificationHandler_t onRegister) { m_onRegister = onRegister; }
  /**
   * @brief Setter for REGISTER response callback
   */
  void setRegisterResponseHandler(SipResponseHandler_t onRegisterResponse) { m_onRegisterResponse = onRegisterResponse; }
  /**
   * @brief Setter for callback on BYE.
   */
  void setOnByeHandler(InviteNotificationHandler_t onBye) { m_onBye = onBye; }
  /**
   * @brief Setter for BYE response callback
   */
  void setByeResponseHandler(SipResponseHandler_t onByeResponse) { m_onByeResponse = onByeResponse; }

private:
  /**
   * @brief overridden from UserAgentBase
   */
  virtual void doHandleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief overridden from UserAgentBase
   */
  virtual boost::system::error_code doStart();
  /**
   * @brief overridden from UserAgentBase
   */
  virtual boost::system::error_code doStop();
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
   * @brief Handler for incoming REGISTER messages. Invoked by UAS.
   */
  void onRegister(const uint32_t uiTxId, const SipMessage& invite, SipMessage& sipResponse);
  /**
   * @brief This is invoked by the UserAgentClient when a response to an REGISTER is received.
   */
  void onRegisterResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);
  /**
   * @brief Handler for incoming INVITE messages. Invoked by UAS. This allows the application to start a ringing tone, display
   * a UI for the user to accept or reject the call, etc.
   */
  void onBye(const uint32_t uiTxId, const SipMessage& bye);
  /**
   * @brief This is invoked by the UserAgentClient when a response to an BYE is received.
   */
  void onByeResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse);

private:
  /**
   * @brief initialises UAC and UAS
   */
  void initialiseUacAndUas(const SipContext& sipContext, boost::system::error_code& ec);
  /**
   * @brief terminates active dialogs
   */
  void terminateDialogs();

private:
  /// Dialog manager
  DialogManager m_dialogManager;
  /// UAC
  std::unique_ptr<UserAgentClient> m_pUac;
  /// UAS
  std::unique_ptr<UserAgentServer> m_pUas;
  /// Callback for INVITEs
  InviteNotificationHandler_t m_onInvite;
  /// INVITE response callback
  SipResponseHandler_t m_onInviteResponse;
  /// Callback for REGISTERs
  SynchronousSipMessageNotificationHandler_t m_onRegister;
  /// REGISTER response callback
  SipResponseHandler_t m_onRegisterResponse;
  /// Callback for INVITEs
  InviteNotificationHandler_t m_onBye;
  /// BYE response callback
  SipResponseHandler_t m_onByeResponse;
  /// map to store caller/callee info, <txid, isCaller>
  std::map<uint32_t, bool> m_mCallerMap;
};

} // rfc3261
} // rtp_plus_plus
