#include "CorePch.h"
#include <rtp++/rfc3261/SipProxy.h>
#include <boost/bind.hpp>
#include <rtp++/rfc3261/SipErrorCategory.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/util/RandomUtil.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

SipProxy::SipProxy(boost::asio::io_service& ioService, const SipContext& sipContext, boost::system::error_code& ec)
  :m_ioService(ioService),
  m_registrar(ioService),
  m_mediaSessionNetworkManager(ioService),
  m_userAgent(ioService, sipContext, ec)
{
  if (ec)
  {
    VLOG(5) << "Error initialising user agent";
    return;
  }
  else
  {
    m_userAgent.setOnInviteHandler(boost::bind(&SipProxy::onInvite, this, _1, _2));
    m_userAgent.setInviteResponseHandler(boost::bind(&SipProxy::onInviteResponse, this, _1, _2, _3));
    m_userAgent.setRegisterHandler(boost::bind(&SipProxy::onRegister, this, _1, _2, _3));
  }
}

boost::system::error_code SipProxy::start()
{
  boost::system::error_code ec;
  ec = m_userAgent.start();
  if (ec)
  {
    LOG(WARNING) << "Error starting UA" << ec.message();
  }

  return ec;
}

boost::system::error_code SipProxy::stop()
{
  VLOG(2) << "SipProxy::stop()";
  boost::system::error_code ec;
  ec = m_userAgent.stop();
  if (ec)
  {
    LOG(WARNING) << "Error stopping UA" << ec.message();
  }

  return ec;
}

void SipProxy::onRegister(const uint32_t uiTxId, const SipMessage& sipRequest, SipMessage& sipResponse)
{
#if 0
  sipResponse.setResponseCode(INTERNAL_SERVER_ERROR);
#else

  bool bDummy;
  boost::optional<std::string> expires = sipRequest.getHeaderField(EXPIRES);
  uint32_t uiExpires = expires ? convert<uint32_t>(*expires, bDummy) : 7200;
  if (m_registrar.registerSipAgent(sipRequest.getToUri(), sipRequest.getFromUri(), sipRequest.getContactUri(), uiExpires))
  {
    VLOG(2) << "AOR registered: " << sipRequest.getToUri();
    sipResponse.setContact(sipRequest.getContact());
    sipResponse.setResponseCode(OK);
  }
  else
  {
    sipResponse.setResponseCode(INTERNAL_SERVER_ERROR);
  }

#endif

  // notify application layer
  if (m_onRegister)
  {
    m_onRegister(uiTxId, sipRequest, sipResponse);
  }
}

void SipProxy::onInvite(const uint32_t uiTxId, const SipMessage& invite)
{
  auto contact = m_registrar.find(invite.getRequestUri());
  if (contact)
  {
    VLOG(2) << "Contact found";
    VLOG(6) << "TODO: only forward invite if max forwards > 0";

    // 1. Reasonable Syntax
    // 2. URI scheme
    // 3. Max - Forwards
    // 4. (Optional)Loop Detection
    // 5. Proxy - Require
    // 6. Proxy - Authorization

    // create a copy of the INVITE and send it to the callee with modifications
    SipMessage copyOfInvite = invite;
    // update request URI
    copyOfInvite.setRequestUri(contact->Contact);
    // update via
    std::ostringstream via;
    via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";branch=z9hG4bK" << random_string(11); // TODO?
    // <<PROTO>> and <<FQDN>> are set by transport layer
    copyOfInvite.insertTopMostVia(via.str());
    // update max forwards
    copyOfInvite.decrementMaxForwards();
    // send with transport layer directly? Or should this be done with UAC?

    boost::optional<SipUri> pSipUri = SipUri::parse(contact->Contact);
    assert(pSipUri); // this MUST have ben validated previously
    m_userAgent.getTransportLayer().sendSipMessage(copyOfInvite, pSipUri->getHost(), pSipUri->getPort(), TP_UDP); 
    VLOG(6) << "TODO: use same protocol as request arrived on?";
  }
  else
  {
    VLOG(2) << "Contact not found";
    m_userAgent.reject(uiTxId);
  }
#if 0
  if (m_onInvite)
  {
    m_onInvite(uiTxId, invite);
  }
  else
  {
    LOG(WARNING) << "No INVITE callback set - rejecting call";
    m_userAgent.reject(uiTxId);
  }
#endif
}

void SipProxy::onInviteResponse(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (!ec)
  {
    ResponseCode eCode = sipResponse.getResponseCode();
    LOG(INFO) << responseCodeString(eCode);
    if (isResponseProvisional(eCode))
    {
    }
    else if (isFinalNonSuccessfulResponse(eCode))
    {
      SipErrorCategory cat;
      boost::system::error_code ec(responseCodeToErrorCode(eCode), cat);
      // m_onSessionEndHandler(ec);
      m_onInviteResponse(ec, uiTxId, sipResponse);
    }
    else if (isResponseSuccessfull(eCode))
    {
      m_onInviteResponse(ec, uiTxId, sipResponse);
    }
  }
  else
  {
    m_onInviteResponse(ec, uiTxId, sipResponse);
  }
}

} // rfc3261
} // rtp_plus_plus
