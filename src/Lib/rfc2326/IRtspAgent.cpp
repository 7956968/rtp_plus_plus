#include "CorePch.h"
#include <rtp++/rfc2326/IRtspAgent.h>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc2326/Rfc2326.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/RtpInfo.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/RtspUtil.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

IRtspAgent::IRtspAgent()
{

}

IRtspAgent::~IRtspAgent()
{

}

void IRtspAgent::handleRequest(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, 
                               const std::vector<std::string>& vSupported)
{
  assert(rtspRequest.isRequest());
  switch (rtspRequest.getMethod())
  {
    case OPTIONS:
    {
      options(rtspRequest, pConnection, vSupported);
      break;
    }
    case DESCRIBE:
    {
      describe(rtspRequest, pConnection, vSupported);
      break;
    }
    case SETUP:
    {
      setup(rtspRequest, pConnection, vSupported);
      break;
    }
    case PLAY:
    {
      play(rtspRequest, pConnection, vSupported);
      break;
    }
    case PAUSE:
    {
      pause(rtspRequest, pConnection, vSupported);
      break;
    }
    case TEARDOWN:
    {
      teardown(rtspRequest, pConnection, vSupported);
      break;
    }
    case GET_PARAMETER:
    {
      getParameter(rtspRequest, pConnection, vSupported);
      break;
    }
    case SET_PARAMETER:
    {
      setParameter(rtspRequest, pConnection, vSupported);
      break;
    }
    case REGISTER:
    {
      registerRtspServer(rtspRequest, pConnection);
      break;
    }
    default:
    {
      // BAD_REQUEST
      pConnection->write(0, createErrorResponse(BAD_REQUEST, rtspRequest.getCSeq(), vSupported));
      break;
    }
  }
}

void IRtspAgent::handleResponse(const RtspMessage& rtspResponse, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // we are not currently sending requests and therefore do not handle responses
  LOG(WARNING) << "TODO: NOT IMPLEMENTED";
}

void IRtspAgent::options(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  /*
    OPTIONS rtsp://127.0.0.1:554/test.aac RTSP/1.0
    CSeq: 2
    User-Agent: ./openRTSP (LIVE555 Streaming Media v2013.07.16)
   */

  std::set<RtspMethod> methods;
  ResponseCode eCode = handleOptions(methods, vSupported);
  if (methods.empty()) methods.insert(OPTIONS);
  std::ostringstream options;
  auto it = methods.begin();
  std::string sOption = methodToString(*it++);
  options << sOption;

  while (it != methods.end())
  {
    std::string sOption = methodToString(*it++);
    options << "," << sOption;
  }

#if 0
  std::string OPTIONS_RESPONSE="RTSP/1.0 <<Code>> <<ResponseString>>\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "Date: <<Date>>\r\n"\
      "Public: <<Options>>\r\n"\
      "Server: <<Server>>\r\n\r\n";

  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<Code>>", toString(eCode));
  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<ResponseString>>", responseCodeString(eCode));
  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<Options>>", options.str());
  boost::algorithm::replace_first(OPTIONS_RESPONSE, "<<Server>>", getServerInfo());

  pConnection->write(0, OPTIONS_RESPONSE);
#else
  std::ostringstream ostr;

  ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
  ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
  ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
  ostr << "Public: " << options.str() << "\r\n";

  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "Server: " << getServerInfo() << "\r\n";
  ostr << "\r\n";

  pConnection->write(0, ostr.str());
#endif
}

