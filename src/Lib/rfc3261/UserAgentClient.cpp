#include "CorePch.h"
#include <rtp++/rfc3261/UserAgentClient.h>
#include <boost/bind.hpp>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/util/RandomUtil.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

UserAgentClient::UserAgentClient(boost::asio::io_service& ioService, const SipContext& sipContext, DialogManager& dialogManager, 
  TransportLayer& transportLayer, boost::system::error_code& ec, TransactionProtocol ePreferredProtocol)
  :TransactionUser(ioService, sipContext, transportLayer),
  m_bShuttingDown(false),
  m_ioService(ioService),
  m_ePreferredProtocol(ePreferredProtocol),
  m_dialogManager(dialogManager)
{
  VLOG(6) << "TODO: need to add stop method in which timers are cancelled";
}

boost::system::error_code UserAgentClient::shutdown(CompletionHandler_t handler)
{
  m_bShuttingDown = true;
  m_onCompletion = handler;
  VLOG(7) << "TODO: keep track of outstanding transactions and call completion handler once all outstanding transactions have completed";
  return boost::system::error_code();
}

boost::system::error_code UserAgentClient::registerWithServer(const SipUri& sipServer, const std::string& to, const std::string& from, uint32_t uiExpiresSeconds, uint32_t& uiTxId)
{
  // Call-ID
  std::string sCallId = generateCallId(m_sipContext.Domain);
  // Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds
  std::ostringstream via;
  // std::string sentBy(pLocalSipUri->getHost());
  // Adding rport
  via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";rport=<<RPORT>>" << ";branch=z9hG4bK" << random_string(11);
  // via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";branch=z9hG4bK" << random_string(11); // TODO?
  // <<PROTO>> and <<FQDN>> are set by transport layer
  auto sipUriString = sipServer.toString();
  assert(sipUriString);
  auto sSipUriString = *sipUriString;
  SipMessage sipRegister = generateSipRequest(methodToString(REGISTER), sSipUriString, to, from, sCallId, via.str());

  sipRegister.setHeaderField(EXPIRES, ::toString(uiExpiresSeconds));
  // begin transaction
  return beginTransaction(uiTxId, sipRegister, sipServer.getHost(), sipServer.getPort(), m_ePreferredProtocol);
}

boost::system::error_code UserAgentClient::invite(const std::string& to, const std::string& from, const std::string& sDomainOrHost,
                                                  uint32_t& uiTxId, const MessageBody& sessionDescription, const std::string& sProxy)
{
  // since the to may contain a display name we need to extract the request URI first.
  std::string sDisplayName;
  std::string sRequestUri;
  size_t posStart = to.find("<");
  if (posStart != std::string::npos)
  {
    size_t posEnd = to.find(">");
    assert(posEnd != std::string::npos);
    sRequestUri = to.substr(posStart + 1, posEnd - posStart - 1);
    sDisplayName = to.substr(0, posStart);
  }
  else
  {
    sRequestUri = to;
  }

  // Call-ID
  std::string sCallId = generateCallId(sDomainOrHost);
  // Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds
  std::ostringstream via;
  // Adding rport
  via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";rport=<<RPORT>>" << ";branch=z9hG4bK" << random_string(11);
  // via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";branch=z9hG4bK" << random_string(11); // TODO?
  // <<PROTO>> and <<FQDN>> are set by transport layer
  SipMessage sipInvite = generateSipRequest(methodToString(INVITE), sRequestUri, to, from, sCallId, via.str());

  boost::optional<SipUri> pSipUri;
  if (sProxy == "")
  {
    pSipUri = SipUri::parse(sipInvite.getToUri());
    if (!pSipUri)
    {
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::generic_category());
    }
  }
  else
  {
    pSipUri = SipUri::parse(sProxy);
    if (!pSipUri)
    {
      VLOG(2) << "Invalid SIP proxy specified: " << sProxy;
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::generic_category());
    }
  }
  // Session Description
  sipInvite.setMessageBody(sessionDescription);
  // begin transaction
  return beginTransaction(uiTxId, sipInvite, pSipUri->getHost(), pSipUri->getPort(), m_ePreferredProtocol);
}

