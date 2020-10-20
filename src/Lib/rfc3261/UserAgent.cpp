#include "CorePch.h"
#include <rtp++/rfc3261/UserAgent.h>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <rtp++/rfc3261/SipErrorCategory.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

UserAgent::UserAgent(boost::asio::io_service& ioService, const SipContext& sipContext, boost::system::error_code& ec)
  :UserAgentBase(ioService, sipContext.FQDN, sipContext.SipPort),
  m_dialogManager(sipContext)
{
    initialiseUacAndUas(sipContext, ec);
}

void UserAgent::initialiseUacAndUas(const SipContext& sipContext, boost::system::error_code& ec)
{
  m_pUac = std::unique_ptr<UserAgentClient>(new UserAgentClient(m_ioService, sipContext, m_dialogManager, m_transportLayer, ec));
  if (ec)
  {
    VLOG(2) << "Error creating UAC: " << ec.message();
    return;
  }
  else
  {
    VLOG(12) << "UAC created";
    m_pUac->setInviteResponseHandler(boost::bind(&UserAgent::onInviteResponse, this, _1, _2, _3));
    m_pUac->setRegisterResponseHandler(boost::bind(&UserAgent::onRegisterResponse, this, _1, _2, _3));
    m_pUac->setByeResponseHandler(boost::bind(&UserAgent::onByeResponse, this, _1, _2, _3));
  }
  m_pUas = std::unique_ptr<UserAgentServer>(new UserAgentServer(m_ioService, sipContext, m_dialogManager, m_transportLayer, ec));
  if (ec)
  {
    VLOG(2) << "Error creating UAS: " << ec.message();
    return;
  }
  else
  {
    VLOG(12) << "UAS created";
    m_pUas->setInviteHandler(boost::bind(&UserAgent::onInvite, this, _1, _2));
    m_pUas->setByeHandler(boost::bind(&UserAgent::onBye, this, _1, _2));
    m_pUas->setRegisterHandler(boost::bind(&UserAgent::onRegister, this, _1, _2, _3));
  }
}

boost::system::error_code UserAgent::doStart()
{
  return boost::system::error_code();
}

boost::system::error_code UserAgent::doStop()
{
  VLOG(6) << "UserAgent::doStop()";

  // terminate existing dialogs: this will result in UAC and UAS sending/receiving messages
  terminateDialogs();

  // TODO: shutdown UAS to not accept new requests but only responses, such as response to BYE?

  // TODO: option one loop while waiting for dialogs to end and transactions to complete
  // option two would be to start shutdown of UAC and UAS and then wait for all transactions
  // to complete using timer and callbacks
  const int MAX_WAIT_SECONDS = 30;
  int iWait = 0;
  while (true)
  {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    if (m_dialogManager.getDialogCount() == 0)
    {
      VLOG(5) << "All dialogs completed, shutting down transport layer";
      break;
    }
    else
    {
      VLOG(5) << "Waiting for active dialogs to complete";
      ++iWait;
      if (iWait >= MAX_WAIT_SECONDS)
      {
        LOG(INFO) << "Max wait time reached - exiting application";
        break;
      }
    }
  }

  return boost::system::error_code();
}

void UserAgent::terminateDialogs()
{
  VLOG(6) << "TODO: implement rules to send BYE";

  std::vector<DialogId_t> vDialogIds = m_dialogManager.getActiveDialogIds();
  for (auto& id : vDialogIds)
  {
    uint32_t uiTxId = 0;
    // use dialog to generate BYE
    SipMessage bye = m_dialogManager.getDialog(id).generateSipRequest(methodToString(BYE));
    m_dialogManager.getDialog(id).terminateDialog();
    boost::optional<SipUri> pSipUri = SipUri::parse(bye.getToUri());
    assert(pSipUri);
    boost::system::error_code ec = m_pUac->beginTransaction(uiTxId, bye, pSipUri->getHost(), pSipUri->getPort(), m_pUac->getPreferredProtocol());
    if (ec)
    {
      LOG(WARNING) << "Failed to begin BYE transaction for dialog " << id;
    }
  }
}

void UserAgent::doHandleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  if (sipMessage.isRequest())
  {
    if (isValidRequest(sipMessage))
    {
      // pass of request to UAS
      m_pUas->handleIncomingRequestFromTransportLayer(sipMessage, sSource, uiPort, eProtocol);
    }
    else
    {
      LOG(WARNING) << "Invalid SIP message: " << sipMessage.toString();
    }
  }
  else if (isValidResponse(sipMessage))
  {
    // pass of request to UAS
    m_pUac->handleIncomingResponseFromTransportLayer(sipMessage, sSource, uiPort, eProtocol);
  }
}

boost::system::error_code UserAgent::registerWithServer(const SipUri& sipServer, const std::string& to, const std::string& from, uint32_t uiExpiresSeconds, uint32_t& uiTxId)
{
  VLOG(2) << "Send register request to server. Need registration callback";
  return m_pUac->registerWithServer(sipServer, to, from, uiExpiresSeconds, uiTxId);
}

