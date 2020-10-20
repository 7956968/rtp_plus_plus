#pragma once
#include <cstdint>
#include <map>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/ClientTransaction.h>
#include <rtp++/rfc3261/Protocols.h>
#include <rtp++/rfc3261/ServerTransaction.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief TransactionUser class as described in RFC3261.
 *
 */
class TransactionUser
{
public:

  /**
   * @brief Constructor
   */
  TransactionUser(boost::asio::io_service& ioService, const SipContext& sipContext, TransportLayer& transportLayer);
  /**
   * @brief Destructor
   */
  virtual ~TransactionUser();
  /**
   * @brief Getter for IO service
   */
  boost::asio::io_service& getIoService() const { return m_ioService; }
  /**
   * @brief begins a client transaction
   */
  boost::system::error_code beginTransaction(uint32_t& uiTxId, const SipMessage& sipMessage, const std::string& sDestination, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief 
   */
  boost::system::error_code cancelTransaction(const uint32_t uiTxId);
  /**
   * @brief 
   */
  boost::system::error_code completeTransaction(const uint32_t uiTxId, const SipMessage& sipMessage);
  /**
   * @brief begins a server transaction
   */
  boost::system::error_code beginServerTransaction(uint32_t& uiTxId, const SipMessage& sipMessage, 
                                                   const std::string& sSource, const uint16_t uiPort, 
                                                   TransactionProtocol eProtocol);
  /**
   * @brief Handler for incoming requests. The transaction
   * must be completed by calling completeTransaction.
   * @param[in] uiTxId ID of the server transaction.
   * @param[in] sipRequest The incoming SIP request
   * @param[out] bTUWillSendResponseWithin200Ms must be set to true if the TU will send a response within 200ms
   *        and false if not. In that case the ServerTransaction will send a provisional response. Otherwise
            eReponseCode should be used as the response code to the request.
   * @param[out] sipResponse The response to be sent if bTUWillSendResponseWithin200Ms == true 
   */
  virtual void processIncomingRequestFromTransactionLayer(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse);

  /**
   * @brief Handler for incoming responses.
   * @param[in] uiTxId ID of the server transaction.
   * @param[in] sipResponse The incoming SIP response
   */
  virtual void processIncomingResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);

public:
  /**
   * @brief Callback for responses from client transactions
   */
  void onClientResponseNotification(const boost::system::error_code& ec, uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse);
  /**
   * @brief Handler for transactions termination.
   */
  void onTransactionTermination(uint32_t uiTxId);
  /**
   * @brief Handler for when beginTransaction is complete
   */
  void onTransactionComplete();
  /**
   * @brief Handler for server transaction
   */
  void onServerTransactionNotification(const uint32_t uiTxId, const boost::system::error_code& ec, const SipMessage& sipResponse);
  /**
   * @brief Handler for server transactions termination.
   */
  void onServerTransactionTermination(uint32_t uiTxId);

protected:
  /// io service
  boost::asio::io_service& m_ioService;
  /// SIP context
  SipContext m_sipContext;
  /// transport layer
  TransportLayer& m_transportLayer;
  /// id for transactions
  uint32_t m_uiLastTxId;
  /// map of client transactions
  std::map<uint32_t, std::unique_ptr<ClientTransaction> > m_mClientTransactions;
  /// map of server transactions
  std::map<uint32_t, std::unique_ptr<ServerTransaction> > m_mServerTransactions;
};

} // rfc3261
} // rtp_plus_plus
