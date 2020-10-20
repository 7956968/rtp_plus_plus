#pragma once
#include <cstdint>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/function.hpp>
#include <rtp++/rfc3261/Protocols.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

enum ClientTransactionState
{
  // calling is for INVITES, trying for non-INVITES
  CTXS_CALLING_OR_TRYING,
  CTXS_PROCEEDING,
  CTXS_COMPLETED,
  CTXS_TERMINATED
};

static std::string toString(ClientTransactionState eState)
{
  switch (eState)
  {
  case CTXS_CALLING_OR_TRYING:
    return "CTXS_CALLING_OR_TRYING";
  case CTXS_PROCEEDING:
    return "CTXS_PROCEEDING";
  case CTXS_COMPLETED:
    return "CTXS_COMPLETED";
  case CTXS_TERMINATED:
    return "CTXS_TERMINATED";
  default:
    return "Unknown";
  }
}

/**
 * @brief
 *
 * Client INVITE Transaction state: calling, proceeding, completed, terminated
 * Client Non-INVITE transactions
 */
class ClientTransaction
{
public:

  enum CtType
  {
    CT_INVITE,
    CT_NON_INVITE
  };

  /**
   * @brief Client transaction response handler
   * boost::system::error_code: error_code
   * uint32_t: Transaction ID
   * SipMessage: SIP request
   * SipMessage: SIP response
   */
  typedef boost::function<void(const boost::system::error_code&, const uint32_t, const SipMessage& , const SipMessage&)> ClientTransactionUserNotifier_t;
  /**
   * @brief Client transaction termination handler
   * uint32_t: Transaction ID
   */
  typedef boost::function<void(uint32_t)> ClientTransactionTerminationNotifier_t;

  /**
   * @brief Constructor The creation of a ClientTransaction results in the sipMessage being sent automatically
   * @param ec Will be set to an error code if anything went wrong with the transaction
   */
  ClientTransaction(boost::asio::io_service& ioService, uint32_t uiTxId, TransportLayer& transportLayer,
    const SipMessage& sipRequest, const std::string& sDestination, const uint16_t uiPort, 
    TransactionProtocol eProtocol, 
    ClientTransactionUserNotifier_t responseCallback,
    ClientTransactionTerminationNotifier_t terminationCallback,
    boost::system::error_code& ec);
  /**
   * @brief Destructor
   */
  ~ClientTransaction();
  /** 
   * @brief Getter for request
   */
  const SipMessage& getRequest() const { return m_originalRequest; }
  /**
   * @brief 
   */
  boost::system::error_code cancelRequest();
  /**
   * @brief This method must be called once a response has been received for an INVITE request
   *
   * It allows the ClientTransaction to stop timers
   */
  void onInviteResponseReceived(const SipMessage& sipMessage);
  /**
   * @brief This method must be called once a response has been received for an INVITE request
   *
   */
  void onNonInviteResponseReceived(const SipMessage& sipMessage);
  /**
   * @brief This method must be called once a response has been received for an INVITE request
   *
   */
  void onResponseReceived(const SipMessage& sipMessage);

private:
  /**
   * @brief Timer A timeout is for retransmissions when using unreliable protocols
   */
  void handleTimerATimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer B is to handle transaction timeouts
   */
  void handleTimerBTimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer D is to handle time in COMPLETED state
   */
  void handleTimerDTimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer E is to handle retransmissions for non-INVITEs with unreliable protocols
   */
  void handleTimerETimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer F is to handle time in COMPLETED state for non-INVITEs
   */
  void handleTimerFTimeout(const boost::system::error_code& ec);
  /**
   * @brief Timer K is to handle time in COMPLETED state for non-INVITEs
   */
  void handleTimerKTimeout(const boost::system::error_code& ec);

private:
  /**
   * @brief begins tx for INVITE transactions
   */
  boost::system::error_code beginInviteTransaction();
  /**
   * @brief begins tx for non-INVITE transactions
   */
  boost::system::error_code beginNonInviteTransaction();
  /**
   * @brief Sends an ACK in response to sipResponse
   */
  void sendAck(const SipMessage& sipResponse);
  /**
   * @brief updates the TX state
   */
  void updateState(ClientTransactionState eState);
private:
  /// io service
  boost::asio::io_service& m_ioService;
  /// timer A in RFC3261
  boost::asio::deadline_timer m_timerA;
  /// timer B in RFC3261
  boost::asio::deadline_timer m_timerB;
  /// timer D in RFC3261
  boost::asio::deadline_timer m_timerD;
  /// timer E in RFC3261
  boost::asio::deadline_timer m_timerE;
  /// timer F in RFC3261
  boost::asio::deadline_timer m_timerF;
  /// timer K in RFC3261
  boost::asio::deadline_timer m_timerK;
  /// TX id
  uint32_t m_uiTxId;
  /// transport layer
  TransportLayer& m_transportLayer;
  /// SIP message
  SipMessage m_originalRequest;
  /// Destination
  std::string m_sDestination;
  /// port
  uint16_t m_uiPort;
  /// protocol
  TransactionProtocol m_eProtocol;
  /// Response notification callback
  ClientTransactionUserNotifier_t m_tuNotifier;
  /// Tx termination callback
  ClientTransactionTerminationNotifier_t m_terminationNotifier;
  /// tx state
  ClientTransactionState m_eTxState;
  /// TX type
  CtType m_eTxType;
  /// Last interval used
  uint32_t m_timerAIntervalMs;
  /// Timer D interval
  uint32_t m_timerDIntervalS;
  /// Timer E interval
  uint32_t m_timerEIntervalMs;
  /// flag whether TU has been notifed of final response
  bool m_bTuHasBeenNotififedOfFinalResponse;
};

} // rfc3261
} // rtp_plus_plus
