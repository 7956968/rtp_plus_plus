#include "CorePch.h"
#include <rtp++/rfc3261/ServerTransaction.h>
#include <algorithm>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/TransactionUser.h>
#include <rtp++/util/RandomUtil.h>

namespace rtp_plus_plus
{

namespace rfc3261
{

#define INTERVAL_T1_DEFAULT_MS 500
#define INTERVAL_T2_DEFAULT_MS 4000
#define INTERVAL_T4_DEFAULT_MS 5000
#define INTERVAL_UNRELIABLE_PROTOCOL_TIMEOUT_S 32

ServerTransaction::ServerTransaction(boost::asio::io_service& ioService, 
                                     SipContext& localContext,
                                     TransactionUser& transactionUser,
                                     uint32_t uiTxId, TransportLayer& transportLayer,
                                     const SipMessage& sipRequest, const std::string& sSource, const uint16_t uiPort,
                                     TransactionProtocol eProtocol, 
                                     ServerTransactionUserNotifier_t tuNotifierCallback,
                                     ServerTransactionTerminationNotifier_t terminationCallback,
                                     boost::system::error_code& ec)
  :m_eTxType(sipRequest.getMethod() == INVITE ? CT_INVITE : CT_NON_INVITE),
  m_eInviteState(STXS_PROCEEDING),
  m_eNonInviteState(STXS_NI_TRYING),
  m_timerG(ioService),
  m_timerH(ioService),
  m_timerI(ioService),
  m_timerJ(ioService),
  m_uiTxId(uiTxId),
  m_localContext(localContext),
  m_transportLayer(transportLayer),
  m_request(sipRequest),
  m_sSource(sSource),
  m_uiPort(uiPort),
  m_eProtocol(eProtocol),
  m_tuNotifier(tuNotifierCallback),
  m_terminationCallback(terminationCallback),
  m_timerGIntervalMs(INTERVAL_T1_DEFAULT_MS)
{
  VLOG(5) << "Creating server transaction for SIP method " << sipRequest.getMethodString();
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    bool bWillTuGenerateResponseWithin200Ms = false;
    // request must be passed to TU
    SipMessage response = generateResponse(NOT_SET);
    transactionUser.processIncomingRequestFromTransactionLayer(m_uiTxId, sipRequest, bWillTuGenerateResponseWithin200Ms, response);
    // generate TRYING response and send it if TU won't send provisional response in 200ms
    if (!bWillTuGenerateResponseWithin200Ms)
    {
      // TODO: only do this for INVITES!
      // generate provisional response
      response.setResponseCode(TRYING);
      m_mostRecentProvisionalSipResponse = response;
      boost::system::error_code ec = sendProvisionalResponse(m_mostRecentProvisionalSipResponse);
      if (ec)
      {
        // TODO: use other error code
        m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
        LOG(WARNING) << "TODO: Error sending provisional response";
      }
    }
    else
    {
      // This must have been set if bWillTuGenerateResponseWithin200Ms == true
      assert(response.getResponseCode() != NOT_SET);
      // The TU has provided the response code to be sent for the request
      if (isResponseProvisional(response.getResponseCode()))
      {
        // generate provisional response
        m_mostRecentProvisionalSipResponse = response;
        boost::system::error_code ec = sendProvisionalResponse(m_mostRecentProvisionalSipResponse);
        if (ec)
        {
          // TODO: use other error code
          m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
          LOG(WARNING) << "TODO: Error sending provisional response";
        }
      }
      else
      {
        // generate provisional response
        m_finalSipResponse = response;
        boost::system::error_code ec = sendResponse(m_finalSipResponse);
        if (ec)
        {
          // TODO: use other error code
          m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
          LOG(WARNING) << "TODO: Error sending final response";
        }
      }
    }
    break;
  }
  case CT_NON_INVITE:
  {
    SipMessage response = generateResponse(NOT_SET);
    bool bDummy;
    transactionUser.processIncomingRequestFromTransactionLayer(m_uiTxId, sipRequest, bDummy, response);
    // The TU has provided the response code to be sent for the request
    if (isResponseProvisional(response.getResponseCode()))
    {
      // generate provisional response
      m_mostRecentProvisionalSipResponse = response;
      boost::system::error_code ec = sendProvisionalResponse(m_mostRecentProvisionalSipResponse);
      if (ec)
      {
        // TODO: use other error code
        m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
        LOG(WARNING) << "TODO: Error sending provisional response";
      }
    }
    else
    {
      // In the case of the ACK message, the response code will not be set. No response should be set?
      if (response.getResponseCode() > NOT_SET)
      {
        VLOG(5) << "Sending response to non-INVITE request: " << responseCodeString(response.getResponseCode());
        // generate provisional response
        m_finalSipResponse = response;
        boost::system::error_code ec = sendResponse(m_finalSipResponse);
        if (ec)
        {
          // TODO: use other error code
          m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
          LOG(WARNING) << "TODO: Error sending final response";
        }
      }
      else
      {
        // NB: in this case we send no response!
        VLOG(6) << "In case of ACK no response code is set";
      }
    }
    break;
  }
  }

}