SipMessage UserAgentClient::generateSipRequest(const std::string& sMethod, const std::string& sRequestUri, const std::string& to, const std::string& from,
                                               const std::string& sCallId, const std::string& sVia, uint32_t uiMaxForwards)
{
  SipMessage sipMessage(sMethod, sRequestUri);
  // Request - URI: The initial Request-URI of the message SHOULD be set to the value of
  // the URI in the To field.
  sipMessage.addHeaderField(TO, to);
  // To: Carol <sip:carol@chicago.com>
  // From: "Bob" <sips:bob@biloxi.com> ;tag=a48s
  std::ostringstream ostrFrom;
  ostrFrom << from << ";tag=";
    ostrFrom << generateFromTag();
  sipMessage.addHeaderField(FROM, ostrFrom.str());
  // Call-ID: f81d4fae-7dec-11d0-a765-00a0c91e6bf6@foo.bar.com
  sipMessage.addHeaderField(CALL_ID, sCallId);
  // CSeq: 4711 INVITE
  uint32_t uiSN = 1; // should this be random for the initial request?!?
  sipMessage.addHeaderField(CSEQ, ::toString(uiSN) + " " + sMethod);
  // Max-Forwards: 70
  sipMessage.addHeaderField(MAX_FORWARDS, ::toString(uiMaxForwards));
  // Contact: 
  sipMessage.addHeaderField(CONTACT, m_sipContext.getContact()); // TODO: verify that this is ok?!?
  // Via:
  sipMessage.addHeaderField(VIA, sVia);
  return sipMessage;
}

std::string UserAgentClient::generateCallId(const std::string& sDomain)
{
  VLOG(6) << " TODO: could really improve this e.g. use GUID?";
  if (sDomain.empty())
  {
    return random_string(16);
  }
  else
  {
    return random_string(16) + "@" + sDomain;
  }
}

std::string UserAgentClient::generateFromTag()
{
  return random_string(4);
}

void UserAgentClient::handleIncomingResponseFromTransportLayer(const SipMessage& sipResponse, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  // match response to request sent by UAC
  // When the transport layer in the client receives a response, it has to
  // determine which client transaction will handle the response, so that
  // the processing of Sections 17.1.1 and 17.1.2 can take place.The
  // branch parameter in the top Via header field is used for this
  // purpose.A response matches a client transaction under two
  // conditions :

  // 1.  If the response has the same value of the branch parameter in
  //     the top Via header field as the branch parameter in the top
  //     Via header field of the request that created the transaction.

  // 2.  If the method parameter in the CSeq header field matches the
  //     method of the request that created the transaction.The
  //    method is needed since a CANCEL request constitutes a
  //    different transaction, but shares the same value of the branch
  //    parameter.

  // find matching request
  VLOG(6) << "Matching response to request";
  // get branch parameter of top-most via
  boost::optional<std::string> branch = sipResponse.getBranchParameter();

  if (branch)
  {
    for (auto& pair : m_mClientTransactions)
    {
      boost::optional<std::string> requestBranch = pair.second->getRequest().getBranchParameter();
      VLOG(6) << "Request branch: " << *requestBranch << " response branch: " << *branch;
      if (*requestBranch == *branch)
      {
        // found matching branch parameter, now check method
        boost::optional<std::string> cseq = sipResponse.getHeaderField(CSEQ);
        boost::optional<std::string> requestCseq = pair.second->getRequest().getHeaderField(CSEQ);
        assert(cseq && requestCseq);
        if (*cseq == *requestCseq)
        {
          // found match: let transaction handle response
          pair.second->onResponseReceived(sipResponse);
          return;
        }
      }
    }
  }
  else
  {
    VLOG(6) << "No branch parameter";
  }

  if (isResponseSuccessfull(sipResponse.getResponseCode()))
  {
    // This could be the case for INVITEs where the 200 OK has already been processed, but the ACK was lost.
    // In this case the UAS will retransmit the 200 OK until it receives an ACK.
    // For this purpose we store previously received 200 OKs temporarily.
    auto it = std::find_if(m_vInviteResponsesFromUas.begin(), m_vInviteResponsesFromUas.end(), [sipResponse](std::pair<SipMessage, SipMessage>& pair)
    {
      // TODO: does this suffice to match retransmissions?
      return (sipResponse.getHeaderField(FROM) == pair.first.getHeaderField(FROM) &&
        sipResponse.getHeaderField(TO) == pair.first.getHeaderField(TO) &&
        sipResponse.getHeaderField(CALL_ID) == pair.first.getHeaderField(CALL_ID) &&
        sipResponse.getHeaderField(VIA) == pair.first.getHeaderField(VIA)
        );
    });
    if (it != m_vInviteResponsesFromUas.end())
    {
      VLOG(2) << "Retransmission of previous 200 OK - retransmitting ACK to Contact: " << sipResponse.getContact();
      boost::optional<SipUri> pContact = SipUri::parse(sipResponse.getContactUri());
      assert(pContact);
      boost::system::error_code ec = m_transportLayer.sendSipMessage(it->second, pContact->getHost(), pContact->getPort(), m_ePreferredProtocol);
      if (ec)
      {
        LOG(WARNING) << "Error sending ACK: " << ec.message();
      }
      else
      {
        VLOG(2) << "Transport layer sent ack";
      }
    }
    else
    {
      LOG(WARNING) << "Unable to match 200 OK response: " << sipResponse.toString();
    }
  }
  else
  {
    // TODO: what to do with unmatched responses?
    LOG(WARNING) << "Unable to match response: " << sipResponse.toString();
  }
}

