#pragma once
#include <cstdint>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

// fwd
class TransactionUser;

/// enum for INVITE server transaction states
enum ServerInviteTransactionState
{
  STXS_PROCEEDING,
  STXS_COMPLETED,
  STXS_CONFIRMED,
  STXS_TERMINATED
};

static std::string toString(ServerInviteTransactionState eState)
{
  switch (eState)
  {
  case STXS_PROCEEDING:
    return "STXS_PROCEEDING";
  case STXS_COMPLETED:
    return "STXS_COMPLETED";
  case STXS_CONFIRMED:
    return "STXS_CONFIRMED";
  case STXS_TERMINATED:
    return "STXS_TERMINATED";
  }
  return "Unknown server INVITE TX state";
}

/// enum for non-INVITE server transaction states
enum ServerNonInviteTransactionState
{
  STXS_NI_TRYING,
  STXS_NI_PROCEEDING,
  STXS_NI_COMPLETED,
  STXS_NI_TERMINATED
};

static std::string toString(ServerNonInviteTransactionState eState)
{
  switch (eState)
  {
  case STXS_NI_TRYING:
    return "STXS_NI_TRYING";
  case STXS_NI_PROCEEDING:
    return "STXS_NI_PROCEEDING";
  case STXS_NI_COMPLETED:
    return "STXS_NI_COMPLETED";
  case STXS_NI_TERMINATED:
    return "STXS_NI_TERMINATED";
  }
  return "Unknown server non-INVITE TX state";
}
/**
 * @brief The server transaction is responsible for the delivery of requests to
 * the TU and the reliable transmission of responses.  It accomplishes
 * this through a state machine.  Server transactions are created by the
 * core when a request is received, and transaction handling is desired
 * for that request (this is not always the case).
 *
 */
class ServerTransaction
{
  enum CtType
  {
    CT_INVITE,
    CT_NON_INVITE
  };
public:

  /**
   * @brief Server transaction user notifier
   * uint32_t: Transaction ID
   * SipMessage: SIP response
   */
  typedef boost::function<void(const uint32_t, const boost::system::error_code&, const SipMessage&)> ServerTransactionUserNotifier_t;
  /**
   * @brief Server transaction termination handler
   * uint32_t: Transaction ID
   */
   typedef boost::function<void(uint32_t)> ServerTransactionTerminationNotifier_t;
  /**
   * @brief Constructor
   *
   */
  ServerTransaction(boost::asio::io_service& ioService, SipContext& localContext,
    TransactionUser& transactionUser,
    uint32_t uiTxId, TransportLayer& transportLayer,
    const SipMessage& sipRequest, const std::string& sSource, const uint16_t uiPort,
    TransactionProtocol eProtocol,
    ServerTransactionUserNotifier_t tuNotifierCallback,
    ServerTransactionTerminationNotifier_t terminationCallback,
    boost::system::error_code& ec);
  /**
   * @brief Destructor
   */
  ~ServerTransaction();
  /**
   * @brief Getter for request
   */
  const SipMessage& getRequest() const { return m_request; }
  /**
   * @brief Called when a request is handled within the transaction
   */
  void onRequestReceived(const SipMessage& sipRequest);
  /**
   * @brief This method should be called by the core when a request retransmission is received.
   *
   */
  void onRequestRetransmissionReceived(const SipMessage& sipRequest);
  /**
   * @brief This method should be called by the core when an ACK is received.
   *
   */
  void onAckReceived(const SipMessage& sipRequest);
  /**
   * @brief
   */
  boost::system::error_code sendResponse(const SipMessage& sipResponse);
  /**
   * @brief Can be called by TU to send provisional responses
   */
  boost::system::error_code sendProvisionalResponse(const SipMessage& sipProvisionalResponse);
#if 0
  /**
   * @brief generates response
   */
  SipMessage generateResponse(ResponseCode eCode, const SipMessage& sipRequest);
#endif
  /**
   * @brief generates response
   */
  SipMessage generateResponse(ResponseCode eCode);

private:
  /**
   * @brief Timer G is to ...
   */
  void handleTimerGTimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer H determines when the server
   * transaction abandons retransmitting the response. Its value is
   * chosen to equal Timer B, the amount of time a client transaction will
   * continue to retry sending a request.
   */
  void handleTimerHTimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer I is to ...
   */
  void handleTimerITimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer J is to ...
   */
  void handleTimerJTimeout(const boost::system::error_code& ec);

private:
  /**
   * @brief method to update INVITE ServerTransaction state
   */
  void updateInviteState(ServerInviteTransactionState eState);
  /**
   * @brief method to update non-INVITE ServerTransaction state
   */
  void updateNonInviteState(ServerNonInviteTransactionState eState);

private:
  /// Tx type
  CtType m_eTxType;
  /// INVITE transaction state
  ServerInviteTransactionState m_eInviteState;
  /// Non-INVITE transaction state
  ServerNonInviteTransactionState m_eNonInviteState;
  /// timer G in RFC3261
  boost::asio::deadline_timer m_timerG;
  /// timer H in RFC3261
  boost::asio::deadline_timer m_timerH;
  /// timer I in RFC3261
  boost::asio::deadline_timer m_timerI;
  /// timer J in RFC3261
  boost::asio::deadline_timer m_timerJ;
  /// TX ID
  uint32_t m_uiTxId;
  /// Local context
  SipContext m_localContext;
  /// Transport
  TransportLayer& m_transportLayer;
  /// SIP request
  SipMessage m_request;
  /// Source
  std::string m_sSource;
  /// port
  uint16_t m_uiPort;
  /// protocol
  TransactionProtocol m_eProtocol;
  /// TU notifier callback
  ServerTransactionUserNotifier_t m_tuNotifier;
  /// termination callback
  ServerTransactionTerminationNotifier_t m_terminationCallback;
  /// most recently sent provisional response
  SipMessage m_mostRecentProvisionalSipResponse;
  /// final response
  SipMessage m_finalSipResponse;
  /// timer G interval
  uint32_t m_timerGIntervalMs;
  /// TO tag for when it needs to be generated
  boost::optional<std::string> m_toTag;
};

} // rfc3261
} // rtp_plus_plus
