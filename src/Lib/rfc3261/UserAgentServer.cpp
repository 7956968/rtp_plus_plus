#include "CorePch.h"
#include <rtp++/rfc3261/UserAgentServer.h>
#include <boost/algorithm/string.hpp>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipDialog.h>
#include <rtp++/rfc3581/Rfc3581.h>
#include <rtp++/rfc4566/Rfc4566.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

UserAgentServer::UserAgentServer(boost::asio::io_service& ioService, const SipContext& sipContext, DialogManager& dialogManager, 
  TransportLayer& transportLayer, boost::system::error_code& ec)
  :TransactionUser(ioService, sipContext, transportLayer),
  m_dialogManager(dialogManager)
{

}

void UserAgentServer::processIncomingRequestFromTransactionLayer(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse)
{
  //  The UAS will receive the request from the transaction layer.If the
  //  request has a tag in the To header field, the UAS core computes the
  //  dialog identifier corresponding to the request and compares it with
  //  existing dialogs.If there is a match, this is a mid - dialog request.
  //  In that case, the UAS first applies the same processing rules for
  //  requests outside of a dialog, discussed in Section 8.2.

  boost::optional<std::string> to = sipRequest.getHeaderField(TO);
  assert(to);

  // check if the To header has a tag
  if (boost::algorithm::contains(*to, "tag="))
  {
    // in dialog
    processRequestInsideOfDialog(uiTxId, sipRequest, bTUWillSendResponseWithin200Ms, sipResponse);
  }
  else
  {
    // out of dialog
    processRequestOutsideOfDialog(uiTxId, sipRequest, bTUWillSendResponseWithin200Ms, sipResponse);
  }

  //  Requests that do not change in any way the state of a dialog may be
  //  received within a dialog(for example, an OPTIONS request).They are
  //  processed as if they had been received outside the dialog.
  //
  //
  //  When a UAS receives a target refresh request, it MUST replace the
  //  dialog's remote target URI with the URI from the Contact header field
  //  in that request, if present.

}

void UserAgentServer::processRequestInsideOfDialog(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse)
{
  // Can we do this here?
  if (!isMethodAllowed(sipRequest.getMethod()))
  {
    sipResponse.setResponseCode(METHOD_NOT_ALLOWED);
    bTUWillSendResponseWithin200Ms = true;
    return;
  }

  VLOG(5) << "Processing request inside of dialog TxID" << uiTxId << " Method: " << sipRequest.getMethodString();
  //  If the request has a tag in the To header field, but the dialog
  //  identifier does not match any existing dialogs, the UAS may have
  //  crashed and restarted, or it may have received a request for a
  //  different(possibly failed) UAS(the UASs can construct the To tags
  //  so that a UAS can identify that the tag was for a UAS for which it is
  //  providing recovery).Another possibility is that the incoming
  //  request has been simply misrouted.Based on the To tag, the UAS MAY
  //  either accept or reject the request.Accepting the request for
  //  acceptable To tags provides robustness, so that dialogs can persist
  //  even through crashes.UAs wishing to support this capability must
  //  take into consideration some issues such as choosing monotonically
  //  increasing CSeq sequence numbers even across reboots, reconstructing
  //  the route set, and accepting out - of - range RTP timestamps and sequence
  //  numbers.
  //
  //  If the UAS wishes to reject the request because it does not wish to
  //  recreate the dialog, it MUST respond to the request with a 481
  //  (Call / Transaction Does Not Exist) status code and pass that to the
  //  server transaction.

  boost::optional<std::string> callId = sipRequest.getHeaderField(CALL_ID);
  boost::optional<std::string> from = sipRequest.getHeaderField(FROM);
  boost::optional<std::string> to = sipRequest.getHeaderField(TO);

  // compute dialog ID
  boost::optional<DialogId_t> id = extractDialogId(sipRequest, false);
  assert(id);

  bTUWillSendResponseWithin200Ms = true;
  m_dialogManager.handleRequest(*id, sipRequest, sipResponse);

  if (sipRequest.getMethod() == BYE)
  {
    // TODO: bye handler notify SipAgent and UI to allow to stop call etc
    if (m_onBye)
    {
      m_onBye(uiTxId, sipRequest);
    }
    else
    {
      VLOG(2) << "No Bye notification set";
    }
  }
}

