#include "CorePch.h"
#include <rtp++/rfc3261/StatelessProxy.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/SipUri.h>
#include <rtp++/util/RandomUtil.h>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

StatelessProxy::StatelessProxy(boost::asio::io_service& ioService, MediaSessionNetworkManager& mediaSessionNetworkManager, uint16_t uiSipPort, boost::system::error_code& ec)
  :UserAgentBase(ioService, mediaSessionNetworkManager.getPrimaryInterface(), uiSipPort),
    m_registrar(ioService)
{
}

boost::system::error_code StatelessProxy::doStart()
{
  VLOG(2) << "StatelessProxy::doStart()";
  return boost::system::error_code();
}

boost::system::error_code StatelessProxy::doStop()
{
  VLOG(2) << "StatelessProxy::doStop()";
  return boost::system::error_code();
}

unsigned long hash(const char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

void StatelessProxy::handleRegister(const SipMessage& request, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  boost::optional<SipMessage> response = SipMessage::createResponse(request);
  if (response)
  {
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

    if (!request.hasAttribute(TO, TAG))
    {
      boost::optional<std::string> to = request.getHeaderField(TO);
      std::string sToTag = random_string(4);
      VLOG(6) << "Generated To tag: " << sToTag;
      std::ostringstream toTag;
      toTag << *to << ";tag=" << sToTag;
      response->addHeaderField(TO, toTag.str());
    }
    else
    {
      boost::optional<std::string> to = request.getHeaderField(TO);
      assert(to);
      VLOG(6) << "To tag already exists " << *to;
      response->addHeaderField(TO, *to);
    }

    bool bDummy;
    boost::optional<std::string> expires = request.getHeaderField(EXPIRES);
    uint32_t uiExpires = expires ? convert<uint32_t>(*expires, bDummy) : 7200;
    if (m_registrar.registerSipAgent(request.getToUri(), request.getFromUri(), request.getContactUri(), uiExpires))
    {
      VLOG(2) << "AOR registered: " << request.getToUri();
      response->setContact(request.getContact());
      response->setResponseCode(OK);
    }
    else
    {
      response->setResponseCode(INTERNAL_SERVER_ERROR);
    }
    m_transportLayer.sendSipMessage(*response, sSource, uiPort, eProtocol);
  }
  else
  {
    LOG(WARNING) << "TODO: Malformed REGISTER request received. What response should be sent";
    SipMessage response(BAD_REQUEST);
    m_transportLayer.sendSipMessage(response, sSource, uiPort, eProtocol);
  }
}

void StatelessProxy::doHandleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  if (isValidRequest(sipMessage))
  {
    const SipMessage& request = sipMessage;

    // first handle REGISTER requests
    if (request.getMethod() == REGISTER)
    {
      handleRegister(request, sSource, uiPort, eProtocol);
      return;
    }
    else
    {
#ifdef USE_REQUEST_URI
      // if we use the request URI, we can't handle domain names
      // choose one target
      // for testing we're just going to use the request URI
      boost::optional<SipUri> destination = SipUri::parse(request.getRequestUri());
      assert(destination);
      if (destination)
#else
      auto contact = m_registrar.find(request.getRequestUri());
      if (contact)
#endif
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
        SipMessage copyOfSipRequest = request;
        // update request URI
        copyOfSipRequest.setRequestUri(contact->Contact);

        // compute branch id - not randomly!

        //  The requirement for unique branch IDs across space and time
        //  applies to stateless proxies as well.However, a stateless
        //  proxy cannot simply use a random number generator to compute
        //  the first component of the branch ID, as described in Section
        //  16.6 bullet 8.  This is because retransmissions of a request
        //  need to have the same value, and a stateless proxy cannot tell
        //  a retransmission from the original request.Therefore, the
        //  component of the branch parameter that makes it unique MUST be
        //  the same each time a retransmitted request is forwarded.Thus
        //  for a stateless proxy, the branch parameter MUST be computed as
        //  a combinatoric function of message parameters which are
        //  invariant on retransmission.

        //  The stateless proxy MAY use any technique it likes to guarantee
        //  uniqueness of its branch IDs across transactions.However, the
        //  following procedure is RECOMMENDED.The proxy examines the
        //  branch ID in the topmost Via header field of the received
        //  request.If it begins with the magic cookie, the first
        //  component of the branch ID of the outgoing request is computed
        //  as a hash of the received branch ID.Otherwise, the first
        //  component of the branch ID is computed as a hash of the topmost
        //  Via, the tag in the To header field, the tag in the From header
        //  field, the Call - ID header field, the CSeq number(but not
        //  method), and the Request - URI from the received request.One of
        //  these fields will always vary across two different
        //  transactions.
        unsigned long uiHash;
        boost::optional<std::string> branch = request.getBranchParameter();
        if (branch && boost::algorithm::starts_with(*branch, MAGIC_COOKIE))
        {
          uiHash = hash(branch->c_str());
        }
        else
        {
          // FIXME: constant for now
          uiHash = 1234;
        }

        // insert via
        std::ostringstream via;
        via << "SIP/2.0/<<PROTO>> " << "<<FQDN>>" << ";branch=z9hG4bK" << ::toString(uiHash);
        VLOG(2) << "new VIA branch parameter for proxy: " << via.str();
        copyOfSipRequest.insertTopMostVia(via.str());

#ifndef USE_REQUEST_URI
        boost::optional<SipUri> destination = SipUri::parse(contact->Contact);
        assert(destination); // this MUST have ben validated previously
#endif
        VLOG(6) << "TODO: use same protocol as request arrived on?";
        m_transportLayer.sendSipMessage(copyOfSipRequest, destination->getHost(), destination->getPort(), TP_UDP);
      }
    }
  }
  else if (isValidResponse(sipMessage))
  {
    SipMessage response = sipMessage;
    // remove top most via
    std::string sSentBy = response.getSentByInTopMostVia();
    std::string sTopMostVia = response.removeTopMostVia();
    // make sure the address is our own?
    VLOG(2) << "Top most via: " << sTopMostVia << " sent-by; " << sSentBy;

    // get next hop in route
    std::string sNextSentBy = response.getSentByInTopMostVia();
    std::string sNextVia = response.getTopMostVia();
    VLOG(2) << "Next top most via: " << sNextVia << " sent-by; " << sNextSentBy;

    std::string sDestination;
    uint16_t uiPort = 5060;
    
    size_t pos = sNextSentBy.find(":");
    if (pos != std::string::npos)
    {
      sDestination = sNextSentBy.substr(0, pos);
      bool bDummy;
      uiPort = convert<uint16_t>(sNextSentBy.substr(pos + 1), bDummy);
      assert(bDummy);
    }
    else
    {
      sDestination = sNextSentBy;
    }
    VLOG(2) << "Forwarding response to " << sDestination << ":" << uiPort;
    m_transportLayer.sendSipMessage(response, sDestination, uiPort, eProtocol);
  }
  else
  {
    LOG(WARNING) << "Invalid SIP message: " << sipMessage.toString();
  }
}

} // rfc3261
} // rtp_plus_plus
