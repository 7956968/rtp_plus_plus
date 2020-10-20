#include "CorePch.h"
#include <rtp++/rfc3261/TransactionUser.h>
#include <boost/bind.hpp>

namespace rtp_plus_plus
{
namespace rfc3261
{

TransactionUser::TransactionUser(boost::asio::io_service& ioService, const SipContext& sipContext, TransportLayer& transportLayer)
  :m_ioService(ioService),
  m_sipContext(sipContext),
  m_transportLayer(transportLayer),
  m_uiLastTxId(0)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::TransactionUser";
#endif
}

TransactionUser::~TransactionUser()
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::~TransactionUser";
#endif
}

boost::system::error_code TransactionUser::beginTransaction(uint32_t& uiTxId, const SipMessage& sipMessage, 
                                                            const std::string& sDestination, const uint16_t uiPort, 
                                                            TransactionProtocol eProtocol)
{
  uiTxId = m_uiLastTxId++;
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::beginTransaction: " << uiTxId;
#endif
  boost::system::error_code ec;
  // create new client transaction object
  std::unique_ptr<ClientTransaction> pClientTransaction = std::unique_ptr<ClientTransaction>(
    new ClientTransaction(m_ioService, uiTxId, m_transportLayer,
    sipMessage, sDestination, uiPort, eProtocol, 
    boost::bind(&TransactionUser::onClientResponseNotification, this, _1, _2, _3, _4),
    boost::bind(&TransactionUser::onTransactionTermination, this, _1),
    ec));
  if (ec)
  {
    LOG(WARNING) << "Error sending message: " << ec.message();
  }
  else
  {
    m_mClientTransactions[uiTxId] = std::move(pClientTransaction);
  }
  return ec;
}

boost::system::error_code TransactionUser::cancelTransaction(const uint32_t uiTxId)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::cancelTransaction: " << uiTxId;
#endif
  auto it = m_mClientTransactions.find(uiTxId);
  if (it == m_mClientTransactions.end())
    return boost::system::error_code(boost::system::errc::no_such_file_or_directory, boost::system::generic_category());

  boost::system::error_code ec = it->second->cancelRequest();
  if (ec)
  {
    LOG(WARNING) << "Error cancelling message: " << ec.message();
  }
  return ec;
}

boost::system::error_code TransactionUser::completeTransaction(const uint32_t uiTxId, const SipMessage& sipMessage)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::completeTransaction: " << uiTxId;
#endif
  return boost::system::error_code();
}

// THIS method can be removed?!?
void TransactionUser::onTransactionComplete()
{
//  VLOG(2) << "TransactionUser::completeTransaction: " << uiTxId;

}

boost::system::error_code TransactionUser::beginServerTransaction(uint32_t& uiTxId, const SipMessage& sipRequest, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  uiTxId = m_uiLastTxId++;
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::beginServerTransaction: " << uiTxId;
#endif
  boost::system::error_code ec;
  std::unique_ptr<ServerTransaction> pServerTransaction = std::unique_ptr<ServerTransaction>(
    new ServerTransaction(m_ioService, m_sipContext, *this, uiTxId, m_transportLayer, sipRequest, sSource, uiPort, eProtocol,
                          boost::bind(&TransactionUser::onServerTransactionNotification, this, _1, _2, _3),
                          boost::bind(&TransactionUser::onServerTransactionTermination, this, _1),
                          ec));
  m_mServerTransactions[uiTxId] = std::move(pServerTransaction);
  return ec;
}

void TransactionUser::onServerTransactionNotification(const uint32_t uiTxId, const boost::system::error_code& ec, const SipMessage& sipResponse)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::onServerTransactionNotification: " << uiTxId;
#endif
}

void TransactionUser::onServerTransactionTermination(uint32_t uiTxId)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::onServerTransactionTermination: " << uiTxId;
#endif
  // NB: by removing the unique_ptr from memory, the tx object becomes invalid!!
  m_mServerTransactions.erase(uiTxId);
}

void TransactionUser::processIncomingRequestFromTransactionLayer(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::processIncomingRequestFromTransactionLayer: " << uiTxId;
#endif
  // let the Transport layer handle sending a provisional response
  bTUWillSendResponseWithin200Ms = false;
  // pass on request to the 
}

void TransactionUser::onTransactionTermination(uint32_t uiTxId)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::onTransactionTermination: " << uiTxId;
#endif
  // remove transaction so that further responses are forwarded directly as specified in 17.1.1.2:
  // The client transaction MUST be destroyed the instant it enters the
  // "Terminated" state.This is actually necessary to guarantee correct
  // operation.The reason is that 2xx responses to an INVITE are treated
  // differently; each one is forwarded by proxies, and the ACK handling
  // in a UAC is different.Thus, each 2xx needs to be passed to a proxy
  // core(so that it can be forwarded) and to a UAC core(so it can be
  // acknowledged).No transaction layer processing takes place.
  // Whenever a response is received by the transport, if the transport
  // layer finds no matching client transaction(using the rules of
  // Section 17.1.3), the response is passed directly to the core.Since
  // the matching client transaction is destroyed by the first 2xx,
  // subsequent 2xx will find no match and therefore be passed to the
  // core.

  // NB: by removing the unique_ptr from memory, the tx object becomes invalid!!
  m_mClientTransactions.erase(uiTxId);
}

void TransactionUser::onClientResponseNotification(const boost::system::error_code& ec, uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
#ifdef DEBUG_TX
  VLOG(2) << "TransactionUser::onClientResponseNotification: " << uiTxId;
#endif
  if (ec)
  {
    LOG(WARNING) << "Client TX Transport error: " << ec.message();
  }
  else
  {
    VLOG(6) << "Client response: " << responseCodeString(sipResponse.getResponseCode());
    processIncomingResponse(ec, uiTxId, sipRequest, sipResponse);
  }
}

void TransactionUser::processIncomingResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  VLOG(2) << "Sub-class must implement this";
}

} // rfc3261
} // rtp_plus_plus