void IRtspAgent::describe(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  VLOG(2) << "Describe for resource " << rtspRequest.getRtspUri();
  if (!checkResource(rtspRequest.getRtspUri()))
  {
    LOG(WARNING) << "No such resource: " << rtspRequest.getRtspUri();
    pConnection->write(0, createErrorResponse(NOT_FOUND, rtspRequest.getCSeq(), vSupported));
    return;
  }

  /*
    DESCRIBE rtsp://127.0.0.1:554/test.aac RTSP/1.0
    CSeq: 3
    User-Agent: ./openRTSP (LIVE555 Streaming Media v2013.07.16)
    Accept: application/sdp
   */
  // check if resource exists

  std::string sSessionDescription;
  std::string sContentType;
  ResponseCode eCode = handleDescribe(rtspRequest, sSessionDescription, sContentType, vSupported);
  if (eCode == OK)
  {
    assert(!sSessionDescription.empty());
#if 0
    std::string DESCRIBE_RESPONSE = "RTSP/1.0 200 OK\r\n"\
        "CSeq: <<CSeq>>\r\n"\
        "Date: <<Date>>\r\n"\
        "Content-Base: <<RtspUri>>\r\n"\
        "Content-Type: <<Content-Type>>\r\n"\
        "Content-Length: <<Content-Length>>\r\n\r\n"\
        "<<MessageBody>>";

    std::string sContentLength = ::toString(sSessionDescription.length());
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<RtspUri>>", rtspRequest.getRtspUri());
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<Content-Type>>", sContentType);
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<Content-Length>>", sContentLength);
    boost::algorithm::replace_first(DESCRIBE_RESPONSE, "<<MessageBody>>", sSessionDescription);
    pConnection->write(0, DESCRIBE_RESPONSE);
#else
    std::ostringstream ostr;

    ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
    ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
    ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
    ostr << "Content-Base: " << rtspRequest.getRtspUri() << "\r\n";
    ostr << "Content-Type: " << sContentType << "\r\n";
    ostr << "Content-Length: " << sSessionDescription.length() << "\r\n";

    if (!vSupported.empty())
    {
      ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
    }

    ostr << "Server: " << getServerInfo() << "\r\n";
    ostr << "\r\n";
    ostr << sSessionDescription;

    pConnection->write(0, ostr.str());
#endif
  }
  else
  {
    // Error
    pConnection->write(0, createErrorResponse(eCode, rtspRequest.getCSeq(), vSupported));
  }
}

