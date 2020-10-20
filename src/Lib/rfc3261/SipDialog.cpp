#include "CorePch.h"
#include <rtp++/rfc3261/SipDialog.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/util/RandomUtil.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

boost::optional<DialogId_t> extractDialogId(const SipMessage& sipMessage, bool isUac)
{
  if (isUac)
  {
    auto callId = sipMessage.getHeaderField(CALL_ID);
    auto localTag = sipMessage.getAttribute(FROM, TAG);
    auto remoteTag = sipMessage.getAttribute(TO, TAG);
    std::string sRemoteTag = remoteTag ? *remoteTag : "null";
    return boost::optional<DialogId_t>(std::make_tuple(*callId, *localTag, sRemoteTag));
  }
  else
  {
    auto callId = sipMessage.getHeaderField(CALL_ID);
    auto localTag = sipMessage.getAttribute(TO, TAG);
    auto remoteTag = sipMessage.getAttribute(FROM, TAG);

    if (callId && localTag && remoteTag)
      return boost::optional<DialogId_t>(std::make_tuple(*callId, *localTag, *remoteTag));
    else if (callId && localTag)
    {
      //A UAS MUST be prepared to receive a
      //  request without a tag in the From field, in which case the tag is
      //  considered to have a value of null.
      return boost::optional<DialogId_t>(std::make_tuple(*callId, *localTag, "null"));
    }
  }
  return boost::optional<DialogId_t>();
}

std::ostream& operator<<(std::ostream& ostream, const DialogId_t& dialogId)
{
  ostream << std::get<0>(dialogId) << ":" << std::get<1>(dialogId) << ":" << std::get<2>(dialogId);
  return ostream;
}

SipDialog::SipDialog(DialogState eState)
  :m_eState(eState), 
  m_uiTxId(UINT32_MAX),
  m_bIsUAC(false),
  m_bSecure(false)
{
  VLOG(10) << "SipDialog constructor";
}

SipDialog::SipDialog(const SipContext& sipContext, DialogState eState)
  :m_eState(eState), 
  m_sipContext(sipContext),
  m_uiTxId(UINT32_MAX),
  m_bIsUAC(false),
  m_bSecure(false)
{
  VLOG(10) << "SipDialog constructor";
}

SipDialog::SipDialog(const SipContext& sipContext, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUAC, bool bSecure, DialogState eState)
  :m_eState(eState),
  m_sipContext(sipContext),
  m_uiTxId(uiTxId),
  m_bIsUAC(bIsUAC),
  m_bSecure(bSecure)
{
  VLOG(10) << "SipDialog constructor";
  intialiseDialogState(bIsUAC, sipRequest);
}

SipDialog::SipDialog(const SipContext& sipContext, const uint32_t uiTxId, DialogId_t id, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUAC, bool bSecure, DialogState eState)
  :m_eState(eState),
  m_sipContext(sipContext),
  m_uiTxId(uiTxId),
  m_dialogId(id),
  m_bIsUAC(bIsUAC),
  m_bSecure(bSecure)
{
  VLOG(10) << "SipDialog constructor: " << id;
  intialiseDialogState(bIsUAC, sipRequest);
}

SipDialog::~SipDialog()
{
  VLOG(10) << "SipDialog destructor";
}

void SipDialog::confirm()
{
  VLOG(2) << "SipDialog confirmed";
  m_eState = DS_CONFIRMED;
  VLOG(2) << "TODO: recompute route set";
}

