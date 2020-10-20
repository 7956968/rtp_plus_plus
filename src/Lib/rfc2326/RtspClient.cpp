#include "CorePch.h"
#include <rtp++/rfc2326/RtspClient.h>
#include <iomanip>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/RtspErrorCategory.h>

using namespace std;
using boost::optional;

namespace rtp_plus_plus
{
namespace rfc2326
{

// RTSP session timeout
const static uint32_t DEFAULT_RTSP_SESSION_TIMEOUT_SECONDS = 60;

// minimum length of session identifier
const static uint32_t MIN_SESSION_LENGTH = 8;


RtspClient::RtspClient(boost::asio::io_service& ioService, const std::string& sRtspUri, const std::string& sIp, const uint16_t uiPort)
  :m_ioService(ioService),
  m_bShuttingDown(false),
  m_sIp(sIp),
  m_uiPort(uiPort),
  m_rtspUri(sRtspUri),
  m_uiSessionTimeout(DEFAULT_RTSP_SESSION_TIMEOUT_SECONDS),
  m_uiSN(1)
{
}

void RtspClient::initialiseConnection()
{
  VLOG(2) << "Initialising new connection to server " << m_sIp << ":" << m_uiPort;
  m_pConnection = RtspClientConnection::create(m_ioService, m_sIp, m_uiPort);
  // setup connection callbacks
  m_pConnection->setReadCompletionHandler(boost::bind(&RtspClient::readCompletionHandler, this, boost::asio::placeholders::error, _2, _3));
  m_pConnection->setWriteCompletionHandler(boost::bind(&RtspClient::writeCompletionHandler, this, boost::asio::placeholders::error, _2, _3, _4));
  m_pConnection->setCloseHandler(boost::bind(&RtspClient::closeCompletionHandler, this, _1));
  m_pConnection->setConnectErrorHandler(boost::bind(&RtspClient::connectErrorHandler, this, boost::asio::placeholders::error, _2));
}

void RtspClient::connectErrorHandler(const boost::system::error_code& error, RtspClientConnectionPtr pConnection )
{
  // this handler could be triggered eg if no connection could be created
  notifyCallerOfError(error);
}

void RtspClient::shutdown()
{
  m_bShuttingDown = true;
  if (m_pConnection)
  {
    // close any outstanding reads
    VLOG(5) << "Shutting down RTSP client. Closing connection";
    m_pConnection->close();
    m_pConnection.reset();
  }
}

boost::system::error_code RtspClient::options(const std::vector<std::string>& vSupported)
{
  if (!m_optionsHandler)
  {
    LOG(WARNING) << "No options handler has been configured";
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }

  // check for outstanding requests: we only handle one at a time
  if (!m_qOutstandingRequests.empty())
  {
    return boost::system::error_code(boost::system::errc::resource_unavailable_try_again, boost::system::get_generic_category());
  }

#if 0
  std::string sOptionsRequest="OPTIONS <<RtspUri>> RTSP/1.0\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "User-Agent: <<UserAgent>>\r\n\r\n";

  boost::algorithm::replace_first(sOptionsRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sOptionsRequest, "<<CSeq>>", ::toString(m_uiSN));
  boost::algorithm::replace_first(sOptionsRequest, "<<UserAgent>>", getClientInfo());
  return sendRequest(OPTIONS, sOptionsRequest);
#else
  std::ostringstream ostr;

  ostr << "OPTIONS " << m_rtspUri.fullUri() << " RTSP/1.0\r\n";
  ostr << "CSeq: " << m_uiSN << "\r\n";
  ostr << "User-Agent: " << getClientInfo() << "\r\n";

  if (!vSupported.empty())
  { 
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "\r\n";
  return sendRequest(OPTIONS, ostr.str());
#endif
}

boost::system::error_code RtspClient::sendRequest(RtspMethod eMethod, const std::string& sRtspRequest)
{
  m_mRequestMap[m_uiSN] = std::make_pair(eMethod, sRtspRequest);
  m_qOutstandingRequests.push_back(eMethod);

  if (!m_pConnection || m_pConnection->eof())
  {
    initialiseConnection();
  }

  m_pConnection->write(m_uiSN, sRtspRequest);
  ++m_uiSN;
  return boost::system::error_code();
}

boost::system::error_code RtspClient::describe(const std::vector<std::string>& vSupported)
{
  if (!m_describeHandler)
  {
    LOG(WARNING) << "No describe handler has been configured";
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }

  // check for outstanding requests: we only handle one at a time
  if (!m_qOutstandingRequests.empty())
  {
    return boost::system::error_code(boost::system::errc::resource_unavailable_try_again, boost::system::get_generic_category());
  }

#if 0
  std::string sDescribeRequest="DESCRIBE <<RtspUri>> RTSP/1.0\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "User-Agent: <<UserAgent>>\r\n"\
      "Accept: application/sdp\r\n\r\n";

  boost::algorithm::replace_first(sDescribeRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sDescribeRequest, "<<CSeq>>", ::toString(m_uiSN));
  boost::algorithm::replace_first(sDescribeRequest, "<<UserAgent>>", getClientInfo());

  return sendRequest(DESCRIBE, sDescribeRequest);
#else
  std::ostringstream ostr;

  ostr << "DESCRIBE " << m_rtspUri.fullUri() << " RTSP/1.0\r\n";
  ostr << "CSeq: " << m_uiSN << "\r\n";
  ostr << "User-Agent: " << getClientInfo() << "\r\n";
  ostr << "Accept: application/sdp\r\n";

  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "\r\n";
  return sendRequest(DESCRIBE, ostr.str());
#endif
}

boost::system::error_code RtspClient::setup(const rfc4566::SessionDescription& sessionDescription, const rfc4566::MediaDescription& media, const Transport& transport, const std::vector<std::string>& vSupported)
{
  if (!m_setupHandler)
  {
    LOG(WARNING) << "No SETUP handler has been configured";
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }

  // check for outstanding requests: we only handle one at a time
  if (!m_qOutstandingRequests.empty())
  {
    return boost::system::error_code(boost::system::errc::resource_unavailable_try_again, boost::system::get_generic_category());
  }

  // TrackId
  optional<std::string> trackId = media.getAttributeValue(rfc4566::CONTROL);

  std::string sTrackId = trackId? *trackId : "";
  if (sTrackId.empty())
  {
    // This shouldn't happen?!?
    LOG(WARNING) << "No control for track!";
  }

  // Transport
  // RTP/AVP;unicast;client_port=35220-35221
  std::string sTransport = transport.toString();

#if 0
  std::string sSetupRequest;
  if (m_sSession.empty())
  {
    sSetupRequest="SETUP <<RtspUri>>/<<TrackId>> RTSP/1.0\r\n"\
        "CSeq: <<CSeq>>\r\n"\
        "User-Agent: <<UserAgent>>\r\n"\
        "Transport: <<Transport>>\r\n\r\n";
  }
  else
  {
    sSetupRequest="SETUP <<RtspUri>>/<<TrackId>> RTSP/1.0\r\n"\
        "CSeq: <<CSeq>>\r\n"\
        "Session: <<Session>>\r\n"\
        "User-Agent: <<UserAgent>>\r\n"\
        "Transport: <<Transport>>\r\n\r\n";
    boost::algorithm::replace_first(sSetupRequest, "<<Session>>", m_sSession);
  }

  boost::algorithm::replace_first(sSetupRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sSetupRequest, "<<TrackId>>", sTrackId);
  boost::algorithm::replace_first(sSetupRequest, "<<CSeq>>", ::toString(m_uiSN));
  boost::algorithm::replace_first(sSetupRequest, "<<UserAgent>>", getClientInfo());
  boost::algorithm::replace_first(sSetupRequest, "<<Transport>>", sTransport);

  return sendRequest(SETUP, sSetupRequest);
#else
  std::ostringstream ostr;

  ostr << "SETUP " << m_rtspUri.fullUri() << "/" << sTrackId << " RTSP/1.0\r\n";
  ostr << "CSeq: " << m_uiSN << "\r\n";
  if (!m_sSession.empty())
    ostr << "Session: " << m_sSession << "\r\n";

  ostr << "User-Agent: " << getClientInfo() << "\r\n";
  ostr << "Transport: " << sTransport << "\r\n";

  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "\r\n";
  return sendRequest(SETUP, ostr.str());
#endif
}

boost::system::error_code RtspClient::play(double dStart, double dEnd, const std::vector<std::string>& vSupported)
{
  if (!m_playHandler)
  {
    LOG(WARNING) << "No PLAY handler has been configured";
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }

  // check for outstanding requests: we only handle one at a time
  if (!m_qOutstandingRequests.empty())
  {
    return boost::system::error_code(boost::system::errc::resource_unavailable_try_again, boost::system::get_generic_category());
  }

  std::stringstream start;
  start << std::setprecision(3) << dStart;

  std::string sStop = "";
  if (dEnd != -1.0)
  {
    std::stringstream stop;
    stop << std::setprecision(3) << dEnd;
    sStop = stop.str();
  }

#if 0
  std::string sPlayRequest = "PLAY <<RtspUri>> RTSP/1.0\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "User-Agent: <<UserAgent>>\r\n"\
      "Session: <<Session>>\r\n"\
      "Range: npt=<<StartDouble>>-<<StopDouble>>\r\n\r\n";

  boost::algorithm::replace_first(sPlayRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sPlayRequest, "<<CSeq>>", ::toString(m_uiSN));
  boost::algorithm::replace_first(sPlayRequest, "<<UserAgent>>", getClientInfo());
  boost::algorithm::replace_first(sPlayRequest, "<<Session>>", m_sSession);
  boost::algorithm::replace_first(sPlayRequest, "<<StartDouble>>", start.str());
  boost::algorithm::replace_first(sPlayRequest, "<<StopDouble>>", sStop);

  return sendRequest(PLAY, sPlayRequest);
#else
  std::ostringstream ostr;

  ostr << "PLAY " << m_rtspUri.fullUri() << " RTSP/1.0\r\n";
  ostr << "CSeq: " << m_uiSN << "\r\n";
  ostr << "Session: " << m_sSession << "\r\n";
  ostr << "User-Agent: " << getClientInfo() << "\r\n";
  ostr << "Range: npt=" << start.str() << "-" << sStop << "\r\n";
  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "\r\n";
  return sendRequest(PLAY, ostr.str());
#endif

}

boost::system::error_code RtspClient::teardown(const std::vector<std::string>& vSupported)
{
  if (!m_teardownHandler)
  {
    LOG(WARNING) << "No TEARDOWN handler has been configured";
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }

  // check for outstanding requests: we only handle one at a time
  if (!m_qOutstandingRequests.empty())
  {
    return boost::system::error_code(boost::system::errc::resource_unavailable_try_again, boost::system::get_generic_category());
  }

#if 0
  std::string sTeardownRequest = "TEARDOWN <<RtspUri>> RTSP/1.0\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "User-Agent: <<UserAgent>>\r\n"\
      "Session: <<Session>>\r\n\r\n";

  boost::algorithm::replace_first(sTeardownRequest, "<<RtspUri>>", m_rtspUri.fullUri());
  boost::algorithm::replace_first(sTeardownRequest, "<<CSeq>>", ::toString(m_uiSN));
  boost::algorithm::replace_first(sTeardownRequest, "<<UserAgent>>", getClientInfo());
  boost::algorithm::replace_first(sTeardownRequest, "<<Session>>", m_sSession);

  return sendRequest(TEARDOWN, sTeardownRequest);
#else
  std::ostringstream ostr;

  ostr << "TEARDOWN " << m_rtspUri.fullUri() << " RTSP/1.0\r\n";
  ostr << "CSeq: " << m_uiSN << "\r\n";
  ostr << "Session: " << m_sSession << "\r\n";
  ostr << "User-Agent: " << getClientInfo() << "\r\n";
  if (!vSupported.empty())
  {
    ostr << "Supported: " << ::toString(vSupported, ',') << "\r\n";
  }

  ostr << "\r\n";
  return sendRequest(TEARDOWN, ostr.str());
#endif
}

void RtspClient::closeCompletionHandler( RtspClientConnectionPtr pConnection )
{
  VLOG(5) << "Close completion handler called - resetting connection";
  m_pConnection.reset();
}

void RtspClient::readCompletionHandler( const boost::system::error_code& error, const std::string& sRtspMessage, RtspClientConnectionPtr pConnection )
{
  if (!error)
  {
    // check if this is RTSP or RTP
    if (sRtspMessage[0] == 0x24)
    {
      VLOG(2) << "Read RTP packet (" << sRtspMessage.length() << ")";
      // TODO:
    }
    else
    {
      VLOG(2) << "Read RTSP message (" << sRtspMessage.length() << "):\r\n" << sRtspMessage;
      boost::optional<RtspMessage> pRtspMessage = RtspMessage::parse(sRtspMessage);
      if (!pRtspMessage)
      {
        // failed to parse message
        LOG(WARNING) << "Failed to parse incoming RTSP message (" << sRtspMessage.length() << "):\r\n" << sRtspMessage;
        notifyCallerOfError(boost::system::error_code(boost::system::errc::protocol_error, boost::system::get_generic_category()));
        return;
      }
      else
      {
        std::vector<std::string> vSupported;
        boost::optional<std::string> supported = pRtspMessage->getHeaderField(SUPPORTED);
        if (supported)
        {
          vSupported = StringTokenizer::tokenize(*supported, ",", true, true);
        }

        if (pRtspMessage->isRequest())
        {
          // request from server
          handleRequest(*pRtspMessage, pConnection, vSupported);
        }
        else
        {
          // response from server
          handleResponse(*pRtspMessage, pConnection, vSupported);
        }
      }
    }
  }
  else
  {
    if (m_bShuttingDown)
    {
      if (( error != boost::asio::error::operation_aborted ) &&
          ( error != boost::asio::error::connection_aborted ))
        VLOG(5) << "Error in read: " << error.message();
    }
    else
    {
      notifyCallerOfError(error);
    }

    // TODO: double check if we should close the connection here.
  }
}

void RtspClient::writeCompletionHandler( const boost::system::error_code& error, uint32_t uiMessageId, const std::string& sRtspMessage, RtspClientConnectionPtr pConnection )
{
  if (!error)
  {
    VLOG(2) << "[C->S] Wrote RTSP message (" << sRtspMessage.length() << "):\r\n" << sRtspMessage;
  }
  else
  {
    // TODO: once the connection has failed all bets are off: should we trigger the handler for all outstanding requests
    LOG(WARNING) << "Error in write: " << error.message();
    notifyCallerOfError(error);
  }
}

void RtspClient::notifyCallerOfError(const boost::system::error_code& error)
{
  // get first outstanding request in request queue
  if (!m_qOutstandingRequests.empty())
  {
    RtspMethod eMethod = m_qOutstandingRequests.front();
    m_qOutstandingRequests.pop_front();

    switch (eMethod)
    {
      case OPTIONS:
      {
        m_optionsHandler(error, std::vector<std::string>(), std::vector<std::string>());
        break;
      }
      case DESCRIBE:
      {
        m_describeHandler(error, MessageBody(), std::vector<std::string>());
        break;
      }
      case SETUP:
      {
        Transport transportDummy;
        m_setupHandler(error, "", transportDummy, std::vector<std::string>());
        break;
      }
      case PLAY:
      {
        m_playHandler(error, std::vector<std::string>());
        break;
      }
      case PAUSE:
      {
        m_pauseHandler(error, std::vector<std::string>());
        break;
      }
      case TEARDOWN:
      {
        m_teardownHandler(error, std::vector<std::string>());
        break;
      }
      case GET_PARAMETER:
      {
        m_getParameterHandler(error, std::vector<std::string>());
        break;
      }
      case SET_PARAMETER:
      {
        m_setParameterHandler(error, std::vector<std::string>());
        break;
      }
      default:
      {
        // TODO:
        assert(false);
        break;
      }
    }
  }
  else
  {
    // could have been an error in an incoming request or could be on application exit
    if (error != boost::asio::error::eof && 
        error != boost::asio::error::operation_aborted && 
        error != boost::asio::error::connection_aborted)
      LOG(WARNING) << "Error: " << error.message();
    // TODO: do we need to ignore bad file descriptor
  }
}

void RtspClient::handleRequest(const RtspMessage& rtspRequest, RtspClientConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  assert(rtspRequest.isRequest());
  switch (rtspRequest.getMethod())
  {
    // TODO:
    //    case OPTIONS:
    //    {
    //      options(rtspRequest, pConnection);
    //      break;
    //    }
    //    case DESCRIBE:
    //    {
    //      describe(rtspRequest, pConnection);
    //      break;
    //    }
    //    case SETUP:
    //    {
    //      setup(rtspRequest, pConnection);
    //      break;
    //    }
    //    case PLAY:
    //    {
    //      play(rtspRequest, pConnection);
    //      break;
    //    }
    //    case PAUSE:
    //    {
    //      pause(rtspRequest, pConnection);
    //      break;
    //    }
    //    case TEARDOWN:
    //    {
    //      teardown(rtspRequest, pConnection);
    //      break;
    //    }
    //    case GET_PARAMETER:
    //    {
    //      getParameter(rtspRequest, pConnection);
    //      break;
    //    }
    //    case SET_PARAMETER:
    //    {
    //      setParameter(rtspRequest, pConnection);
    //      break;
    //    }
    default:
    {
      // TODO: BAD_REQUEST?!?
      // pConnection->write(0, createErrorResponse(BAD_REQUEST, rtspRequest.getCSeq()));
      break;
    }
  }
}

void RtspClient::handleResponse(const RtspMessage& rtspResponse, RtspClientConnectionPtr pConnection, const std::vector<std::string>& vSupported)
{
  bool bDummy;
  uint32_t uiSN = convert<uint32_t>(rtspResponse.getCSeq(), bDummy);
  assert(bDummy);
  auto it = m_mRequestMap.find(uiSN);
  if (it != m_mRequestMap.end())
  {
    RtspErrorCategory cat;
    boost::system::error_code ec(responseCodeToErrorCode(rtspResponse.getResponseCode()), cat);

    // get Supported header info
    vector<string> vSupported;
    optional<string> supported = rtspResponse.getHeaderField(SUPPORTED);
    if (supported)
    {
      vSupported = StringTokenizer::tokenize(*supported, ",", true, true);
    }

    // pop outstanding request
    assert(!m_qOutstandingRequests.empty());
    m_qOutstandingRequests.pop_front();
    // locate request and request handler
    switch (it->second.first)
    {
      case OPTIONS:
      {
        vector<string> vOptions;
        if (!ec)
        {
          optional<string> options = rtspResponse.getHeaderField(PUBLIC);
          if (options)
          {
            vOptions = StringTokenizer::tokenize(*options, ",", true, true);
          }
        }
        m_optionsHandler(ec, vOptions, vSupported);
        break;
      }
      case DESCRIBE:
      {
        m_describeHandler(ec, rtspResponse.getMessageBody(), vSupported);
        break;
      }
      case SETUP:
      {
        if (!ec)
        {
          // get session header
          optional<string> session = rtspResponse.getHeaderField(SESSION);
          optional<string> transportHeader = rtspResponse.getHeaderField(TRANSPORT);

          
          if (session)
          {
            const std::string sTimeout(";timeout=");
            size_t posTimeout = session->find(sTimeout);
            if (posTimeout != std::string::npos)
            {
              m_sSession = session->substr(0, posTimeout);
              assert(m_sSession.size() >= MIN_SESSION_LENGTH);

              bool bSuccess;
              m_uiSessionTimeout = convert<uint32_t>(session->substr(posTimeout + sTimeout.length()), bSuccess);
              if (!bSuccess)
              {
                LOG(WARNING) << "Failed to parse RTSP Session header: " << *session;
              }
              else
              {
                VLOG(2) << "RTSP session timeout " << m_uiSessionTimeout << "s";
              }
            }
            else
            {
              m_sSession = *session;
            }
          }
          else
          {
            LOG(WARNING) << "Setup contains no Session header";
            m_sSession = "";
          }
          if (m_sSession.empty())
          {
            LOG(WARNING) << "Setup success but empty Session!";
            m_setupHandler(ec, "", Transport(), vSupported);
          }

          if (!transportHeader)
          {
            LOG(WARNING) << "Setup success but empty Transport!";
            m_setupHandler(ec, "", Transport(), vSupported);
          }
          else
          {
            VLOG(5) << "Setup with transport: " << *transportHeader;
          }
          // get transport header
          Transport transport(*transportHeader);
          m_setupHandler(ec, m_sSession, transport, vSupported);
        }
        else
        {
          m_setupHandler(ec, "", Transport(), vSupported);
        }
        break;
      }
      case PLAY:
      {
        m_playHandler(ec, vSupported);
        break;
      }
      case TEARDOWN:
      {
        m_sSession = "";
        m_teardownHandler(ec, vSupported);
        break;
      }
      default:
      {
        LOG(WARNING) << "Unhandled method: " << methodToString(it->second.first);
      }
    }
  }
  else
  {
    LOG(WARNING) << "Unable to find original request!";
  }
}

} // rfc2326
} // rtp_plus_plus