boost::system::error_code UserAgent::initiateSession(const std::string& to, const std::string& from, const std::string& sDomainOrHost, uint32_t& uiTxId,
                                                     const MessageBody& sessionDescription, const std::string& sProxy)
{
  boost::system::error_code ec = m_pUac->invite(to, from, sDomainOrHost, uiTxId, sessionDescription, sProxy);
  m_mCallerMap[uiTxId] = false;
  return ec;
}

boost::system::error_code UserAgent::terminateSession(const uint32_t uiTxId)
{
  // TODO: UA needs to remember if the local agent is the caller or callee.
  //  This section describes the procedures for terminating a session
  //  established by SIP.The state of the session and the state of the
  //  dialog are very closely related.When a session is initiated with an
  //  INVITE, each 1xx or 2xx response from a distinct UAS creates a
  //  dialog, and if that response completes the offer / answer exchange, it
  //  also creates a session.As a result, each session is "associated"
  //  with a single dialog - the one which resulted in its creation.If an
  //  initial INVITE generates a non - 2xx final response, that terminates
  //  all sessions(if any) and all dialogs(if any) that were created
  //  through responses to the request.By virtue of completing the
  //  transaction, a non - 2xx final response also prevents further sessions
  //  from being created as a result of the INVITE.The BYE request is
  //  used to terminate a specific session or attempted session.In this
  //  case, the specific session is the one with the peer UA on the other
  //  side of the dialog.When a BYE is received on a dialog, any session
  //  associated with that dialog SHOULD terminate.A UA MUST NOT send a
  //  BYE outside of a dialog.The caller's UA MAY send a BYE for either
  //  confirmed or early dialogs, and the callee's UA MAY send a BYE on
  //  confirmed dialogs, but MUST NOT send a BYE on early dialogs.
  VLOG(6) << "TODO: implement rules to send BYE";
  if (m_dialogManager.dialogExists(uiTxId))
  {
    // use dialog to generate BYE
    SipMessage bye = m_dialogManager.getDialog(uiTxId).generateSipRequest(methodToString(BYE));
    m_dialogManager.getDialog(uiTxId).terminateDialog();
    boost::optional<SipUri> pSipUri = SipUri::parse(bye.getToUri());
    if (!pSipUri)
    {
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::generic_category());
    }

    uint32_t uiByeTxId = 0;
    return m_pUac->beginTransaction(uiByeTxId, bye, pSipUri->getHost(), pSipUri->getPort(), m_pUac->getPreferredProtocol());
  }
  else
  {
    return boost::system::error_code(boost::system::errc::no_such_device, boost::system::generic_category());
  }
}

boost::system::error_code UserAgent::ringing(const uint32_t uiTxId)
{
  return m_pUas->ringing(uiTxId);
}

boost::system::error_code UserAgent::accept(const uint32_t uiTxId, const MessageBody& sessionDescription)
{
  return m_pUas->accept(uiTxId, sessionDescription);
}

boost::system::error_code UserAgent::reject(const uint32_t uiTxId)
{
  return m_pUas->reject(uiTxId);
}

void UserAgent::onInvite(const uint32_t uiTxId, const SipMessage& invite)
{
  m_mCallerMap[uiTxId] = false;
  if (m_onInvite)
  {
    m_onInvite(uiTxId, invite);
  }
  else
  {
    LOG(WARNING) << "No INVITE callback set";
  }
}

void UserAgent::onBye(const uint32_t uiTxId, const SipMessage& bye)
{
  if (m_onBye)
  {
    m_onBye(uiTxId, bye);
  }
  else
  {
    LOG(WARNING) << "No BYE callback set";
  }
}

void UserAgent::onInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (m_onInviteResponse)
    m_onInviteResponse(ec, uiTxId, sipResponse);
  else
    LOG(WARNING) << "No INVITE response handler configured";
}

void UserAgent::onByeResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (m_onByeResponse)
    m_onByeResponse(ec, uiTxId, sipResponse);
  else
    LOG(WARNING) << "No BYE response handler configured";
}

void UserAgent::onRegister(const uint32_t uiTxId, const SipMessage& invite, SipMessage& sipResponse)
{
  if (m_onRegister)
  {
    m_onRegister(uiTxId, invite, sipResponse);
  }
  else
  {
    LOG(WARNING) << "No REGISTER callback set";
    sipResponse.setResponseCode(INTERNAL_SERVER_ERROR);
  }
}

void UserAgent::onRegisterResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (m_onRegisterResponse)
    m_onRegisterResponse(ec, uiTxId, sipResponse);
  else
    LOG(WARNING) << "No REGISTER response handler configured";
}

} // rfc3261
} // rtp_plus_plus