void SipDialog::intialiseDialogState(bool bIsUac, const SipMessage& sipMessage)
{
  if (!bIsUac)
  {
    //  The UAS then constructs the state of the dialog.This state MUST be
    //  maintained for the duration of the dialog.

    //  If the request arrived over TLS, and the Request - URI contained a SIPS
    //  URI, the "secure" flag is set to TRUE.

    //  The route set MUST be set to the list of URIs in the Record - Route
    //  header field from the request, taken in order and preserving all URI
    //  parameters.If no Record - Route header field is present in the
    //  request, the route set MUST be set to the empty set.This route set,
    //  even if empty, overrides any pre - existing route set for future
    //  requests in this dialog.The remote target MUST be set to the URI
    //  from the Contact header field of the request.
    VLOG(5) << "TODO: Record-Route";

    //  The remote sequence number MUST be set to the value of the sequence
    //  number in the CSeq header field of the request.The local sequence
    //  number MUST be empty.The call identifier component of the dialog ID
    //  MUST be set to the value of the Call - ID in the request.The local
    //  tag component of the dialog ID MUST be set to the tag in the To field
    //  in the response to the request(which always includes a tag), and the
    //  remote tag component of the dialog ID MUST be set to the tag from the
    //  From field in the request.A UAS MUST be prepared to receive a
    //  request without a tag in the From field, in which case the tag is
    //  considered to have a value of null.
    m_uiRemoteSN = sipMessage.getRequestSequenceNumber();
    m_uiLocalSN = EMPTY;

    //  This is to maintain backwards compatibility with RFC 2543, which
    //  did not mandate From tags.
    //  The remote URI MUST be set to the URI in the From field, and the
    //  local URI MUST be set to the URI in the To field.
    boost::optional<SipUri> remoteUri = SipUri::parse(sipMessage.getFromUri());
    assert(remoteUri);
    m_remoteUri = *remoteUri;
    boost::optional<SipUri> localUri = SipUri::parse(sipMessage.getToUri());
    assert(localUri);
    m_localUri = *localUri;
  }
  else
  {
    //  When a UAC receives a response that establishes a dialog, it
    //  constructs the state of the dialog.This state MUST be maintained
    //  for the duration of the dialog.

    //  If the request was sent over TLS, and the Request - URI contained a
    //  SIPS URI, the "secure" flag is set to TRUE.

    //  The route set MUST be set to the list of URIs in the Record - Route
    //  header field from the response, taken in reverse order and preserving
    //  all URI parameters.If no Record - Route header field is present in
    //  the response, the route set MUST be set to the empty set.This route
    //  set, even if empty, overrides any pre - existing route set for future
    //  requests in this dialog.The remote target MUST be set to the URI
    //  from the Contact header field of the response.
    VLOG(5) << "TODO: Record-Route";

    //  The local sequence number MUST be set to the value of the sequence
    //  number in the CSeq header field of the request.The remote sequence
    //  number MUST be empty(it is established when the remote UA sends a
    //  request within the dialog).The call identifier component of the
    //  dialog ID MUST be set to the value of the Call - ID in the request.
    //  The local tag component of the dialog ID MUST be set to the tag in
    //  the From field in the request, and the remote tag component of the
    //  dialog ID MUST be set to the tag in the To field of the response.A
    //  UAC MUST be prepared to receive a response without a tag in the To
    //  field, in which case the tag is considered to have a value of null.
    m_uiLocalSN = sipMessage.getRequestSequenceNumber();
    m_uiRemoteSN = EMPTY;
    //  This is to maintain backwards compatibility with RFC 2543, which
    //  did not mandate To tags.

    //  The remote URI MUST be set to the URI in the To field, and the local
    //  URI MUST be set to the URI in the From field.
    boost::optional<SipUri> remoteUri = SipUri::parse(sipMessage.getToUri());
    assert(remoteUri);
    m_remoteUri = *remoteUri;
    boost::optional<SipUri> localUri = SipUri::parse(sipMessage.getFromUri());
    assert(localUri);
    m_localUri = *localUri;
  }
}

void SipDialog::onSend200Ok()
{
  VLOG(10) << "SipDialog::onSend200Ok";
  VLOG(6) << "TODO: remember response and send response periodically until ACK is received";
}

