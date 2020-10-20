#include "CorePch.h"
#include <rtp++/rfc3261/ClientTransaction.h>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <rtp++/rfc3261/Rfc3261.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

ClientTransaction::ClientTransaction(boost::asio::io_service& ioService, uint32_t uiTxId, TransportLayer& transportLayer,
  const SipMessage& sipMessage, const std::string& sDestination, const uint16_t uiPort, TransactionProtocol eProtocol,
  ClientTransactionUserNotifier_t responseCallback, ClientTransactionTerminationNotifier_t terminationNotifier,
  boost::system::error_code& ec)
  :m_ioService(ioService),
  m_timerA(ioService, boost::posix_time::milliseconds(INTERVAL_T1_DEFAULT_MS)),
  m_timerB(ioService, boost::posix_time::milliseconds(64*INTERVAL_T1_DEFAULT_MS)),
  m_timerD(ioService),
  m_timerE(ioService, boost::posix_time::milliseconds(INTERVAL_T1_DEFAULT_MS)),
  m_timerF(ioService, boost::posix_time::milliseconds(64 * INTERVAL_T1_DEFAULT_MS)),
  m_timerK(ioService),
  m_uiTxId(uiTxId),
  m_transportLayer(transportLayer),
  m_originalRequest(sipMessage),
  m_sDestination(sDestination),
  m_uiPort(uiPort),
  m_eProtocol(eProtocol),
  m_tuNotifier(responseCallback),
  m_terminationNotifier(terminationNotifier),
  m_eTxState(CTXS_CALLING_OR_TRYING),
  m_timerAIntervalMs(INTERVAL_T1_DEFAULT_MS),
  m_timerDIntervalS(isTransportUnreliable(m_eProtocol) ? INTERVAL_UNRELIABLE_PROTOCOL_TIMEOUT_S : 0),
  m_timerEIntervalMs(INTERVAL_T1_DEFAULT_MS),
  m_bTuHasBeenNotififedOfFinalResponse(false)
{
  m_eTxType = sipMessage.getMethod() == INVITE ? CT_INVITE : CT_NON_INVITE;
  // check if we're dealing with an INVITE tx or a non-INVITE.
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    ec = beginInviteTransaction();
    break;
  }
  case CT_NON_INVITE:
  {
    ec = beginNonInviteTransaction();
    break;
  }
  }
}

ClientTransaction::~ClientTransaction()
{
#ifdef DEBUG_TX
  VLOG(2) << "~ClientTransaction: " << m_uiTxId;
#endif
}

boost::system::error_code ClientTransaction::cancelRequest()
{
  return boost::system::error_code();
}

boost::system::error_code ClientTransaction::beginInviteTransaction()
{
  // send INVITE using transport layer
  boost::system::error_code ec = m_transportLayer.sendSipMessage(m_originalRequest, m_sDestination, m_uiPort, m_eProtocol);
  // if transport is unreliable start timer A.
  if (isTransportUnreliable(m_eProtocol))
    m_timerA.async_wait(boost::bind(&ClientTransaction::handleTimerATimeout, this, boost::asio::placeholders::error));

  // always start timer B.
  m_timerB.async_wait(boost::bind(&ClientTransaction::handleTimerBTimeout, this, boost::asio::placeholders::error));

  return ec;
}

boost::system::error_code ClientTransaction::beginNonInviteTransaction()
{
  // send non-INVITE using transport layer
  boost::system::error_code ec = m_transportLayer.sendSipMessage(m_originalRequest, m_sDestination, m_uiPort, m_eProtocol);
  if (ec)
  {
    // notify TU of timeout: there is no need to invoke the callback as the error code is passed back from the constructor
    return ec;
  }
  // if transport is unreliable start timer A.
  if (isTransportUnreliable(m_eProtocol))
    m_timerE.async_wait(boost::bind(&ClientTransaction::handleTimerETimeout, this, boost::asio::placeholders::error));

  // always start timer F.
  m_timerF.async_wait(boost::bind(&ClientTransaction::handleTimerFTimeout, this, boost::asio::placeholders::error));

  return boost::system::error_code();
}

void ClientTransaction::handleTimerATimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(2) << "ClientTransaction::handleTimerATimeout: " << ec.message();
    m_timerAIntervalMs = m_timerAIntervalMs * 2;
    // time for a retransmission
    boost::system::error_code ec = m_transportLayer.sendSipMessage(m_originalRequest, m_sDestination, m_uiPort, m_eProtocol);
    if (ec)
    {
      updateState(CTXS_TERMINATED);
    }
    else
    {
      // start another timer with twice the duration
      m_timerA.expires_from_now(boost::posix_time::milliseconds(m_timerAIntervalMs));
      m_timerA.async_wait(boost::bind(&ClientTransaction::handleTimerATimeout, this, boost::asio::placeholders::error));
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer A: " << ec.message();
    }
  }
}