ServerTransaction::~ServerTransaction()
{
#ifdef DEBUG_TX
  VLOG(2) << "~ServerTransaction: " << m_uiTxId;
#endif
}

void ServerTransaction::onRequestReceived(const SipMessage& sipRequest)
{
  VLOG(2) << "ServerTransaction::onRequestReceived: Method: " << sipRequest.getMethod() << " " << sipRequest.getMethodString();
  if (sipRequest.getMethod() == ACK)
  {
    onAckReceived(sipRequest);
  }
  // TODO: how to check for RTX
  VLOG(2) << "TODO: check for RTX";
}

void ServerTransaction::onRequestRetransmissionReceived(const SipMessage& sipRequest)
{
#ifdef DEBUG_TX
  VLOG(2) << "ServerTransaction::onRequestRetransmissionReceived";
#endif
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    // INVITE transactions
    switch (m_eInviteState)
    {
    case STXS_PROCEEDING:
    {
      // If a request
      // retransmission is received while in the "Proceeding" state, the most
      // recent provisional response that was received from the TU MUST be
      // passed to the transport layer for retransmission.
      sendProvisionalResponse(m_mostRecentProvisionalSipResponse);
      break;
    }
    case STXS_COMPLETED:
    {
      // Furthermore,
      // while in the "Completed" state, if a request retransmission is
      // received, the server SHOULD pass the response to the transport for
      // retransmission.
      sendResponse(m_finalSipResponse);
      break;
    }
    case STXS_CONFIRMED:
    {
      VLOG(6) << "TODO: request rtx received in confirmed state";
      break;
    }
    case STXS_TERMINATED:
    {
      assert(false);
    }
    }

    break;
  }
  case CT_NON_INVITE:
  {
    // NON-INVITE transactions
    switch (m_eNonInviteState)
    {
    case STXS_NI_TRYING:
    {
      // Once in the "Trying" state, any further request
      // retransmissions are discarded.A request is a retransmission if it
      // matches the same server transaction, using the rules specified in
      // Section 17.2.3.
      break;
    }
    case STXS_NI_PROCEEDING:
    {
      // If a retransmission of the
      // request is received while in the "Proceeding" state, the most
      // recently sent provisional response MUST be passed to the transport
      // layer for retransmission.
      sendProvisionalResponse(m_mostRecentProvisionalSipResponse);
      break;
    }
    case STXS_NI_COMPLETED:
    {
      // While in the "Completed" state, the
      // server transaction MUST pass the final response to the transport
      // layer for retransmission whenever a retransmission of the request is
      // received.
      sendResponse(m_finalSipResponse);
      break;
    }
    case STXS_NI_TERMINATED:
    {
      assert(false);
    }
    }
    break;
  }
  }
}