void IRtspAgent::setup(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  /*
    SETUP rtsp://127.0.0.1:8554/test.aac/track1 RTSP/1.0
    CSeq: 4
    User-Agent: ./openRTSP (LIVE555 Streaming Media v2013.07.16)
    Transport: RTP/AVP;unicast;client_port=33544-33545
   */

  RtspUri rtspUri(rtspRequest.getRtspUri());
  std::string sSession = rtspRequest.getSession();
  VLOG(2) << "Incoming SETUP for "
          << rtspUri.getRelativePath() << "/"
          << rtspUri.getName() << " Session: " << sSession;

  // check resource
  if (!checkResource(rtspRequest.getRtspUri()))
  {
    pConnection->write(0, createErrorResponse(NOT_FOUND, rtspRequest.getCSeq(), vSupported));
    return;
  }

  boost::optional<std::string> sTransport = rtspRequest.getHeaderField(TRANSPORT);
  if (!sTransport)
  {
    // 400 BAD REQUEST
    pConnection->write(0, createErrorResponse(BAD_REQUEST, rtspRequest.getCSeq(), vSupported));
  }
  else
  {
    std::vector<std::string> vTransports = StringTokenizer::tokenize(*sTransport, ",", true, true);
    boost::optional<Transport> transport = selectTransport(rtspUri, vTransports);
    if (!transport)
    {
      // 461 UNSUPPORTED_TRANSPORT
      pConnection->write(0, createErrorResponse(UNSUPPORTED_TRANSPORT, rtspRequest.getCSeq(), vSupported));
    }
    else
    {
      // update transport with source and destination addresses
      // this is needed in handleSetup

      // TODO: server in RTSP request should be ok but what if name is used in request instead of IP
      transport->setSource(rtspUri.getServerIp());
      transport->setDestination(pConnection->getClientIp());

      // we need to handle two cases here: where the session hasn't been generated yet, and
      // where the request is part of an existing session
      boost::optional<std::string> session = rtspRequest.getHeaderField(SESSION);
      std::string sSession = session ? *session : "";
      ResponseCode eCode = handleSetup(rtspRequest, sSession, *transport, vSupported);

      if (eCode == OK)
      {
        // take the transport info and send back RTSP response
        uint16_t uiClientRtpPort = transport->getClientPortStart();
        uint16_t uiClientRtcpPort = transport->getRtcpMux() ? uiClientRtpPort : uiClientRtpPort + 1;
        uint16_t uiServerRtpPort = transport->getServerPortStart();
        uint16_t uiServerRtcpPort = transport->getRtcpMux() ? uiServerRtpPort : uiServerRtpPort + 1;

        VLOG(2) << "Transport client ports: " << uiClientRtpPort << "-" << uiClientRtcpPort
                << " server ports: " << uiServerRtpPort << "-" << uiServerRtcpPort;

        // check if we're doing MPRTP
        std::ostringstream transportOstr;

        transportOstr << transport->getSpecifier();
        transportOstr << (transport->isUnicast() ? ";unicast" : ";multicast");

        transportOstr << ";destination=" << pConnection->getClientIp()
                      << ";source=" << rtspUri.getServerIp() << ";client_port=" << uiClientRtpPort << "-" << uiClientRtcpPort
                      << ";server_port=" << uiServerRtpPort << "-" << uiServerRtcpPort;

        // TODO: move functionality to subclass so that IRtspAgent does not know about MPRTP?!?
        if (transport->isUsingMpRtp())
        {
          transportOstr << ";src_mprtp_addr=\"" << transport->getSrcMpRtpAddr() << "\"";
          transportOstr << ";dest_mprtp_addr=\"" << transport->getDestMpRtpAddr() << "\"";
        }

        // this handles MPRTP extmap!
        if (transport->hasExtmaps())
        {
          transportOstr << ";extmap=\"" << transport->getExtmaps() << "\"";
        }

        if (transport->getRtcpMux())
        {
          transportOstr << ";" << rfc2326::bis::RTCP_MUX;
        }

#if 0
        std::string SETUP_RESPONSE = "RTSP/1.0 200 OK\r\n"\
          "CSeq: <<CSeq>>\r\n"\
          "Date: <<Date>>\r\n"\
          "Transport: RTP/AVP;unicast;destination=<<Destination>>;source=<<Source>>;client_port=<<ClientRtp>>-<<ClientRtcp>>;server_port=<<ServerRtp>>-<<ServerRtcp>>\r\n"\
          "Session: <<Session>>\r\n\r\n";

        boost::algorithm::replace_first(SETUP_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<RtspUri>>", rtspRequest.getRtspUri());
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<Session>>", sSession);
        // client info
        std::string sDestination = pConnection->getClientIp();
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<Destination>>", sDestination);
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<ClientRtp>>", ::toString(uiClientRtpPort));
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<ClientRtcp>>", ::toString(uiClientRtcpPort));

        // server info
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<Source>>", rtspUri.getServerIp());
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<ServerRtp>>", ::toString(uiServerRtpPort));
        boost::algorithm::replace_first(SETUP_RESPONSE, "<<ServerRtcp>>", ::toString(uiServerRtcpPort));

        pConnection->write(0, SETUP_RESPONSE);

#else
        std::ostringstream ostr;

        ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
        ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
        ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
        ostr << "Transport: " << transportOstr.str() << "\r\n";
        ostr << "Session: " << sSession << "\r\n";
        
        if (!vSupported.empty())
        {
          ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
        }

        ostr << "Server: " << getServerInfo() << "\r\n";
        ostr << "\r\n";
        
        pConnection->write(0, ostr.str());
#endif
      }
      else
      {
        pConnection->write(0, createErrorResponse(eCode, rtspRequest.getCSeq(), vSupported));
      }
    }
  }
}

void IRtspAgent::play(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // check resource
  if (!checkResource(rtspRequest.getRtspUri()))
  {
    pConnection->write(0, createErrorResponse(NOT_FOUND, rtspRequest.getCSeq(), vSupported));
    return;
  }

  RtpInfo rtpInfo(rtspRequest.getRtspUri());
  ResponseCode eCode = handlePlay(rtspRequest, rtpInfo, vSupported);
  if (eCode == OK)
  {
    /*
    const std::string PLAY_RESPONSE="RTSP/1.0 200 OK\r\n"\
        "CSeq: 5\r\n"\
        "Date: Sat, Jul 20 2013 12:30:59 GMT\r\n"\
        "Range: npt=0.000-\r\n"\
        "Session: 37855789\r\n"\
        "RTP-Info: url=rtsp://127.0.0.1:8554/test.aac/track1;seq=16942;rtptime=3178379927\r\n\r\n";
     */
#if 0
    std::string PLAY_RESPONSE="RTSP/1.0 200 OK\r\n"\
        "CSeq: <<CSeq>>\r\n"\
        "Date: <<Date>>\r\n"\
        "Range: npt=now-\r\n"\
        "Session: <<Session>>\r\n"\
        "RTP-Info: <<RtpInfo>>\r\n\r\n";

    boost::algorithm::replace_first(PLAY_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
    boost::algorithm::replace_first(PLAY_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
    boost::algorithm::replace_first(PLAY_RESPONSE, "<<Session>>", rtspRequest.getSession());
    boost::algorithm::replace_first(PLAY_RESPONSE, "<<RtpInfo>>", rtpInfo.toString());

    pConnection->write(0, PLAY_RESPONSE);
#else
    std::ostringstream ostr;

    ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
    ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
    ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
    ostr << "Range: npt=now-\r\n";
    ostr << "Session: " << rtspRequest.getSession() << "\r\n";
    ostr << "RTP-Info: " << rtpInfo.toString() << "\r\n";

    if (!vSupported.empty())
    {
      ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
    }

    ostr << "Server: " << getServerInfo() << "\r\n";
    ostr << "\r\n";

    pConnection->write(0, ostr.str());
#endif
  }
  else
  {
    pConnection->write(0, createErrorResponse(eCode, rtspRequest.getCSeq(), vSupported));
  }
}

void IRtspAgent::pause(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // check resource
  if (!checkResource(rtspRequest.getRtspUri()))
  {
    pConnection->write(0, createErrorResponse(NOT_FOUND, rtspRequest.getCSeq(), vSupported));
    return;
  }

  ResponseCode eCode = handlePause(rtspRequest, vSupported);
#if 0
  std::string PAUSE_RESPONSE="RTSP/1.0 <<Code>> <<ResponseCodeString>>\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "Date: <<Date>>\r\n\r\n";

  boost::algorithm::replace_first(PAUSE_RESPONSE, "<<Code>>", ::toString((uint32_t)eCode));
  boost::algorithm::replace_first(PAUSE_RESPONSE, "<<ResponseCodeString>>", responseCodeString(eCode));
  boost::algorithm::replace_first(PAUSE_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
  boost::algorithm::replace_first(PAUSE_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
  pConnection->write(0, PAUSE_RESPONSE);
#else
  std::ostringstream ostr;

  ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
  ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
  ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
  ostr << "Session: " << rtspRequest.getSession() << "\r\n";

  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "Server: " << getServerInfo() << "\r\n";
  ostr << "\r\n";

  pConnection->write(0, ostr.str());
#endif
}

void IRtspAgent::teardown(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // check resource
  if (!checkResource(rtspRequest.getRtspUri()))
  {
    pConnection->write(0, createErrorResponse(NOT_FOUND, rtspRequest.getCSeq(), vSupported));
    return;
  }

  ResponseCode eCode = handleTeardown(rtspRequest, vSupported);

#if 0
  std::string TEARDOWN_RESPONSE="RTSP/1.0 <<Code>> <<ResponseCodeString>>\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "Date: <<Date>>\r\n\r\n";

  boost::algorithm::replace_first(TEARDOWN_RESPONSE, "<<Code>>", ::toString((uint32_t)eCode));
  boost::algorithm::replace_first(TEARDOWN_RESPONSE, "<<ResponseCodeString>>", responseCodeString(eCode));
  boost::algorithm::replace_first(TEARDOWN_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
  boost::algorithm::replace_first(TEARDOWN_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
  pConnection->write(0, TEARDOWN_RESPONSE);
#else
  std::ostringstream ostr;

  ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
  ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
  ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
  ostr << "Session: " << rtspRequest.getSession() << "\r\n";

  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "Server: " << getServerInfo() << "\r\n";
  ostr << "\r\n";

  pConnection->write(0, ostr.str());
#endif

}

void IRtspAgent::getParameter(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  std::string sContentType;
  std::string sContent;
  ResponseCode eCode = handleGetParameter(rtspRequest, sContentType, sContent, vSupported);
  if (eCode == OK)
  {
#if 0
    std::string GET_PARAMETER_RESPONSE = "RTSP/1.0 200 OK\r\n"\
        "CSeq: <<CSeq>>\r\n"\
        "Date: <<Date>>\r\n"\
        "Content-Base: <<RtspUri>>\r\n"\
        "Content-Type: <<Content-Type>>\r\n"\
        "Content-Length: <<Content-Length>>\r\n\r\n"\
        "<<MessageBody>>";

    std::string sContentLength = ::toString(sContent.length());
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<CSeq>>", rtspRequest.getCSeq());
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<RtspUri>>", rtspRequest.getRtspUri());
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<Content-Type>>", sContentType);
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<Content-Length>>", sContentLength);
    boost::algorithm::replace_first(GET_PARAMETER_RESPONSE, "<<MessageBody>>", sContent);

    pConnection->write(0, GET_PARAMETER_RESPONSE);
#else
    std::ostringstream ostr;

    ostr << "RTSP/1.0 " << toString(eCode) << " " << responseCodeString(eCode) << "\r\n";
    ostr << "CSeq: " << rtspRequest.getCSeq() << "\r\n";
    ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";
    ostr << "Content-Type: " << sContentType << "\r\n";
    ostr << "Content-Length: " << sContent.length() << "\r\n";
    ostr << "Session: " << rtspRequest.getSession() << "\r\n";

    if (!vSupported.empty())
    {
      ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
    }

    ostr << "Server: " << getServerInfo() << "\r\n";
    ostr << "\r\n";
    ostr << sContent;
    pConnection->write(0, ostr.str());
#endif

  }
  else
  {
    pConnection->write(0, createErrorResponse(eCode, rtspRequest.getCSeq(), vSupported));
  }
}

void IRtspAgent::setParameter(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  // TODO: call handleSetParameter
}

void IRtspAgent::registerRtspServer(const RtspMessage& rtspRequest, RtspServerConnectionPtr pConnection)
{
  if (!supportsRegisterRequest())
  {
    // 400 BAD REQUEST
    pConnection->write(0, createErrorResponse(BAD_REQUEST, rtspRequest.getCSeq()));
  }
  else
  {
    pConnection->write(0, createErrorResponse(OK, rtspRequest.getCSeq()));
  }
}

ResponseCode IRtspAgent::handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const
{
  methods.insert(OPTIONS);
  return OK;
}

ResponseCode IRtspAgent::handleDescribe(const RtspMessage& describe, std::string& sSessionDescription,
                                        std::string& sContentType, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handleSetup(const RtspMessage& setup, std::string& sSession,
                                     Transport& transport, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handlePause(const RtspMessage& pause, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handleGetParameter(const RtspMessage& getParameter, std::string& sContentType, std::string& sContent, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

ResponseCode IRtspAgent::handleSetParameter(const RtspMessage& setParameter, const std::vector<std::string>& vSupported)
{
  return NOT_IMPLEMENTED;
}

bool IRtspAgent::checkResource(const std::string& sResource)
{
  return false;
}

boost::optional<Transport> IRtspAgent::selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports)
{
  return boost::optional<Transport>();
}

bool IRtspAgent::supportsRegisterRequest() const
{
  return false;
}

} // rfc2326
} // rtp_plus_plus


