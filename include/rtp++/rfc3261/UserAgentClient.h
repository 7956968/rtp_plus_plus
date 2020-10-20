#pragma once
#include <map>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/ClientTransaction.h>
#include <rtp++/rfc3261/DialogManager.h>
#include <rtp++/rfc3261/Protocols.h>
#include <rtp++/rfc3261/TransactionUser.h>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @typedef error_code, uiTxId, SipResponse
 */
typedef boost::function<void(const boost::system::error_code&, uint32_t, const SipMessage&)> SipResponseHandler_t;

/**
 * @typedef completion handler
 */
typedef boost::function<void()> CompletionHandler_t;

/**
 * @brief UserAgentClient is used to send SIP messages to a SIP UAS.
 */
class UserAgentClient : public TransactionUser
{
public:
  /**
   * @brief Constructor
   */
  UserAgentClient(boost::asio::io_service& ioService, const SipContext& sipContext, DialogManager& dialogManager, TransportLayer& transportLayer,
    boost::system::error_code& ec, TransactionProtocol ePreferredProtocol = TP_UDP);
  /**
   * @brief shuts the component down
   */
  boost::system::error_code shutdown(CompletionHandler_t handler);
  /**
   * @brief Getter for preferred protocol
   */
  TransactionProtocol getPreferredProtocol() const { return m_ePreferredProtocol; }
  /**
   * @brief registers a SipClient by sending an REGISTER
   * @param[in] sipServer: sip:registrar.chicago.com Note: no user@ allowed!
   * @param[in] to e.g. Carol <sip:carol@chicago.com>
   * @param[in] from e.g. "Bob" <sips:bob@biloxi.com> 
   * @param[in] uiExpiresSeconds The number of seconds that the registration should be valid for. A value of zero will result in deregistration from the server
   */
  boost::system::error_code registerWithServer(const SipUri& sipServer, const std::string& to, const std::string& from, uint32_t uiExpiresSeconds, uint32_t& uiTxId);
  /**
   * @brief starts a session by sending an INVITE
   * @param[in] To: e.g. Carol <sip:carol@chicago.com>
   * @param[in] From: e.g. "Bob" <sips:bob@biloxi.com> 
   */
  boost::system::error_code invite(const std::string& to, const std::string& from, const std::string& domain, uint32_t& uiTxId,
                                   const MessageBody& sessionDescription, const std::string& sProxy = "");
  /**
   * @brief Handles incoming responses from the UAS
   */
  void handleIncomingResponseFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief Handler for incoming responses.
   * @param[in] uiTxId ID of the server transaction.
   * @param[in] sipResponse The incoming SIP response
   */
  virtual void processIncomingResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);

public:
  /**
   * @brief Setter for INVITE response callback
   */
  void setInviteResponseHandler(SipResponseHandler_t onInviteResponse) { m_onInviteResponse = onInviteResponse;  }
  /**
   * @brief Setter for REGISTER response callback
   */
  void setRegisterResponseHandler(SipResponseHandler_t onRegisterResponse) { m_onRegisterResponse = onRegisterResponse; }
  /**
   * @brief Setter for BYE response callback
   */
  void setByeResponseHandler(SipResponseHandler_t onByeResponse) { m_onByeResponse = onByeResponse; }

private:
  /**
   * @brief generates SIP message with required header fields outside of a dialog
   * @param[in] sMethod String representation of SIP method
   * @param[in] to To header field containing URI of destination. May contain a display name
   * @param[in] from From header field containing URI of sender. May contain a display name
   * @param[in] sCallId Call-ID to be used for SIP message.
   * @param[in] sVia Via header field.
   * @param[in] uiMaxForwards Max-Forwards header
   */
  SipMessage generateSipRequest(const std::string& sMethod, const std::string& sRequestUri, const std::string& to, const std::string& from,
                                const std::string& sCallId, const std::string& sVia, uint32_t uiMaxForwards = DEFAULT_MAX_FORWARDS);
  /**
   * @brief generates a call id using the domain (if provided)
   */
  std::string generateCallId(const std::string& sDomain);
  /**
   * @brief generates a tag for the from field
   */
  std::string generateFromTag();
  /**
   * @brief handles REGISTER responses
   */
  void handleRegisterResponse(const boost::system::error_code&, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);
  /**
   * @brief handles INVITE responses
   */
  void handleInviteResponse(const boost::system::error_code&, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);
  /**
   * @brief handles BYE responses
   */
  void handleByeResponse(const boost::system::error_code&, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);
  /**
   * @brief generates ACK
   */
  SipMessage generateAck(const SipMessage& sipRequest, const SipMessage& sipResponse);
  /**
   * @brief Completes INVITE transaction
   */
  void completeInviteTransaction(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const SipMessage& sipResponse);

private:
  /// shutdown flag
  bool m_bShuttingDown;
  /// io service
  boost::asio::io_service& m_ioService;
  /// preferred protocol
  TransactionProtocol m_ePreferredProtocol;
  /// SIP dialog manager
  DialogManager& m_dialogManager;
  /// INVITE response handler
  SipResponseHandler_t m_onInviteResponse;
  /// REGISTER response handler
  SipResponseHandler_t m_onRegisterResponse;
  /// BYE response handler
  SipResponseHandler_t m_onByeResponse;
  /// INVITE responses and ACKs: these need to be stored temporarily so that the ACK can be resent of a response is retransmitted by the UAS
  std::vector<std::pair<SipMessage, SipMessage> > m_vInviteResponsesFromUas;
  /// map to store timers in order to cancel them
  std::map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mTimers;
  /// completion handler 
  CompletionHandler_t m_onCompletion;
};

} // rfc3261
} // rtp_plus_plus