#if 0
SipMessage ServerTransaction::generateResponse(ResponseCode eCode, const SipMessage& sipRequest)
{
  VLOG(2) << "ServerTransaction::generateResponse";

  // 8.2.6 Generating the Response
  SipMessage response(eCode);

  // 8.2.6.1 Sending a Provisional Response
  if (isResponseProvisional(eCode))
  {
    //  One largely non - method - specific guideline for the generation of
    //  responses is that UASs SHOULD NOT issue a provisional response for a
    //  non - INVITE request.Rather, UASs SHOULD generate a final response to
    //  a non - INVITE request as soon as possible.
    //  When a 100 (Trying)response is generated, any Timestamp header field
    //  present in the request MUST be copied into this 100 (Trying)
    //  response.If there is a delay in generating the response, the UAS
    //  SHOULD add a delay value into the Timestamp value in the response.
    //  This value MUST contain the difference between the time of sending of
    //  the response and receipt of the request, measured in seconds.
  }

  // 8.2.6.2 Headers and Tags
  //  The From field of the response MUST equal the From header field of
  //  the request.The Call - ID header field of the response MUST equal the
  //  Call - ID header field of the request.The CSeq header field of the
  //  response MUST equal the CSeq field of the request.The Via header
  //  field values in the response MUST equal the Via header field values
  //  in the request and MUST maintain the same ordering.
  response.addHeaderField(FROM, *sipRequest.getHeaderField(FROM));
  response.addHeaderField(CALL_ID, *sipRequest.getHeaderField(CALL_ID));
  response.addHeaderField(CSEQ, *sipRequest.getHeaderField(CSEQ));
  std::vector<std::string> vVia = sipRequest.getHeaderFields(VIA);
  for (auto& via : vVia)
    response.addHeaderField(VIA, via);

  //  If a request contained a To tag in the request, the To header field
  //  in the response MUST equal that of the request.However, if the To
  //  header field in the request did not contain a tag, the URI in the To
  //  header field in the response MUST equal the URI in the To header
  //  field; additionally, the UAS MUST add a tag to the To header field in
  //  the response(with the exception of the 100 (Trying)response, in
  //  which a tag MAY be present).This serves to identify the UAS that is
  //  responding, possibly resulting in a component of a dialog ID.The
  //  same tag MUST be used for all responses to that request, both final
  //  and provisional(again excepting the 100 (Trying)).Procedures for
  //  the generation of tags are defined in Section 19.3.
  boost::optional<std::string> to = sipRequest.getHeaderField(TO);
  if (boost::algorithm::contains(*to, "tag="))
  {
    VLOG(2) << "To tag already exists " << *to;
    response.addHeaderField(TO, *sipRequest.getHeaderField(TO));
  }
  else
  {
    std::ostringstream toTag;
    toTag << *to << ";tag=" << random_string(4);
    VLOG(2) << "Generated To tag: " << toTag.str();
    response.addHeaderField(TO, toTag.str());
  }
  return response;
}
#endif