void UserAgentClient::processIncomingResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  switch (sipResponse.getMethod())
  {
  case REGISTER:
  {
    handleRegisterResponse(ec, uiTxId, sipRequest, sipResponse);
    break;
  }
  case INVITE:
  {
    handleInviteResponse(ec, uiTxId, sipRequest, sipResponse);
    break;
  }
  case BYE:
  {
    handleByeResponse(ec, uiTxId, sipRequest, sipResponse);
    break;
  }
  default:
  {
    VLOG(2) << "Unhandled method: " << sipResponse.getMethodString();
    break;
  }
  }
}

void UserAgentClient::handleRegisterResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  if (isResponseProvisional(sipResponse.getResponseCode()))
  {
    VLOG(2) << "REGISTERING: " << sipResponse.getResponseCode() << " " << responseCodeString(sipResponse.getResponseCode());
  }
  else if (isResponseSuccessfull(sipResponse.getResponseCode()))
  {
    LOG(WARNING) << "REGISTER successful: " << sipResponse.getResponseCode() << " " << responseCodeString(sipResponse.getResponseCode());
  }
  else // if (isFinalNonSuccessfulResponse(sipResponse.getResponseCode()))
  {
    LOG(WARNING) << "Failed to REGISTER: " << sipResponse.getResponseCode() << " " << responseCodeString(sipResponse.getResponseCode());
  }

  if (m_onRegisterResponse)
    m_onRegisterResponse(ec, uiTxId, sipResponse);
  else
  {
    LOG(WARNING) << "No REGISTER response handler configured";
  }
}