void ClientTransaction::handleTimerBTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ClientTransaction::handleTimerBTimeout: " << ec.message();
    // If the client transaction is still in the "Calling" state when timer
    // B fires, the client transaction SHOULD inform the TU that a timeout
    // has occurred.The client transaction MUST NOT generate an ACK.The
    // value of 64 * T1 is equal to the amount of time required to send seven
    // requests in the case of an unreliable transport.
    if (m_eTxState == CTXS_CALLING_OR_TRYING)
    {
      // notify TU of timeout
      m_tuNotifier(boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), m_uiTxId, m_originalRequest, SipMessage());
      // set state to terminated
      updateState(CTXS_TERMINATED);
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer B: " << ec.message();
    }
  }
}

void ClientTransaction::handleTimerDTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ClientTransaction::handleTimerDTimeout: " << ec.message();
    switch (m_eTxState)
    {
    case CTXS_CALLING_OR_TRYING:
    case CTXS_PROCEEDING:
    {
      assert(false);
      break;
    }
    case CTXS_COMPLETED:
    {
      VLOG(5) << "Timer D has fired, client TX terminated";
      updateState(CTXS_TERMINATED);
      break;
    }
    case CTXS_TERMINATED:
    {
      assert(false);
      break;
    }
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer D: " << ec.message();
    }
  }
}

void ClientTransaction::handleTimerETimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ClientTransaction::handleTimerETimeout: " << ec.message();
    if (m_eTxState == CTXS_CALLING_OR_TRYING)
    {
      uint32_t t2 = INTERVAL_T2_DEFAULT_MS;
      m_timerEIntervalMs = std::min(2 * m_timerEIntervalMs, t2);
      // time for a retransmission
      boost::system::error_code ec = m_transportLayer.sendSipMessage(m_originalRequest, m_sDestination, m_uiPort, m_eProtocol);
      if (ec)
      {
        updateState(CTXS_TERMINATED);
      }
      else
      {
        // start another timer with twice the duration
        m_timerE.expires_from_now(boost::posix_time::milliseconds(m_timerEIntervalMs));
        m_timerE.async_wait(boost::bind(&ClientTransaction::handleTimerETimeout, this, boost::asio::placeholders::error));
      }
    }
    else if (m_eTxState == CTXS_PROCEEDING)
    {
      // If Timer E fires while in the "Proceeding" state, the request MUST be
      // passed to the transport layer for retransmission, and Timer E MUST be
      // reset with a value of T2 seconds.
      // time for a retransmission
      boost::system::error_code ec = m_transportLayer.sendSipMessage(m_originalRequest, m_sDestination, m_uiPort, m_eProtocol);
      if (ec)
      {
        updateState(CTXS_TERMINATED);
      }
      else
      {
        m_timerEIntervalMs = INTERVAL_T2_DEFAULT_MS;
        m_timerE.expires_from_now(boost::posix_time::milliseconds(m_timerEIntervalMs));
      }
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer E: " << ec.message();
    }
  }
}

void ClientTransaction::handleTimerFTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ClientTransaction::handleTimerFTimeout: " << ec.message();
    // If Timer F fires while the client transaction is still in the
    // "Trying" state, the client transaction SHOULD inform the TU about the
    // timeout, and then it SHOULD enter the "Terminated" state.If a
    // provisional response is received while in the "Trying" state, the
    // response MUST be passed to the TU, and then the client transaction
    // SHOULD move to the "Proceeding" state.If a final response(status
    // codes 200 - 699) is received while in the "Trying" state, the response
    // MUST be passed to the TU, and the client transaction MUST transition
    // to the "Completed" state.

    // If timer F fires while in the
    // "Proceeding" state, the TU MUST be informed of a timeout, and the
    // client transaction MUST transition to the terminated state.
    if (m_eTxState == CTXS_CALLING_OR_TRYING || m_eTxState == CTXS_PROCEEDING)
    {
      // notify TU of timeout
      m_tuNotifier(boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), m_uiTxId, m_originalRequest, SipMessage());
      // set state to terminated
      updateState(CTXS_TERMINATED);
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer F: " << ec.message();
    }
  }
}

void ClientTransaction::handleTimerKTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ClientTransaction::handleTimerKTimeout: " << ec.message();
    // If Timer K fires while in this state, the client transaction
    // MUST transition to the "Terminated" state.
    assert(m_eTxState == CTXS_COMPLETED);
    // set state to terminated
    updateState(CTXS_TERMINATED);
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer K: " << ec.message();
    }
  }
}

void ClientTransaction::onResponseReceived(const SipMessage& sipMessage)
{
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    onInviteResponseReceived(sipMessage);
    break;
  }
  case CT_NON_INVITE:
  {
    onNonInviteResponseReceived(sipMessage);
    break;
  }
  }
}