SipMessage ServerTransaction::generateResponse(ResponseCode eCode)
{
  VLOG(5) << "ServerTransaction::generateResponse: " << eCode << " " << responseCodeString(eCode);

  SipMessage& sipRequest = m_request;

  boost::optional<SipMessage> response = SipMessage::createResponse(sipRequest);
  assert(response);

  // 8.2.6 Generating the Response
  response->setResponseCode(eCode);

  // 8.2.6.1 Sending a Provisional Response
  if (isResponseProvisional(eCode))
  {
    //  One largely non - method - specific guideline for the generation of
    //  responses is that UASs SHOULD NOT issue a provisional response for a
    //  non - INVITE request.Rather, UASs SHOULD generate a final response to
    //  a non - INVITE request as soon as possible.
    //  When a 100 (Trying)response is generated, any Timestamp header field
    //  present in the request MUST be copied into this 100 (Trying)
    //  response.If there is a delay in generating the response, the UAS
    //  SHOULD add a delay value into the Timestamp value in the response.
    //  This value MUST contain the difference between the time of sending of
    //  the response and receipt of the request, measured in seconds.
  }

  //  If a request contained a To tag in the request, the To header field
  //  in the response MUST equal that of the request.However, if the To
  //  header field in the request did not contain a tag, the URI in the To
  //  header field in the response MUST equal the URI in the To header
  //  field; additionally, the UAS MUST add a tag to the To header field in
  //  the response(with the exception of the 100 (Trying)response, in
  //  which a tag MAY be present).This serves to identify the UAS that is
  //  responding, possibly resulting in a component of a dialog ID.The
  //  same tag MUST be used for all responses to that request, both final
  //  and provisional(again excepting the 100 (Trying)).Procedures for
  //  the generation of tags are defined in Section 19.3.

  if (!sipRequest.hasAttribute(TO, TAG))
  {
    boost::optional<std::string> to = *sipRequest.getHeaderField(TO);
    if (!m_toTag)
    {
      m_toTag = boost::optional<std::string>(random_string(4));
      VLOG(6) << "Generated To tag: " << *m_toTag;
    }
    std::ostringstream toTag;
    toTag << *to << ";tag=" << *m_toTag;
    response->addHeaderField(TO, toTag.str());
  }
  else
  {
    boost::optional<std::string> to = sipRequest.getHeaderField(TO);
    assert(to);
    VLOG(6) << "To tag already exists " << *to;
    response->addHeaderField(TO, *to);
  }

  // Add contact header
  response->addHeaderField(CONTACT, m_localContext.getContact());
  return *response;
}

void ServerTransaction::onAckReceived(const SipMessage& sipRequest)
{
#ifdef DEBUG_TX
  VLOG(2) << "ServerTransaction::onAckReceived";
#endif
  // If an ACK is received while the server transaction is in the
  // "Completed" state, the server transaction MUST transition to the
  // "Confirmed" state.As Timer G is ignored in this state, any
  // retransmissions of the response will cease.
  assert(m_eTxType == CT_INVITE);
  switch (m_eInviteState)
  {
  case STXS_PROCEEDING:
  {
    assert(false);
  }
  case STXS_COMPLETED:
  {
    updateInviteState(STXS_CONFIRMED);
    break;
  }
  case STXS_CONFIRMED:
  case STXS_TERMINATED:
  {
    assert(false);
  }
  }
}

