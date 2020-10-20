#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/ClientTransaction.h>
#include <rtp++/rfc3261/DialogManager.h>
#include <rtp++/rfc3261/TransactionUser.h>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @typedef Invite notification handler (Transaction ID, SIP Message)
 * This callback expects the called method to complete the INVITE by invoking accept or reject
 * using uiTxId.
 * @TODO: rename more appropriate: also using for BYE
 */
typedef boost::function<void(const uint32_t uiTxId, const SipMessage&)> InviteNotificationHandler_t;

/**
 * @typedef Notification handler (Tx ID, SipMessage, ResponseCode)
 * This callback expects the called method to set the response code of the SipMessage.
 */
typedef boost::function<void(const uint32_t uiTxId, const SipMessage&, SipMessage&)> SynchronousSipMessageNotificationHandler_t;

/**
 * @typedef error_code This will be called once the UAC has completed shutting down
 */
typedef boost::function<void(const boost::system::error_code)> ShutdownCompletionHandler_t;

/**
 * @brief The UserAgentServer class implements the UAS functionality as defined in RFC3261.
 *
 */
class UserAgentServer : public TransactionUser
{
public:
  /**
   * @brief Constructor
   */
  UserAgentServer(boost::asio::io_service& ioService, const SipContext& sipContext, DialogManager& dialogManager, 
    TransportLayer& transportLayer, boost::system::error_code& ec);
  /**
   * @brief stops the UAC, on completion of all UAC tasks, m_shutdownCompletionHandler will be called.
   */
  boost::system::error_code shutdown();
  /**
   * @brief Handler for incoming SIP messages from transport layer.
   */
  void handleIncomingRequestFromTransportLayer(const SipMessage& sipRequest, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief handles incoming requests from transaction layer
   */
  virtual void processIncomingRequestFromTransactionLayer(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse);
  /**
   * @brief informs the user agent that the local side is ringing
   */
  boost::system::error_code ringing(const uint32_t uiTxId);
  /**
   * @brief accepts an incoming call
   */
  boost::system::error_code accept(const uint32_t uiTxId, const MessageBody& sessionDescription);
  /**
   * @brief rejects an incoming call
   */
  boost::system::error_code reject(const uint32_t uiTxId);

public:
  /**
   * @brief Setter for INVITE handler
   */
  void setInviteHandler(InviteNotificationHandler_t onInvite) { m_onInvite = onInvite; }
  /**
   * @brief Setter for REGISTER handler
   */
  void setRegisterHandler(SynchronousSipMessageNotificationHandler_t onRegister) { m_onRegister = onRegister; }
  /**
   * @brief Setter for BYE handler
   */
  void setByeHandler(InviteNotificationHandler_t onBye) { m_onBye = onBye; }
  /**
   * @brief Setter for shutdown completion handler
   */
  void setShutdownCompletionHandler(ShutdownCompletionHandler_t shutdownCompletionHandler) { m_shutdownCompletionHandler = shutdownCompletionHandler; }

private:
  void processRequestInsideOfDialog(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse);
  void processRequestOutsideOfDialog(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse);
  /**
   * @brief Specifies whether a SIP method is allowed by the UAS
   */
  bool isMethodAllowed(Method eMethod) const;

private:
  DialogManager& m_dialogManager;
  /// INVITE callback
  InviteNotificationHandler_t m_onInvite;
  /// BYE callback
  InviteNotificationHandler_t m_onBye;
  /// REGISTER callback
  SynchronousSipMessageNotificationHandler_t m_onRegister;
  /// Shutdown completion handler
  ShutdownCompletionHandler_t m_shutdownCompletionHandler;
};

} // rfc3261
} // rtp_plus_plus