void ClientTransaction::onInviteResponseReceived(const SipMessage& sipMessage)
{
  assert(sipMessage.isResponse());

  if (isTransportUnreliable(m_eProtocol))
  {
    // stop retransmissions
    m_timerA.cancel();
  }

  switch (m_eTxState)
  {
  case CTXS_CALLING_OR_TRYING:
  {
    // If the client transaction receives a provisional response while in
    // the "Calling" state, it transitions to the "Proceeding" state.In the
    // "Proceeding" state, the client transaction SHOULD NOT retransmit the
    // request any longer.Furthermore, the provisional response MUST be
    // passed to the TU. Any further provisional responses MUST be passed
    // up to the TU while in the "Proceeding" state.
    if (isResponseProvisional(sipMessage.getResponseCode()))
    {
      updateState(CTXS_PROCEEDING);
      m_timerA.cancel();
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
    }
    // NB!!!: passthrough
  }
  case CTXS_PROCEEDING:
  {
    //  Any further provisional responses MUST be passed
    //  up to the TU while in the "Proceeding" state.
    if (isResponseProvisional(sipMessage.getResponseCode()))
    {
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
    }
    // When in either the "Calling" or "Proceeding" states, reception of a
    // response with status code from 300 - 699 MUST cause the client
    // transaction to transition to "Completed".The client transaction
    // MUST pass the received response up to the TU, and the client
    // transaction MUST generate an ACK request, even if the transport is
    // reliable(guidelines for constructing the ACK from the response are
    // given in Section 17.1.1.3) and then pass the ACK to the transport
    // layer for transmission.The ACK MUST be sent to the same address,
    // port, and transport to which the original request was sent.The
    // client transaction SHOULD start timer D when it enters the
    // "Completed" state, with a value of at least 32 seconds for unreliable
    // transports, and a value of zero seconds for reliable transports.
    // Timer D reflects the amount of time that the server transaction can
    // remain in the "Completed" state when unreliable transports are used.
    // This is equal to Timer H in the INVITE server transaction, whose
    // default is 64 * T1.However, the client transaction does not know the
    // value of T1 in use by the server transaction, so an absolute minimum
    // of 32s is used instead of basing Timer D on T1.
    
    else if (isFinalNonSuccessfulResponse(sipMessage.getResponseCode()))
    {
      updateState(CTXS_COMPLETED);
      m_timerD.expires_from_now(boost::posix_time::seconds(m_timerDIntervalS));
      m_timerD.async_wait(boost::bind(&ClientTransaction::handleTimerDTimeout, this, _1));
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
      m_bTuHasBeenNotififedOfFinalResponse = true;
      sendAck(sipMessage);
    }
    else if (isResponseSuccessfull(sipMessage.getResponseCode()))
    {
      // When in either the "Calling" or "Proceeding" states, reception of a
      // 2xx response MUST cause the client transaction to enter the
      // "Terminated" state, and the response MUST be passed up to the TU.
      // The handling of this response depends on whether the TU is a proxy
      // core or a UAC core.A UAC core will handle generation of the ACK for
      // this response, while a proxy core will always forward the 200 (OK)
      // upstream.The differing treatment of 200 (OK)between proxy and UAC
      // is the reason that handling of it does not take place in the
      // transaction layer.
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
      updateState(CTXS_TERMINATED);
      // Note how the TranaactionLayer is not responsible for sending the ACK in this case!
    }
    break;
  }
  case CTXS_COMPLETED:
  {
    // Any retransmissions of the final response that are received while in
    // the "Completed" state MUST cause the ACK to be re - passed to the
    // transport layer for retransmission, but the newly received response
    // MUST NOT be passed up to the TU.
    if (m_bTuHasBeenNotififedOfFinalResponse)
    {
      // we have likely received a retransmission of the final response code, send another ACK

      sendAck(sipMessage);
    }
    break;
  }
  case CTXS_TERMINATED:
  {
    assert(false);
  }
  }
}