boost::system::error_code ServerTransaction::sendResponse(const SipMessage& sipResponse)
{
#ifdef DEBUG_TX
  VLOG(2) << "ServerTransaction::sendResponse";
#endif
  if (isResponseProvisional(sipResponse.getResponseCode()))
  {
    return sendProvisionalResponse(sipResponse);
  }

  // send final response 
  switch (m_eTxType)
  {
  case CT_INVITE:
  {
#ifdef DEBUG_TX
    VLOG(2) << "ServerTransaction::sendResponse INVITE TX";
#endif
    // INVITE
    switch (m_eInviteState)
    {
    case STXS_PROCEEDING:
    {
      if (isResponseSuccessfull(sipResponse.getResponseCode()))
      {
        // If, while in the "Proceeding" state, the TU passes a 2xx response to
        // the server transaction, the server transaction MUST pass this
        // response to the transport layer for transmission.It is not
        // retransmitted by the server transaction; retransmissions of 2xx
        // responses are handled by the TU.The server transaction MUST then
        // transition to the "Terminated" state.
        m_transportLayer.sendSipMessage(sipResponse, m_sSource, m_uiPort, m_eProtocol);
        m_finalSipResponse = sipResponse;
        updateInviteState(STXS_TERMINATED);
      }
      else if (isFinalNonSuccessfulResponse(sipResponse.getResponseCode()))
      {
        // While in the "Proceeding" state, if the TU passes a response with
        // status code from 300 to 699 to the server transaction, the response
        // MUST be passed to the transport layer for transmission, and the state
        // machine MUST enter the "Completed" state.For unreliable transports,
        // timer G is set to fire in T1 seconds, and is not set to fire for
        // reliable transports.
        m_transportLayer.sendSipMessage(sipResponse, m_sSource, m_uiPort, m_eProtocol);
        m_finalSipResponse = sipResponse;
        updateInviteState(STXS_COMPLETED);
        if (isTransportUnreliable(m_eProtocol))
        {
          m_timerG.expires_from_now(boost::posix_time::milliseconds(INTERVAL_T1_DEFAULT_MS));
          m_timerG.async_wait(boost::bind(&ServerTransaction::handleTimerGTimeout, this, boost::asio::placeholders::error));
        }
      }
      break;
    }
    case STXS_COMPLETED:
    case STXS_CONFIRMED:
    case STXS_TERMINATED:
    {
      VLOG(6) << "TODO: sendResponse while completed";
      assert(false);
    }
    }
    break;
  }
  case CT_NON_INVITE:
  {
#ifdef DEBUG_TX
    VLOG(2) << "ServerTransaction::sendResponse non-INVITE TX";
#endif
    // NON-INVITE
    switch (m_eNonInviteState)
    {
    case STXS_NI_TRYING:
    {
      //  While in the "Trying" state, if the TU passes a provisional response
      //  to the server transaction, the server transaction MUST enter the
      //  "Proceeding" state.The response MUST be passed to the transport
      //  layer for transmission.
      if (isResponseProvisional(sipResponse.getResponseCode()))
      {
        m_mostRecentProvisionalSipResponse = sipResponse;
        m_transportLayer.sendSipMessage(sipResponse, m_sSource, m_uiPort, m_eProtocol);
        updateNonInviteState(STXS_NI_PROCEEDING);
        break;
      }
      // fall-through to handle final responses
    }
    case STXS_NI_PROCEEDING:
    {
      //  Any further provisional responses that are
      //  received from the TU while in the "Proceeding" state MUST be passed
      //  to the transport layer for transmission.
      if (isResponseProvisional(sipResponse.getResponseCode()))
      {
        m_mostRecentProvisionalSipResponse = sipResponse;
        m_transportLayer.sendSipMessage(sipResponse, m_sSource, m_uiPort, m_eProtocol);
      }
      // If the TU passes a final response(status
      // codes 200 - 699) to the server while in the "Proceeding" state, the
      // transaction MUST enter the "Completed" state, and the response MUST
      // be passed to the transport layer for transmission.
      if (isResponseFinal(sipResponse.getResponseCode()))
      {
        m_transportLayer.sendSipMessage(sipResponse, m_sSource, m_uiPort, m_eProtocol);
        m_finalSipResponse = sipResponse;
        updateNonInviteState(STXS_NI_COMPLETED);
      }
      break;
    }
    case STXS_NI_COMPLETED:
    {
      // Any other final responses passed by the TU to the server
      // transaction MUST be discarded while in the "Completed" state.
      break;
    }
    case STXS_NI_TERMINATED:
    {
      VLOG(6) << "TODO";
      assert(false);
    }
    }
    break;
  }
  }

  return boost::system::error_code();
}