SipMessage SipDialog::generateSipRequest(const std::string& sMethod)
{
  // The request URU will be calculated later
  SipMessage sipMessage(sMethod, "");

  //  A request within a dialog is constructed by using many of the
  //  components of the state stored as part of the dialog.
  //
  //  The URI in the To field of the request MUST be set to the remote URI
  //  from the dialog state.The tag in the To header field of the request
  //  MUST be set to the remote tag of the dialog ID.The From URI of the
  //  request MUST be set to the local URI from the dialog state.The tag
  //  in the From header field of the request MUST be set to the local tag
  //  of the dialog ID.If the value of the remote or local tags is null,
  //  the tag parameter MUST be omitted from the To or From header fields,
  //  respectively.
  //
  std::string sToTag = std::get<2>(m_dialogId);
  sipMessage.addHeaderField(TO, *m_remoteUri.toString());
  if (!sToTag.empty() && sToTag != "null")
    sipMessage.setAttribute(TO, TAG, sToTag);

  std::string sFromTag = std::get<1>(m_dialogId);
  sipMessage.addHeaderField(FROM, *m_localUri.toString());
  if (!sFromTag.empty() && sFromTag != "null")
    sipMessage.setAttribute(FROM, TAG, sFromTag);

  //  The Call - ID of the request MUST be set to the Call - ID of the dialog.
  sipMessage.addHeaderField(CALL_ID, std::get<0>(m_dialogId));

  //  Requests within a dialog MUST contain strictly monotonically
  //  increasing and contiguous CSeq sequence numbers(increasing - by - one)
  //  in each direction(excepting ACK and CANCEL of course, whose numbers
  //  equal the requests being acknowledged or cancelled).Therefore, if
  //  the local sequence number is not empty, the value of the local
  //  sequence number MUST be incremented by one, and this value MUST be
  //  placed into the CSeq header field.If the local sequence number is
  //  empty, an initial value MUST be chosen using the guidelines of
  //  Section 8.1.1.5.The method field in the CSeq header field value
  //  MUST match the method of the request.
  //  With a length of 32 bits, a client could generate, within a single
  //  call, one request a second for about 136 years before needing to
  //  wrap around.The initial value of the sequence number is chosen
  //  so that subsequent requests within the same call will not wrap
  //  around.A non - zero initial value allows clients to use a time -
  //  based initial sequence number.A client could, for example,
  //  choose the 31 most significant bits of a 32 - bit second clock as an
  //  initial sequence number.
  uint32_t uiSN = 1; // should this be random for the initial request?!?
  sipMessage.addHeaderField(CSEQ, ::toString(uiSN) + " " + sMethod);

  //  The UAC uses the remote target and route set to build the Request - URI
  //  and Route header field of the request.
  //
  //  If the route set is empty, the UAC MUST place the remote target URI
  //  into the Request - URI.The UAC MUST NOT add a Route header field to
  //  the request.
  if (m_routeSet.empty())
  {
    sipMessage.setRequestUri(*m_remoteUri.toString());
  }
  else
  {
    VLOG(2) << "TODO: set request URI from route set for in dialog request";
  }
  //
  //  If the route set is not empty, and the first URI in the route set
  //  contains the lr parameter(see Section 19.1.1), the UAC MUST place
  //  the remote target URI into the Request - URI and MUST include a Route
  //  header field containing the route set values in order, including all
  //  parameters.
  //
  //  If the route set is not empty, and its first URI does not contain the
  //  lr parameter, the UAC MUST place the first URI from the route set
  //  into the Request - URI, stripping any parameters that are not allowed
  //  in a Request - URI.The UAC MUST add a Route header field containing
  //  the remainder of the route set values in order, including all
  //  parameters.The UAC MUST then place the remote target URI into the
  //  Route header field as the last value.
  //
  //  For example, if the remote target is sip : user@remoteua and the route
  //  set contains :

  //  <sip : proxy1>, <sip : proxy2>, <sip : proxy3; lr>, <sip : proxy4>
  //
  //  The request will be formed with the following Request - URI and Route
  //  header field :
  //
  //  METHOD sip : proxy1
  //  Route : <sip : proxy2>, <sip : proxy3; lr>, <sip : proxy4>, <sip : user@remoteua>

  //  If the first URI of the route set does not contain the lr
  //  parameter, the proxy indicated does not understand the routing
  //  mechanisms described in this document and will act as specified in
  //  RFC 2543, replacing the Request - URI with the first Route header
  //  field value it receives while forwarding the message.Placing the
  //  Request - URI at the end of the Route header field preserves the
  //  information in that Request - URI across the strict router(it will
  //  be returned to the Request - URI when the request reaches a loose -
  //  router).
  VLOG(2) << "TODO: route-set";

  //  A UAC SHOULD include a Contact header field in any target refresh
  //  requests within a dialog, and unless there is a need to change it,
  //  the URI SHOULD be the same as used in previous requests within the
  //  dialog.If the "secure" flag is true, that URI MUST be a SIPS URI.
  //  As discussed in Section 12.2.2, a Contact header field in a target
  //  refresh request updates the remote target URI.This allows a UA to
  //  provide a new contact address, should its address change during the
  //  duration of the dialog.
  sipMessage.addHeaderField(CONTACT, m_sipContext.getContact()); // TODO: verify that this is ok?!?

  //  However, requests that are not target refresh requests do not affect
  //  the remote target URI for the dialog.

  //  The rest of the request is formed as described in Section 8.1.1.
  // Max-Forwards: 70
  uint32_t uiMaxForwards = 70;
  sipMessage.addHeaderField(MAX_FORWARDS, ::toString(uiMaxForwards));
  // Via:
  std::ostringstream via;
  // std::string sentBy(pLocalSipUri->getHost());
  via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";branch=z9hG4bK" << random_string(11); // TODO?
  // <<PROTO>> and <<FQDN>> are set by transport layer
  sipMessage.addHeaderField(VIA, via.str());

  //  Once the request has been constructed, the address of the server is
  //  computed and the request is sent, using the same procedures for
  //  requests outside of a dialog(Section 8.1.2).

  //  The procedures in Section 8.1.2 will normally result in the
  //  request being sent to the address indicated by the topmost Route
  //  header field value or the Request - URI if no Route header field is
  //  present.Subject to certain restrictions, they allow the request
  //  to be sent to an alternate address(such as a default outbound
  //  proxy not represented in the route set).

  return sipMessage;
}