void ClientTransaction::updateState(ClientTransactionState eState)
{
  VLOG(5) << "Updating client tx state to " << toString(eState);
  m_eTxState = eState;
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    if (m_eTxState == CTXS_TERMINATED)
    {
      m_timerA.cancel();
      m_timerB.cancel();
      m_timerD.cancel();
      m_terminationNotifier(m_uiTxId);
    }
    break;
  }
  case CT_NON_INVITE:
  {
    // Once the client transaction enters the "Completed" state, it MUST set
    // Timer K to fire in T4 seconds for unreliable transports, and zero
    // seconds for reliable transports.The "Completed" state exists to
    // buffer any additional response retransmissions that may be received
    // (which is why the client transaction remains there only for
    // unreliable transports).T4 represents the amount of time the network
    // will take to clear messages between client and server transactions.
    // The default value of T4 is 5s.  A response is a retransmission when
    // it matches the same transaction, using the rules specified in Section
    // 17.1.3.If Timer K fires while in this state, the client transaction
    // MUST transition to the "Terminated" state.
    if (m_eTxState == CTXS_COMPLETED)
    {
      if (isTransportUnreliable(m_eProtocol))
      {
        m_timerK.expires_from_now(boost::posix_time::milliseconds(INTERVAL_T4_DEFAULT_MS));
        m_timerK.async_wait(boost::bind(&ClientTransaction::handleTimerKTimeout, this, boost::asio::placeholders::error));
      }
    }
    else if (m_eTxState == CTXS_TERMINATED)
    {
      m_timerE.cancel();
      m_timerF.cancel();
      m_timerK.cancel();
      m_terminationNotifier(m_uiTxId);
    }
    break;
  }
  }
}

void ClientTransaction::onNonInviteResponseReceived(const SipMessage& sipMessage)
{
  assert(sipMessage.isResponse());
  switch (m_eTxState)
  {
  case CTXS_CALLING_OR_TRYING:
  {
    // If a
    // provisional response is received while in the "Trying" state, the
    // response MUST be passed to the TU, and then the client transaction
    // SHOULD move to the "Proceeding" state.If a final response(status
    // codes 200 - 699) is received while in the "Trying" state, the response
    // MUST be passed to the TU, and the client transaction MUST transition
    // to the "Completed" state.
    if (isResponseProvisional(sipMessage.getResponseCode()))
    {
      updateState(CTXS_PROCEEDING);
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
    }
    // NB: pass through!
  }
  case CTXS_PROCEEDING:
  {
    // If a
    // final response(status codes 200 - 699) is received while in the
    // "Proceeding" state, the response MUST be passed to the TU, and the
    // client transaction MUST transition to the "Completed" state.
    if (isResponseFinal(sipMessage.getResponseCode()))
    {
      m_timerE.cancel();
      m_timerF.cancel();
      updateState(CTXS_COMPLETED);
      m_tuNotifier(boost::system::error_code(), m_uiTxId, m_originalRequest, sipMessage);
    }
    break;
  }
  case CTXS_COMPLETED:
  {
    break; 
  }
  case CTXS_TERMINATED:
  {
    assert(false);
  }
  }
}

void ClientTransaction::sendAck(const SipMessage& sipResponse)
{
  // This section specifies the construction of ACK requests sent within
  // the client transaction.A UAC core that generates an ACK for 2xx
  // MUST instead follow the rules described in Section 13.
  //
  // The ACK request constructed by the client transaction MUST contain
  // values for the Call - ID, From, and Request - URI that are equal to the
  // values of those header fields in the request passed to the transport
  // by the client transaction(call this the "original request").
  
  SipMessage ack(ACK, m_originalRequest.getRequestUri());
  ack.copyHeaderField(CALL_ID, m_originalRequest);
  ack.copyHeaderField(FROM, m_originalRequest);
  ack.copyHeaderField(MAX_FORWARDS, m_originalRequest);
  // The To
  // header field in the ACK MUST equal the To header field in the
  // response being acknowledged, and therefore will usually differ from
  // the To header field in the original request by the addition of the
  // tag parameter.The ACK MUST contain a single Via header field, and
  // this MUST be equal to the top Via header field of the original
  // request.The CSeq header field in the ACK MUST contain the same
  // value for the sequence number as was present in the original request,
  // but the method parameter MUST be equal to "ACK".
  ack.copyHeaderField(TO, sipResponse);
  // TODO: double check is this the top-most VIA?
  ack.copyHeaderField(VIA, m_originalRequest);
  ack.setCSeq(m_originalRequest.getRequestSequenceNumber());
  // If the INVITE request whose response is being acknowledged had Route
  // header fields, those header fields MUST appear in the ACK.This is
  // to ensure that the ACK can be routed properly through any downstream
  // stateless proxies.
  
  // TODO

  // Although any request MAY contain a body, a body in an ACK is special
  // since the request cannot be rejected if the body is not understood.
  // Therefore, placement of bodies in ACK for non - 2xx is NOT RECOMMENDED,
  // but if done, the body types are restricted to any that appeared in
  // the INVITE, assuming that the response to the INVITE was not 415.  If
  // it was, the body in the ACK MAY be any type listed in the Accept
  // header field in the 415.
  
  // TODO

  boost::system::error_code ec = m_transportLayer.sendSipMessage(ack, m_sDestination, m_uiPort, m_eProtocol);
  if (ec)
  {
    LOG(WARNING) << "Error sending ACK: " << ec.message();
    updateState(CTXS_TERMINATED);
  }

}

} // rfc3261
} // rtp_plus_plus