boost::system::error_code ServerTransaction::sendProvisionalResponse(const SipMessage& sipProvisionalResponse)
{
  boost::system::error_code ec;

  switch (m_eTxType)
  {
  case CT_INVITE:
  {
    // INVITE
    switch (m_eInviteState)
    {
      // The TU passes any number of provisional responses to the server
      // transaction.So long as the server transaction is in the
      // "Proceeding" state, each of these MUST be passed to the transport
      // layer for transmission.They are not sent reliably by the
      // transaction layer(they are not retransmitted by it) and do not cause
      // a change in the state of the server transaction.If a request
      // retransmission is received while in the "Proceeding" state, the most
      // recent provisional response that was received from the TU MUST be
      // passed to the transport layer for retransmission.A request is a
      // retransmission if it matches the same server transaction based on the
      // rules of Section 17.2.3.
    case STXS_PROCEEDING:
    {
      ec = m_transportLayer.sendSipMessage(sipProvisionalResponse, m_sSource, m_uiPort, m_eProtocol);
      break;
    }
    case STXS_COMPLETED:
    case STXS_CONFIRMED:
    case STXS_TERMINATED:
    {
      assert(false);
    }
    }
    break;
  }
  case CT_NON_INVITE:
  {
    // NON-INVITE
    switch (m_eNonInviteState)
    {
      // While in the "Trying" state, if the TU passes a provisional response
      // to the server transaction, the server transaction MUST enter the
      // "Proceeding" state.The response MUST be passed to the transport
      // layer for transmission.Any further provisional responses that are
      // received from the TU while in the "Proceeding" state MUST be passed
      // to the transport layer for transmission.
    case STXS_NI_TRYING:
    {
      ec = m_transportLayer.sendSipMessage(sipProvisionalResponse, m_sSource, m_uiPort, m_eProtocol);
      updateNonInviteState(STXS_NI_PROCEEDING);
      break;
    }
    case STXS_NI_PROCEEDING:
    {
      ec = m_transportLayer.sendSipMessage(sipProvisionalResponse, m_sSource, m_uiPort, m_eProtocol);
      break;
    }
    case STXS_NI_COMPLETED:
    case STXS_NI_TERMINATED:
    {
      assert(false);
    }
    }
    break;
  }
  }

  m_mostRecentProvisionalSipResponse = sipProvisionalResponse;
  return ec;
}

void ServerTransaction::updateInviteState(ServerInviteTransactionState eState)
{
  VLOG(5) << "INVITE state update [" << toString(eState) << "]";

  m_eInviteState = eState;
  switch (eState)
  {
  case STXS_PROCEEDING:
  {
    assert(false);
  }
  case STXS_CONFIRMED:
  {
    // we can cancel timer H now as we have received the ACK
    m_timerH.cancel();
    // The purpose of the "Confirmed" state is to absorb any additional ACK
    // messages that arrive, triggered from retransmissions of the final
    // response.When this state is entered, timer I is set to fire in T4
    // seconds for unreliable transports, and zero seconds for reliable
    // transports.
    uint32_t timerIIntervalMs = (isTransportUnreliable(m_eProtocol)) ? INTERVAL_T4_DEFAULT_MS : 0;
    m_timerI.expires_from_now(boost::posix_time::milliseconds(timerIIntervalMs));
    m_timerI.async_wait(boost::bind(&ServerTransaction::handleTimerITimeout, this, boost::asio::placeholders::error));
    break;
  }
  case STXS_COMPLETED:
  {
    // When the "Completed" state is entered, timer H MUST be set to fire in
    // 64 * T1 seconds for all transports.Timer H determines when the server
    // transaction abandons retransmitting the response.Its value is
    // chosen to equal Timer B, the amount of time a client transaction will
    // continue to retry sending a request.  
    m_timerH.expires_from_now(boost::posix_time::milliseconds(64 * INTERVAL_T1_DEFAULT_MS));
    m_timerH.async_wait(boost::bind(&ServerTransaction::handleTimerHTimeout, this, boost::asio::placeholders::error));

    break;
  }
  case STXS_TERMINATED:
  {
    // Once the transaction is in the "Terminated" state, it MUST be
    // destroyed immediately. As with client transactions, this is needed
    // to ensure reliability of the 2xx responses to INVITE.

    // cancel timers as the transaction will be destroyed
    m_timerG.cancel();
    m_timerH.cancel();
    m_timerI.cancel();
    m_terminationCallback(m_uiTxId);
    break;
  }
  }
}