void UserAgentServer::processRequestOutsideOfDialog(const uint32_t uiTxId, const SipMessage& sipRequest, bool& bTUWillSendResponseWithin200Ms, SipMessage& sipResponse)
{
  //  Once a request is authenticated(or authentication is skipped), the
  //  UAS MUST inspect the method of the request.If the UAS recognizes
  //  but does not support the method of a request, it MUST generate a 405
  //  (Method Not Allowed) response.Procedures for generating responses
  //  are described in Section 8.2.6.The UAS MUST also add an Allow
  //  header field to the 405 (Method Not Allowed) response.The Allow
  //  header field MUST list the set of methods supported by the UAS
  //  generating the message.The Allow header field is presented in
  //  Section 20.5.
  if (!isMethodAllowed(sipRequest.getMethod()))
  {
    sipResponse.setResponseCode(METHOD_NOT_ALLOWED);
    bTUWillSendResponseWithin200Ms = true;
    return;
  }
  //If a UAS does not understand a header field in a request(that is,
  //  the header field is not defined in this specification or in any
  //  supported extension), the server MUST ignore that header field and
  //  continue processing the message.A UAS SHOULD ignore any malformed
  //  header fields that are not necessary for processing requests.
  VLOG(6) << "TODO: Process headers";
  // 8.2.2.1 To and Request-URI

  switch (sipRequest.getMethod())
  {
  case REGISTER:
  {
    VLOG(6) << "UAS REGISTER";
    if (m_onRegister)
    {
      m_onRegister(uiTxId, sipRequest, sipResponse);
      return;
    }
    else
    {
      LOG(WARNING) << "REGISTER CALLBACK NOT SET";
      sipResponse.setResponseCode(INTERNAL_SERVER_ERROR);
    }
    return;
  }
  case INVITE:
  {
    VLOG(6) << "UAS INVITE";
    // notify user of incoming call
    assert(m_onInvite);
    m_onInvite(uiTxId, sipRequest);
    VLOG(6) << "UAS INVITE Done";
    return;
  }
  case ACK:
  {
    VLOG(6) << "TODO: UAS has received ACK, check for response that caused ACK and stop retransmission + timers";
    return;
  }
    // CAN A BYE be received outside of dialog?
#if 1
  case BYE:
  {
    VLOG(6) << "BYE received outside of dialog";
    break;
  }
#endif
  default:
  {
    assert(false);
  }
  }
}

bool UserAgentServer::isMethodAllowed(Method eMethod) const
{
  switch (eMethod)
  {
  case REGISTER:
  {
    return true;
  }
  case INVITE:
  {
    return true;
  }
  case ACK:
  {
    return true;
  }
  case BYE:
  {
    return true;
  }
  default:
  {
    VLOG(2) << "Method not allowed: " << methodToString(eMethod);
    return false;
  }
  }
}