void SipDialog::handleRequest(const SipMessage& sipRequest, SipMessage& sipResponse)
{
  VLOG(10) << "SipDialog::handleRequest";
  //  If the remote sequence number is empty, it MUST be set to the value
  //  of the sequence number in the CSeq header field value in the request.
  //  If the remote sequence number was not empty, but the sequence number
  //  of the request is lower than the remote sequence number, the request
  //  is out of order and MUST be rejected with a 500 (Server Internal
  //  Error) response.If the remote sequence number was not empty, and
  //  the sequence number of the request is greater than the remote
  //  sequence number, the request is in order.It is possible for the
  //  CSeq sequence number to be higher than the remote sequence number by
  //  more than one.This is not an error condition, and a UAS SHOULD be
  //  prepared to receive and process requests with CSeq values more than
  //  one higher than the previous received request.The UAS MUST then set
  //  the remote sequence number to the value of the sequence number in the
  //  CSeq header field value in the request.

  if (m_uiRemoteSN == EMPTY)
  {
    m_uiRemoteSN = sipRequest.getRequestSequenceNumber();
    VLOG(6) << "Initial dialog remote SN to " << m_uiRemoteSN;
  }
  else
  {
    if (sipRequest.getRequestSequenceNumber() < m_uiRemoteSN)
    {
      LOG(WARNING) << "Request out of order";
      sipResponse.setResponseCode(INTERNAL_SERVER_ERROR);
      return;
    }
    else
    {
      m_uiRemoteSN = sipRequest.getRequestSequenceNumber();
      VLOG(6) << "Updated dialog remote SN to " << m_uiRemoteSN;
    }
  }


  //  If a proxy challenges a request generated by the UAC, the UAC has
  //  to resubmit the request with credentials.The resubmitted request
  //  will have a new CSeq number.The UAS will never see the first
  //  request, and thus, it will notice a gap in the CSeq number space.
  //  Such a gap does not represent any error condition.

  switch (sipRequest.getMethod())
  {
  case INVITE:
  {
    VLOG(6) << "TODO: re-INVITE";
    assert(false);
    break;
  }
  case ACK:
  {
    VLOG(6) << "TODO: ACK received in dialog";
    VLOG(6) << "TODO: STOP RTX of 200 OK";
    break;
  }
#if 1
  case BYE:
  {
    VLOG(6) << "BYE received in dialog";
    sipResponse.setResponseCode(OK);
    break;
  }
#endif
  default:
  {
    assert(false);
  }
  }
}

void SipDialog::terminateDialog()
{
  VLOG(6) << "SipDialog terminating";
  m_eState = DS_COMPLETE;
}


} // rfc3261
} // rtp_plus_plus