void ServerTransaction::updateNonInviteState(ServerNonInviteTransactionState eState)
{
  VLOG(5) << "NON-INVITE state update [" << toString(eState) << "]";

  m_eNonInviteState = eState;
  switch (eState)
  {
  case STXS_NI_TRYING:
  {
    assert(false);
  }
  case STXS_NI_PROCEEDING:
  {
    break;
  }
  case STXS_NI_COMPLETED:
  {
    // When the server transaction enters the "Completed" state, it MUST set
    // Timer J to fire in 64 * T1 seconds for unreliable transports, and zero
    // seconds for reliable transports.
    uint32_t timerJIntervalMs = (isTransportUnreliable(m_eProtocol)) ? 64 * INTERVAL_T1_DEFAULT_MS : 0;
    m_timerJ.expires_from_now(boost::posix_time::milliseconds(timerJIntervalMs));
    m_timerJ.async_wait(boost::bind(&ServerTransaction::handleTimerJTimeout, this, boost::asio::placeholders::error));
    break;
  }
  case STXS_NI_TERMINATED:
  {
    m_timerJ.cancel();
    // The server transaction MUST be destroyed the instant it enters the
    // "Terminated" state.    
    m_terminationCallback(m_uiTxId);
    break;
  }
  }
}

void ServerTransaction::handleTimerGTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ServerTransaction::handleTimerGTimeout";
    // If timer G fires, the response
    // is passed to the transport layer once more for retransmission, and
    // timer G is set to fire in MIN(2 * T1, T2) seconds.From then on, when
    // timer G fires, the response is passed to the transport again for
    // transmission, and timer G is reset with a value that doubles, unless
    // that value exceeds T2, in which case it is reset with the value of
    // T2.This is identical to the retransmit behavior for requests in the
    // "Trying" state of the non - INVITE client transaction.
    switch (m_eInviteState)
    {
    case STXS_COMPLETED:
    {
      boost::system::error_code ec = m_transportLayer.sendSipMessage(m_finalSipResponse, m_sSource, m_uiPort, m_eProtocol);
      if (ec)
      {
        // TODO: use other error code for transport errors
        m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
      }
      uint32_t t1 = INTERVAL_T1_DEFAULT_MS;
      m_timerGIntervalMs = std::min(2 * m_timerGIntervalMs, t1);
      m_timerG.expires_from_now(boost::posix_time::milliseconds(m_timerGIntervalMs));
      m_timerG.async_wait(boost::bind(&ServerTransaction::handleTimerGTimeout, this, boost::asio::placeholders::error));
      break;
    }
    case STXS_CONFIRMED:
    {
      // If an ACK is received while the server transaction is in the
      // "Completed" state, the server transaction MUST transition to the
      // "Confirmed" state.As Timer G is ignored in this state, any
      // retransmissions of the response will cease.
      break;
    }
    case STXS_PROCEEDING:
    case STXS_TERMINATED:
    {
      assert(false);
    }
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer G: " << ec.message();
    }
  }
}

void ServerTransaction::handleTimerHTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ServerTransaction::handleTimerHTimeout";
    assert(m_eTxType == CT_INVITE);
    // If timer H fires while in the "Completed" state, it implies that the
    // ACK was never received.In this case, the server transaction MUST
    // transition to the "Terminated" state, and MUST indicate to the TU
    // that a transaction failure has occurred.
    switch (m_eInviteState)
    {
    case STXS_PROCEEDING:
    {
      assert(false);
    }
    case STXS_COMPLETED:
    {
      // notify of TX failure: must occur before state update as termination removes transaction
      m_tuNotifier(m_uiTxId, boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category()), SipMessage());
      updateInviteState(STXS_TERMINATED);
      break;
    }
    case STXS_CONFIRMED:
    case STXS_TERMINATED:
    {
      break;
    }
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer H: " << ec.message();
    }
  }
}

void ServerTransaction::handleTimerITimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ServerTransaction::handleTimerITimeout";
    // Once timer I fires, the server MUST transition to the
    // "Terminated" state.
    updateInviteState(STXS_TERMINATED);
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer I: " << ec.message();
    }
  }
}

void ServerTransaction::handleTimerJTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    VLOG(5) << "ServerTransaction::handleTimerJTimeout";
    // The server transaction remains in this state until Timer J fires, at
    // which point it MUST transition to the "Terminated" state.
    updateInviteState(STXS_TERMINATED);
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      VLOG(5) << "Error in timer J: " << ec.message();
    }
  }
}

} // rfc3261
} // rtp_plus_plus