void UserAgentServer::handleIncomingRequestFromTransportLayer(const SipMessage& sipRequest, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  VLOG(5) << "SIP request received by UAS from " << sSource << ":" << uiPort;
  //  18.2.1 Receiving Requests
  //  When the server transport receives a request over any transport, it
  //  MUST examine the value of the "sent-by" parameter in the top Via
  //  header field value.If the host portion of the "sent-by" parameter
  //  contains a domain name, or if it contains an IP address that differs
  //  from the packet source address, the server MUST add a "received"
  //  parameter to that Via header field value.This parameter MUST
  //  contain the source address from which the packet was received.This
  //  is to assist the server transport layer in sending the response,
  //  since it must be sent to the source IP address from which the request
  //  came.
  SipMessage& request = const_cast<SipMessage&>(sipRequest);
  std::ostringstream received;
  received << "received=" << sSource;
  request.appendToHeaderField(VIA, received.str());

  //  RFC3581
  //  When a server compliant to this specification(which can be a proxy
  //  or UAS) receives a request, it examines the topmost Via header field
  //  value.If this Via header field value contains an "rport" parameter
  //  with no value, it MUST set the value of the parameter to the source
  //  port of the request.This is analogous to the way in which a server
  //  will insert the "received" parameter into the topmost Via header
  //  field value.In fact, the server MUST insert a "received" parameter
  //  containing the source IP address that the request came from, even if
  //  it is identical to the value of the "sent-by" component.Note that
  //  this processing takes place independent of the transport protocol.
  if (request.hasAttribute(VIA, rfc3581::RPORT))
  {
    std::ostringstream via;
    // copy the via: break into parts and then replace the part
    std::string sTopMostVia = request.getTopMostVia();
    std::vector<std::string> attributes = StringTokenizer::tokenize(sTopMostVia, ";", false, true);
    for (size_t i = 0; i < attributes.size(); ++i)
    {
      if (boost::algorithm::contains(attributes[i], rfc3581::RPORT))
      {
        via << attributes[i] << "=" << uiPort;
      }
      else
      {
        via << attributes[i];
      }
      if (i < attributes.size() - 1)
        via << ";";
    }
    VLOG(6) << "Updating topmost via to " << via.str();
    request.setTopMostVia(via.str());
  }


  // 17.2.3 Matching Requests to Server Transaction

  // When a request is received from the network by the server, it has to
  //  be matched to an existing transaction.This is accomplished in the
  //  following manner.

  //  The branch parameter in the topmost Via header field of the request
  //  is examined.If it is present and begins with the magic cookie
  //  "z9hG4bK", the request was generated by a client transaction
  //  compliant to this specification.Therefore, the branch parameter
  //  will be unique across all transactions sent by that client.The
  //  request matches a transaction if:

  // 1. the branch parameter in the request is equal to the one in the
  //  top Via header field of the request that created the
  //  transaction, and

  //  2. the sent - by value in the top Via of the request is equal to the
  //  one in the request that created the transaction, and

  //  3. the method of the request matches the one that created the
  //  transaction, except for ACK, where the method of the request
  //  that created the transaction is INVITE.

  //  This matching rule applies to both INVITE and non - INVITE transactions
  //  alike.
  boost::optional<std::string> branch = sipRequest.getBranchParameter();
  if (branch && boost::algorithm::starts_with(*branch, MAGIC_COOKIE))
  {
    VLOG(6) << "Branch parameter: " << *branch;
    for (auto& pair : m_mServerTransactions)
    {
      // 1. & 2.
      if (
          sipRequest.matchHeaderFieldAttribute(VIA, BRANCH, pair.second->getRequest()) &&
          sipRequest.getSentByInTopMostVia() == pair.second->getRequest().getSentByInTopMostVia()
          )
      {
        VLOG(6) << "Matched branch parameter and sent-by";
        // 3.
        std::string sMethod1 = pair.second->getRequest().getMethodString();
        if (sMethod1 == methodToString(INVITE))
        {
          std::string sMethod2 = sipRequest.getMethodString();
          if (sMethod2 == methodToString(ACK))
          {
            // found match: let transaction handle response
            pair.second->onRequestReceived(sipRequest);
            return;
          }
        }
        else
        {
          // just check for match
          std::string sMethod2 = sipRequest.getMethodString();
          if (sMethod1 == sMethod2)
          {
            // found match: let transaction handle response
            pair.second->onRequestReceived(sipRequest);
            return;
          }
        }
      }
    }

    // no matching transaction was found
    // Create server transaction for incoming request
    //  The server transaction is responsible for the delivery of requests to
    //  the TU and the reliable transmission of responses.It accomplishes
    //  this through a state machine.Server transactions are created by the
    //  core when a request is received, and transaction handling is desired
    //  for that request(this is not always the case).

    //  As with the client transactions, the state machine depends on whether
    //  the received request is an INVITE request.

    // RG: if a 2xx response was sent to an INVITE, then the server tx has been destroyed.
    // Since an ACK to a 200 should be its own tx anyway, there is no way for it to match
    // to a previous tx.
    // Therefore we should just check all requests here to see if they are ACKS, in which case
    // we try to match them to an response that the TU is trying to send directly via the transport layer?

    uint32_t uiTxId; // TODO: do we need this?
    beginServerTransaction(uiTxId, sipRequest, sSource, uiPort, eProtocol);

    //  Requests sent within a dialog, as any other requests, are atomic.If
    //  a particular request is accepted by the UAS, all the state changes
    //  associated with it are performed.If the request is rejected, none
    //  of the state changes are performed.
    //
    //  Note that some requests, such as INVITEs, affect several pieces of
    //  state.
    //
  }
  else
  {
    VLOG(6) << "No branch parameter";
    //  If the branch parameter in the top Via header field is not present,
    //  or does not contain the magic cookie, the following procedures are
    //  used.These exist to handle backwards compatibility with RFC 2543
    //  compliant implementations.

    //  The INVITE request matches a transaction if the Request - URI, To tag,
    //  From tag, Call - ID, CSeq, and top Via header field match those of the
    //  INVITE request which created the transaction.In this case, the
    //  INVITE is a retransmission of the original one that created the
    //  transaction.The ACK request matches a transaction if the Request -
    //  URI, From tag, Call - ID, CSeq number(not the method), and top Via
    //  header field match those of the INVITE request which created the
    //  transaction, and the To tag of the ACK matches the To tag of the
    //  response sent by the server transaction.Matching is done based on
    //  the matching rules defined for each of those header fields.
    //  Inclusion of the tag in the To header field in the ACK matching
    //  process helps disambiguate ACK for 2xx from ACK for other responses
    //  at a proxy, which may have forwarded both responses(This can occur
    //  in unusual conditions.Specifically, when a proxy forked a request,
    //  and then crashes, the responses may be delivered to another proxy,
    //  which might end up forwarding multiple responses upstream).An ACK
    //  request that matches an INVITE transaction matched by a previous ACK
    //  is considered a retransmission of that previous ACK.
    return;
  }
  //  The sent - by value is used as part of the matching process because
  //  there could be accidental or malicious duplication of branch
  //  parameters from different clients.



  // where do we determine whether a request is a retransmission or not?
}