void UserAgentClient::handleInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  if (isResponseProvisional(sipResponse.getResponseCode()))
  {
    //  Within this specification, only
    //  2xx and 101 - 199 responses with a To tag, where the request was
    //  INVITE, will establish a dialog.
    if (sipResponse.getResponseCode() > TRYING)
    {
      //  Zero, one or multiple provisional responses may arrive before one or
      //  more final responses are received.Provisional responses for an
      //  INVITE request can create "early dialogs".If a provisional response
      //  has a tag in the To field, and if the dialog ID of the response does
      //  not match an existing dialog, one is constructed using the procedures
      //  defined in Section 12.1.2.

      //  The early dialog will only be needed if the UAC needs to send a
      //  request to its peer within the dialog before the initial INVITE
      //  transaction completes.Header fields present in a provisional
      //  response are applicable as long as the dialog is in the early state
      //  (for example, an Allow header field in a provisional response
      //  contains the methods that can be used in the dialog while this is in
      //  the early state).
      // create early dialog
      VLOG(5) << "Creating early dialog";
      m_dialogManager.createDialog(uiTxId, sipRequest, sipResponse, true, true);
    }
  }
  else if (isResponseSuccessfull(sipResponse.getResponseCode()))
  {
    //  Multiple 2xx responses may arrive at the UAC for a single INVITE
    //  request due to a forking proxy.Each response is distinguished by
    //  the tag parameter in the To header field, and each represents a
    //  distinct dialog, with a distinct dialog identifier.

    //  If the dialog identifier in the 2xx response matches the dialog
    //  identifier of an existing dialog, the dialog MUST be transitioned to
    //  the "confirmed" state, and the route set for the dialog MUST be
    //  recomputed based on the 2xx response using the procedures of Section
    //  12.2.1.2.Otherwise, a new dialog in the "confirmed" state MUST be
    //  constructed using the procedures of Section 12.1.2.
    
    boost::optional<DialogId_t> id = extractDialogId(sipResponse, true);
    assert(id);

    if (m_dialogManager.dialogExists(*id))
    {
      m_dialogManager.getDialog(*id).confirm();
    }
    else
    {
      VLOG(2) << "Creating confirmed dialog";
      m_dialogManager.createDialog(uiTxId, sipRequest, sipResponse, true, false);
    }

    //  Note that the only piece of state that is recomputed is the route
    //  set.Other pieces of state such as the highest sequence numbers
    //  (remote and local) sent within the dialog are not recomputed.The
    //  route set only is recomputed for backwards compatibility.RFC
    //  2543 did not mandate mirroring of the Record - Route header field in
    //  a 1xx, only 2xx.  However, we cannot update the entire state of
    //  the dialog, since mid - dialog requests may have been sent within
    //  the early dialog, modifying the sequence numbers, for example.

    VLOG(6) << "UAC core generating ACK";
    //  The UAC core MUST generate an ACK request for each 2xx received from
    //  the transaction layer.The header fields of the ACK are constructed
    //  in the same way as for any request sent within a dialog(see Section
    //  12) with the exception of the CSeq and the header fields related to
    //  authentication.The sequence number of the CSeq header field MUST be
    //  the same as the INVITE being acknowledged, but the CSeq method MUST
    //  be ACK.The ACK MUST contain the same credentials as the INVITE.If
    //  the 2xx contains an offer(based on the rules above), the ACK MUST
    //  carry an answer in its body.If the offer in the 2xx response is not
    //  acceptable, the UAC core MUST generate a valid answer in the ACK and
    //  then send a BYE immediately.
    SipMessage ack = generateAck(sipRequest, sipResponse);
    
    VLOG(2) << "TODO: If 200 OK contained OFFER, ACK must contain ANSWER";

    VLOG(2) << "If OFFER is not acceptable, generate valid answer and send BYE";

    VLOG(6) << "Storing response and ACK for retransmissions";
    m_vInviteResponsesFromUas.push_back(std::make_pair(sipResponse, ack));
    //  Once the ACK has been constructed, the procedures of[4] are used to
    //  determine the destination address, port and transport.However, the
    //  request is passed to the transport layer directly for transmission,
    //  rather than a client transaction.This is because the UAC core
    //  handles retransmissions of the ACK, not the transaction layer.The
    //  ACK MUST be passed to the client transport every time a
    //  retransmission of the 2xx final response that triggered the ACK
    //  arrives.

    VLOG(5) << "Sending ACK directly to Contact: " << sipResponse.getContact();
    boost::optional<SipUri> pContact = SipUri::parse(sipResponse.getContactUri());
    assert(pContact);
    boost::system::error_code ec = m_transportLayer.sendSipMessage(ack, pContact->getHost(), pContact->getPort(), m_ePreferredProtocol);
    if (ec)
    {
      LOG(WARNING) << "Error sending ACK: " << ec.message();
    }
    else
    {
      VLOG(6) << "Transport layer sent ack";
    }

    //  The UAC core considers the INVITE transaction completed 64 * T1 seconds
    //  after the reception of the first 2xx response.At this point all the
    //  early dialogs that have not transitioned to established dialogs are
    //  terminated.Once the INVITE transaction is considered completed by
    //  the UAC core, no more new 2xx responses are expected to arrive.

    // start timer to terminate unestablished dialogs. At this point we don't have to remember the response anymore
    boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_ioService);
    m_mTimers[pTimer] = pTimer;
    pTimer->expires_from_now(boost::posix_time::milliseconds(64 * INTERVAL_T1_DEFAULT_MS));
    pTimer->async_wait(boost::bind(&UserAgentClient::completeInviteTransaction, this, _1, pTimer, sipResponse));

    //  If, after acknowledging any 2xx response to an INVITE, the UAC does
    //  not want to continue with that dialog, then the UAC MUST terminate
    //  the dialog by sending a BYE request as described in Section 15.
  }
  else // if (isFinalNonSuccessfulResponse(sipResponse.getResponseCode()))
  {
    //  A 3xx response may contain one or more Contact header field values
    //  providing new addresses where the callee might be reachable.
    //  Depending on the status code of the 3xx response(see Section 21.3),
    //  the UAC MAY choose to try those new addresses.
    if (isResponseRedirectional(sipResponse.getResponseCode()))
    {

    }
    //  
    //  A single non - 2xx final response may be received for the INVITE.  4xx,
    //  5xx and 6xx responses may contain a Contact header field value
    //  indicating the location where additional information about the error
    //  can be found.Subsequent final responses(which would only arrive
    //  under error conditions) MUST be ignored.

    //  All early dialogs are considered terminated upon reception of the
    //  non - 2xx final response.

    VLOG(2) << "TODO: terminate early dialog if exists";
    boost::optional<DialogId_t> id = extractDialogId(sipResponse, true);
    assert(id);
    m_dialogManager.terminateDialog(uiTxId, *id);
    //  After having received the non - 2xx final response the UAC core
    //  considers the INVITE transaction completed.The INVITE client
    //  transaction handles the generation of ACKs for the response(see
    //  Section 17).

  }

  if (m_onInviteResponse)
    m_onInviteResponse(ec, uiTxId, sipResponse);
  else
  {
    LOG(WARNING) << "No INVITE response handler configured";
  }
}

