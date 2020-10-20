#include "CorePch.h"
#include <rtp++/rfc2326/RtspMessage.h>
#include <limits>
#include <boost/regex.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/FileUtil.h>
#include <rtp++/rfc2326/HeaderFields.h>
#include <rtp++/rfc2326/MessageBody.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

boost::optional<RtspMessage> RtspMessage::create(const std::string& sMethodOrResponseCode,
                                                 const std::string& sRtspUriOrResponseDescription,
                                                 const std::string& sVersion)
{
  RtspMethod eMethod = stringToMethod(sMethodOrResponseCode);
  if (eMethod != UNKNOWN)
  {
    return boost::optional<RtspMessage>(RtspMessage(sMethodOrResponseCode, sRtspUriOrResponseDescription, sVersion));
  }
  else
  {
    // try response
    ResponseCode eCode = stringToResponseCode(sMethodOrResponseCode);
    std::string sResponse = responseCodeString(eCode);
    // round-about way of checking which could fail if string is wrong...
    if (sResponse == sRtspUriOrResponseDescription)
    {
      RtspMessage rtspMessage;
      rtspMessage.setResponse(eCode);
      return boost::optional<RtspMessage>(rtspMessage);
    }
  }
  return boost::optional<RtspMessage>();
}

RtspMessage::RtspMessage()
  :m_sVersion("1.0"),
    m_eType(RTSP_NOT_SET),
    m_eMethod(UNKNOWN),
    m_eCode(NOT_SET)
{

}

RtspMessage::RtspMessage (const std::string& sMethod, const std::string& sRtspUri,
                          const std::string& sRtspVersion)
  :m_sVersion(sRtspVersion),
    m_eType(RTSP_REQUEST),
    m_eMethod(stringToMethod(sMethod)),
    m_sRtspUri(sRtspUri),
    m_eCode(NOT_SET)
{

}

std::string RtspMessage::getCSeq() const
{
  //  extract CSeq and use in response
  boost::optional<std::string> CSeq = getHeaderField(CSEQ);
  // there's a bug in the wowza server
  if (!CSeq)
  {
    const std::string CSEQ_WOWZA("Cseq");
    CSeq = getHeaderField(CSEQ_WOWZA);
    if (CSeq)
    {
      LOG(WARNING) << "Bug in Wowza CSeq header casing!";
    }
  }
  return (CSeq) ? *CSeq : "";
}

std::string RtspMessage::getSession() const
{
  //  extract CSeq and use in response
  boost::optional<std::string> Session = getHeaderField(SESSION);
  return (Session) ? *Session : "";
}

boost::optional<std::string> RtspMessage::getHeaderField(const std::string& sName) const
{
  auto it = m_mHeaderMap.find(sName);
  if (it != m_mHeaderMap.end())
  {
    return boost::optional<std::string>(it->second);
  }
  return boost::optional<std::string>();
}

// changes the data structure to be a response
void RtspMessage::setResponse(ResponseCode eCode)
{
  m_eType = RTSP_RESPONSE;
  m_eCode = eCode;
}

boost::optional<RtspMessage> RtspMessage::parse(const std::string& sRtspMessage)
{
#define DEBUG_INCOMING_RTSP
#ifdef DEBUG_INCOMING_RTSP
  static int fileNo = 0;
  std::ostringstream outFile;
  outFile << "rtsp_" << fileNo++ << ".log";
  FileUtil::writeFile(outFile.str().c_str(), sRtspMessage, false);
#endif

  boost::regex rtspUri_regex("(OPTIONS|DESCRIBE|SETUP|PLAY|PAUSE|TEARDOWN|GET_PARAMETER|SET_PARAMETER|REGISTER) (rtsp://[^ ]+) RTSP/([0-9].[0-9])");

  std::istringstream istr(sRtspMessage);
  std::string sLine;
  if (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, rtspUri_regex))
    {
      std::string sMethod = match[1];
      std::string sRtspUri = match[2];
      std::string sRtspVersion = match[3];
      VLOG(15) << "Match RTSP request. Version: " << sRtspVersion << " Method: " << sMethod << " URI: " << sRtspUri;
      RtspMessage rtspMessage(sMethod, sRtspUri, sRtspVersion);
      if (rtspMessage.getMethod() != UNKNOWN)
      {
        // read remaining header fields
        while (std::getline(istr, sLine))
        {
          size_t pos = sLine.find_first_of(':');
          if (pos == std::string::npos)
          {
            break;
          }
          std::string sValue = sLine.substr(pos + 1);
          boost::algorithm::trim(sValue);
          rtspMessage.m_mHeaderMap[sLine.substr(0, pos)] = sValue;
        }
        return boost::optional<RtspMessage>(rtspMessage);
      }
      else
      {
        LOG(WARNING) << "Unknown RTSP method: " << rtspMessage.getMethod();
        return boost::optional<RtspMessage>();
      }
    }
    else
    {
      VLOG(15) << "Didn't match request - trying to match response.";
      // try to match response header
      boost::regex rtspResponse_regex("RTSP/([0-9].[0-9]) ([0-9][0-9][0-9]) (\\w*)");
      //        if (boost::algorithm::starts_with(sLine, "RTSP/1.0 "))
      if (boost::regex_search(sLine, match, rtspResponse_regex))
      {
        std::string sResponseCode = match[2];
        std::string sResponseMessage = match[3];
        VLOG(10) << "Matched RTSP response. Code: " << sResponseCode << " Message: " << sResponseMessage;

        ResponseCode eCode = stringToResponseCode(sResponseCode);
        RtspMessage rtspMessage;
        rtspMessage.setResponse(eCode);

        // read remaining header fields
        while (std::getline(istr, sLine))
        {
          size_t pos = sLine.find_first_of(':');

          if (pos == std::string::npos)
          {
            std::ostringstream ostr;
            while (std::getline(istr, sLine))
            {
              ostr << sLine << "\n";
            }
            std::string sMessageBody = ostr.str();
            if (!sMessageBody.empty())
            {
              MessageBody messageBody(sMessageBody);
              rtspMessage.setMessageBody(messageBody);
            }
            break;
          }
          // assert(pos != std::string::npos);

          std::string sValue = sLine.substr(pos + 1);
          boost::algorithm::trim(sValue);
          rtspMessage.m_mHeaderMap[sLine.substr(0, pos)] = sValue;
        }

        return boost::optional<RtspMessage>(rtspMessage);
      }
      else
      {
        LOG(WARNING) << "Parsing RTSP: Failed to match request or response";
        return boost::optional<RtspMessage>();
      }
    }
  }
  else
  {
    LOG(WARNING) << "Parsing RTSP: Failed to getline";
    return boost::optional<RtspMessage>();
  }
}

} // rfc2326
} // rtp_plus_plus