boost::system::error_code UserAgentServer::ringing(const uint32_t uiTxId)
{
  // locate server tx and use it to send reject.
  auto it = m_mServerTransactions.find(uiTxId);
  if (it == m_mServerTransactions.end())
  {
    return boost::system::error_code(boost::system::errc::no_such_device, boost::system::get_generic_category());
  }
  SipMessage sipResponse = m_mServerTransactions[uiTxId]->generateResponse(RINGING);
  m_mServerTransactions[uiTxId]->sendResponse(sipResponse);
  return boost::system::error_code();
}

boost::system::error_code UserAgentServer::reject(const uint32_t uiTxId)
{
  // locate server tx and use it to send reject.
  auto it = m_mServerTransactions.find(uiTxId);
  if (it == m_mServerTransactions.end())
  {
    return boost::system::error_code(boost::system::errc::no_such_device, boost::system::get_generic_category());
  }
  // TODO: allow application to reject call based on bad offer. For this the application 
  // would have to supply a response code
  // RFC6337 
  //  When a UA receives an INVITE request with an unacceptable offer, it
  //  should respond with a 488 response, preferably with Warning header
  //  field indicating the reason of the rejection, unless another response
  //  code is more appropriate to reject it(Pattern 1 and Pattern 3).
  SipMessage sipResponse = m_mServerTransactions[uiTxId]->generateResponse(BUSY_HERE);
  m_mServerTransactions[uiTxId]->sendResponse(sipResponse);
  return boost::system::error_code();
}

boost::system::error_code UserAgentServer::accept(const uint32_t uiTxId, const MessageBody& sessionDescription)
{
  // 13.3.1.4 The INVITE is Accepted
  //  The UAS core generates a 2xx response.This response establishes a
  //  dialog, and therefore follows the procedures of Section 12.1.1 in
  //  addition to those of Section 8.2.6.

  // locate server tx
  auto it = m_mServerTransactions.find(uiTxId);
  if (it == m_mServerTransactions.end())
  {
    return boost::system::error_code(boost::system::errc::no_such_device, boost::system::get_generic_category());
  }
  SipMessage sipResponse = m_mServerTransactions[uiTxId]->generateResponse(OK);
  //  A 2xx response to an INVITE SHOULD contain the Allow header field and
  //  the Supported header field, and MAY contain the Accept header field.
  //  Including these header fields allows the UAC to determine the
  //  features and extensions supported by the UAS for the duration of the
  //  call, without probing.
  VLOG(6) << "TODO: Add Allow, Supported, Accept";

  //  If the INVITE request contained an offer, and the UAS had not yet
  //  sent an answer, the 2xx MUST contain an answer.If the INVITE did
  //  not contain an offer, the 2xx MUST contain an offer if the UAS had
  //  not yet sent an offer.
  
  VLOG(6) << "TODO: if the INVITE contained offer, add answer, else add offer";
  sipResponse.setMessageBody(sessionDescription);

  //  Once the response has been constructed, it is passed to the INVITE
  //  server transaction.Note, however, that the INVITE server
  //  transaction will be destroyed as soon as it receives this final
  //  response and passes it to the transport.Therefore, it is necessary
  //  to periodically pass the response directly to the transport until the
  //  ACK arrives.The 2xx response is passed to the transport with an
  //  interval that starts at T1 seconds and doubles for each
  //  retransmission until it reaches T2 seconds(T1 and T2 are defined in
  //  Section 17).Response retransmissions cease when an ACK request for
  //  the response is received.This is independent of whatever transport
  //  protocols are used to send the response.

  // create dialog before sending the response destroys transaction
  const SipMessage& invite = m_mServerTransactions[uiTxId]->getRequest();
  DialogId_t id = m_dialogManager.createDialog(uiTxId, invite, sipResponse, false, false);

  // destroys server transaction
  m_mServerTransactions[uiTxId]->sendResponse(sipResponse);
  VLOG(6) << "TODO: remember response and send response periodically until ACK is received";
  // HACK: since the client transaction is supposed to be destroyed when a 200 OK is sent,
  // not sure where to store the response, using dialog for now
  m_dialogManager.getDialog(id).onSend200Ok();

  //  Since 2xx is retransmitted end - to - end, there may be hops between
  //  UAS and UAC that are UDP.To ensure reliable delivery across
  //  these hops, the response is retransmitted periodically even if the
  //  transport at the UAS is reliable.

  //  If the server retransmits the 2xx response for 64 * T1 seconds without
  //  receiving an ACK, the dialog is confirmed, but the session SHOULD be
  //  terminated.This is accomplished with a BYE, as described in Section
  //  15.

  // start timer to periodically send response until ACK is received
  VLOG(2) << "TODO: fire timer in 64 * T1 after which session should be terminated if no ACK was received";

  return boost::system::error_code();
}

} // rfc3261
} // rtp_plus_plus