void UserAgentClient::completeInviteTransaction(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const SipMessage& sipResponse)
{
  if (!ec)
  {
    VLOG(6) << "INVITE transaction completed - removing timer and sip response.";
    // remove timer from map
    auto it = m_mTimers.find(pTimer);
    m_mTimers.erase(it);
    // forget sip response ack
    auto it2 = std::find_if(m_vInviteResponsesFromUas.begin(), m_vInviteResponsesFromUas.end(), [sipResponse](std::pair<SipMessage, SipMessage>& pair)
    {
      // TODO: does this suffice to match retransmissions?
      return (sipResponse.getHeaderField(FROM) == pair.first.getHeaderField(FROM) &&
        sipResponse.getHeaderField(TO) == pair.first.getHeaderField(TO) &&
        sipResponse.getHeaderField(CALL_ID) == pair.first.getHeaderField(CALL_ID) &&
        sipResponse.getHeaderField(VIA) == pair.first.getHeaderField(VIA)
        );
    });
    assert(it2 != m_vInviteResponsesFromUas.end());
    m_vInviteResponsesFromUas.erase(it2);
  }
  else
  {
    // don't access map: this may already be destroyed
    VLOG(6) << "Timer cancelled?";
  }
  delete pTimer;
}

void UserAgentClient::handleByeResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  VLOG(5) << "BYE response received: " << sipResponse.getResponseCode();
  // terminate dialog

  //  If the response for the BYE is a 481
  //  (Call / Transaction Does Not Exist) or a 408 (Request Timeout) or no
  //  response at all is received for the BYE(that is, a timeout is
  //  returned by the client transaction), the UAC MUST consider the
  //  session and the dialog terminated.

  // To identify the dialog, we need to get the ID from the sipRequest.
  // The uiTxId is the new TX id of the BYE and not the one associated with creation of dialog!
  VLOG(6) << "Terminating dialog";
  boost::optional<DialogId_t> id = extractDialogId(sipRequest, true);
  // FIXME: not sure what id to use as the tx id of the INVITE and BYE vary...
  m_dialogManager.terminateDialog(0, *id);

  if (m_onByeResponse)
  {
    m_onByeResponse(ec, uiTxId, sipResponse);
  }
  else
  {
    VLOG(2) << "No BYE response callback configured";
  }
}

SipMessage UserAgentClient::generateAck(const SipMessage& sipRequest, const SipMessage& sipResponse)
{
  // ACK goes directly to Contact received in response
  // SipMessage ack(ACK, sipRequest.getRequestUri());
  SipMessage ack(ACK, sipResponse.getContactUri());
  ack.copyHeaderField(CALL_ID, sipRequest);
  ack.copyHeaderField(FROM, sipRequest);
  ack.copyHeaderField(MAX_FORWARDS, sipRequest);
  ack.copyHeaderField(TO, sipResponse);

  std::ostringstream via;
  via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";rport=<<RPORT>>" << ";branch=z9hG4bK" << random_string(11);
  VLOG(6) << "new VIA branch parameter for 200 OK: " << via.str();

  ack.addHeaderField(VIA, via.str());
  // ack.copyHeaderField(VIA, sipRequest);
  // Use original request sequence number
  ack.setCSeq(sipRequest.getRequestSequenceNumber());
  return ack;
}

} // rfc3261
} // rtp_plus_plus
