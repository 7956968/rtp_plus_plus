#include "CorePch.h"
#include <rtp++/rfc2326/RtspUtil.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

bool extractRtspInfo(const std::string& sRtspMessage, std::string& sRequest,
                     std::string& sRtspUri, std::string& sRtspVersion)
{
  boost::regex rtspUri_regex("(OPTIONS|DESCRIBE|SETUP|PLAY|PAUSE|TEARDOWN|GET_PARAMETER|SET_PARAMETER) (rtsp://[^ ]+) RTSP/(1.0)");

  std::istringstream istr(sRtspMessage);
  std::string sLine;
  if (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, rtspUri_regex))
    {
//      std::cout << "Matched: \"" << match[0] << " and " << match[1] << " and " << match[2] << "\n";
      sRequest = match[1];
      sRtspUri = match[2];
      sRtspVersion = match[3];
      return true;
    }
  }
  LOG(INFO) << "No match";
  return false;
}

bool getCSeq(const std::string& sRtspMessage, uint32_t& uiCSeq)
{
  boost::regex cseq_regex("(CSeq: )(\\d+)");

  std::istringstream istr(sRtspMessage);
  std::string sLine;
  while (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, cseq_regex))
    {
//      std::cout << "Matched: \"" << match[0] << " and " << match[1] << " and " << match[2] << "\n";
      bool bDummy;
      uiCSeq = convert<uint32_t>(match[2], bDummy);
      assert(bDummy);
      return true;
    }
  }
  LOG(INFO) << "No match";
  return false;
}

bool getCSeqString(const std::string& sRtspMessage, std::string& sCSeq)
{
  boost::regex cseq_regex("(CSeq: )(\\d+)");

  std::istringstream istr(sRtspMessage);
  std::string sLine;
  while (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, cseq_regex))
    {
//      std::cout << "Matched: \"" << match[0] << " and " << match[1] << " and " << match[2] << "\n";
      sCSeq = match[2];
      return true;
    }
  }
  LOG(INFO) << "No match";
  return false;
}

bool getContentLength(const std::string& sRtspMessage, uint32_t& uiContentLength)
{
  boost::regex content_length_regex("(Content-Length: )(\\d+)");
  std::istringstream istr(sRtspMessage);
  std::string sLine;
  while (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, content_length_regex))
    {
      bool bDummy;
      uiContentLength = convert<uint32_t>(match[2], bDummy);
      assert(bDummy);
      return true;
    }
  }
  return false;
}

bool getContentLengthString(const std::string& sRtspMessage, std::string& sContentLength)
{
  boost::regex content_length_regex("(Content-Length: )(\\d+)");
  std::istringstream istr(sRtspMessage);
  std::string sLine;
  while (std::getline(istr, sLine))
  {
    boost::smatch match;
    if (boost::regex_search(sLine, match, content_length_regex))
    {
      sContentLength = match[2];
      return true;
    }
  }
  return false;
}

std::string getCurrentDateString()
{
  boost::posix_time::ptime t = boost::posix_time::second_clock::universal_time();

  // Format dates and times
  boost::posix_time::time_facet* output_facet = new boost::posix_time::time_facet();

  std::stringstream ss;
  ss.imbue(std::locale(std::locale::classic(), output_facet));
  output_facet->format("%a, %d %b %Y %H:%M:%S GMT");

  ss.str("");
  ss << t;
  return ss.str();
}

bool getTransport(const std::string& sRtspMessage, Transport& transport)
{
  std::istringstream istr(sRtspMessage);
  std::string sLine;
  while (std::getline(istr, sLine))
  {
    if (boost::algorithm::starts_with(sLine, "Transport: "))
    {
      Transport t(sLine.substr(11));
      transport = t;
      return t.isValid();
    }
  }
  return false;
}

std::string createErrorResponse(ResponseCode eCode, uint32_t CSeq, const std::vector<std::string>& vSupported, const std::vector<std::string>& vUnsupported)
{
  return createErrorResponse(eCode, toString(CSeq), vSupported, vUnsupported);
}

std::string createErrorResponse(ResponseCode eCode, const std::string& CSeq, const std::vector<std::string>& vSupported, const std::vector<std::string>& vUnsupported)
{
#if 1
  std::ostringstream ostr;
  ostr << "RTSP/1.0 " << (uint32_t)eCode << " " << responseCodeString(eCode) << "\r\n";
  ostr << "CSeq: " << CSeq << "\r\n";
  ostr << "Date: " << rfc2326::getCurrentDateString() << "\r\n";

  if (!vSupported.empty())
  {
    ostr << toString(vSupported) << "\r\n";
  }

  if (!vUnsupported.empty())
  {
    ostr << toString(vUnsupported) << "\r\n";
  }

  ostr << "\r\n";
  return ostr.str();
#else
  std::string ERROR_RESPONSE="RTSP/1.0 <<Code>> <<ResponseCodeString>>\r\n"\
      "CSeq: <<CSeq>>\r\n"\
      "Date: <<Date>>\r\n\r\n";

  boost::algorithm::replace_first(ERROR_RESPONSE, "<<Code>>", ::toString((uint32_t)eCode));
  boost::algorithm::replace_first(ERROR_RESPONSE, "<<ResponseCodeString>>", responseCodeString(eCode));
  boost::algorithm::replace_first(ERROR_RESPONSE, "<<CSeq>>", CSeq);
  boost::algorithm::replace_first(ERROR_RESPONSE, "<<Date>>", rfc2326::getCurrentDateString());
  return ERROR_RESPONSE;
#endif
}

}
}
