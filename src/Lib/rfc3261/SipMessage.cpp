#include "CorePch.h"
#include <rtp++/rfc3261/SipMessage.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/FileUtil.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc3261/Rfc3261.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/// A valid SIP request formulated by a UAC MUST, at a minimum, contain
/// the following header fields : To, From, CSeq, Call - ID, Max - Forwards,
/// and Via; all of these header fields are mandatory in all SIP
/// requests.
bool isValidRequest(const SipMessage& sipMessage)
{
  if ((sipMessage.hasHeader(TO)) &&
      (sipMessage.hasHeader(FROM)) &&
      (sipMessage.hasHeader(CSEQ)) &&
      (sipMessage.hasHeader(CALL_ID)) &&
      (sipMessage.hasHeader(MAX_FORWARDS)))
  {
    return true;
  }
  return false;
}

bool isValidResponse(const SipMessage& sipMessage)
{
  if ((sipMessage.hasHeader(TO)) &&
    (sipMessage.hasHeader(FROM)) &&
    (sipMessage.hasHeader(CSEQ)) &&
    (sipMessage.hasHeader(CALL_ID)))
  {
    return true;
  }
  return false;
}

boost::optional<SipMessage> SipMessage::create(const std::string& sSipMessage)
{
  // get rid of multi line header fields
  boost::regex regex("(\r\n([ \t])+)");
  std::string sSipMessageCopy = boost::regex_replace(sSipMessage, regex, " ");

#define DEBUG_INCOMING_SIP
#ifdef DEBUG_INCOMING_SIP
  static int fileNo = 0;
  std::ostringstream outFile;
  outFile << "sip_" << fileNo << ".log";
  std::ostringstream outFile2;
  outFile2 << "sip_copy_" << fileNo++ << ".log";
  FileUtil::writeFile(outFile.str().c_str(), sSipMessage, false);
  FileUtil::writeFile(outFile2.str().c_str(), sSipMessageCopy, false);
#endif

  // extract message body if it exists
  std::string sMessageBody;
  std::size_t posMessageBody = sSipMessageCopy.find("\r\n\r\n");
  // a valid message should always contain this?
  if (posMessageBody != std::string::npos)
  {
    sMessageBody = sSipMessageCopy.substr(posMessageBody + 4);
    // avoid parsing of message body but leave a CRLF for getline
    sSipMessageCopy = sSipMessageCopy.substr(0, posMessageBody + 2);
  }
  else
  {
    LOG(WARNING) << "Invalid SIP message format - no CRLFCRLF ";
    return boost::optional<SipMessage>();
  }
  std::istringstream istr(sSipMessageCopy);
  std::string sLine;
  if (std::getline(istr, sLine))
  {
    boost::algorithm::trim(sLine);
    // Request-Line  =  Method SP Request-URI SP SIP-Version CRLF
    // Status-Line  =  SIP-Version SP Status-Code SP Reason-Phrase CRLF
    const std::string SIP_2_0("SIP/2.0");
    // try to match either request or response
    if (boost::algorithm::contains(sLine, SIP_2_0))
    {
      std::vector<std::string> tokens = StringTokenizer::tokenize(sLine, " ", true, true);
      if (tokens.size() < 3)
      {
        LOG(WARNING) << "Unknown SIP message format: " << sLine;
        return boost::optional<SipMessage>();
      }

      SipMessage sipMessage;
      if (boost::algorithm::starts_with(sLine, SIP_2_0))
      {
        // response: description consists of multiple words
        size_t pos = sLine.find(" ");
        pos = sLine.find(" ", pos + 1);
        sipMessage.setResponse(tokens[1], sLine.substr(pos + 1));
      }
      else if (boost::algorithm::ends_with(sLine, SIP_2_0))
      {
        // request
        sipMessage.setRequest(tokens[0], tokens[1]);
      }
      else
      {
        LOG(WARNING) << "Unknown SIP message format: " << sLine;
        return boost::optional<SipMessage>();
      }
      std::string sContentType;
      uint32_t uiContentLength = 0;
      // read remaining header fields
      while (std::getline(istr, sLine))
      {
        size_t pos = sLine.find_first_of(':');
        if (pos == std::string::npos)
        {
          break;
        }
        std::string sHeader = sLine.substr(0, pos);
        std::string sHeaderLower = boost::algorithm::to_lower_copy(sHeader);
        std::string sValue = sLine.substr(pos + 1);
        boost::algorithm::trim(sValue);
        sipMessage.addHeaderField(sHeader, sValue);
        // the method needs to be set for responses
        if (sHeaderLower == boost::algorithm::to_lower_copy(CSEQ))
        {
          size_t pos = sValue.find(" ");
          assert(pos != std::string::npos);
          std::string sMethod = sValue.substr(pos + 1);
          sipMessage.setMethod(sMethod);
        }
        else if (sHeaderLower == boost::algorithm::to_lower_copy(CONTENT_TYPE))
        {
          sContentType = sValue;
        }
        else if (sHeaderLower == boost::algorithm::to_lower_copy(CONTENT_LENGTH))
        {
          bool bDummy;
          uiContentLength = convert<uint32_t>(sValue, bDummy);
        }
      }

#ifdef DEBUG
      if (sMessageBody.length() != uiContentLength)
      {
        LOG(WARNING) << "ContentLength header and message body length mismatch!";
      }
#endif
      if (uiContentLength > 0 && !sContentType.empty() && sMessageBody.length() == uiContentLength)
      {
        MessageBody messageBody(sMessageBody, sContentType);
        sipMessage.setMessageBody(messageBody);
      }
      return boost::optional<SipMessage>(sipMessage);
    }
    else
    {
      LOG(WARNING) << "Unknown SIP message format: " << sSipMessage;
      return boost::optional<SipMessage>();
    }
  }
  else
  {
    LOG(WARNING) << "Invalid SIP message: Failed to getline";
    return boost::optional<SipMessage>();
  }
}

boost::optional<SipMessage> SipMessage::createResponse(const SipMessage& request)
{
  if (!isValidRequest(request))
  {
    LOG(WARNING) << "Invalid SIP request: Failed to generate response";
    return boost::optional<SipMessage>();
  }

  SipMessage response(NOT_SET);

  // 8.2.6.2 Headers and Tags
  //  The From field of the response MUST equal the From header field of
  //  the request.The Call - ID header field of the response MUST equal the
  //  Call - ID header field of the request.The CSeq header field of the
  //  response MUST equal the CSeq field of the request.The Via header
  //  field values in the response MUST equal the Via header field values
  //  in the request and MUST maintain the same ordering.
  response.addHeaderField(FROM, *request.getHeaderField(FROM));
  response.addHeaderField(CALL_ID, *request.getHeaderField(CALL_ID));
  response.addHeaderField(CSEQ, *request.getHeaderField(CSEQ));
  std::vector<std::string> vVia = request.getHeaderFields(VIA);
  for (auto& via : vVia)
    response.addHeaderField(VIA, via);

  return boost::optional<SipMessage>(response);
}

SipMessage::SipMessage()
  :m_eType(SIP_NOT_SET),
  m_sVersion("2.0"),
  m_eMethod(UNKNOWN),
  m_eResponseCode(NOT_SET)
{

}

SipMessage::SipMessage(const Method eMethod, const std::string& sRequestUri)
  :m_eType(SIP_REQUEST),
  m_sVersion("2.0"),
  m_eMethod(eMethod),
  m_sMethod(methodToString(eMethod)),
  m_sRequestUriString(sRequestUri),
  m_eResponseCode(NOT_SET)
{

}

SipMessage::SipMessage(const std::string& sMethod, const std::string& sRequestUri)
  :m_eType(SIP_REQUEST),
  m_sVersion("2.0"),
  m_eMethod(stringToMethod(sMethod)),
  m_sMethod(sMethod),
  m_sRequestUriString(sRequestUri),
  m_eResponseCode(NOT_SET)
{

}

SipMessage::SipMessage(const ResponseCode& eCode)
  :m_eType(SIP_RESPONSE),
  m_sVersion("2.0"),
  m_eMethod(UNKNOWN),
  m_eResponseCode(eCode)
{

}

void SipMessage::setRequest(const std::string& sMethod, const std::string& sRequestUri)
{
  m_eType = SIP_REQUEST;
  m_eMethod = stringToMethod(sMethod);
  m_sMethod = sMethod;
  m_sRequestUriString = sRequestUri;
}

void SipMessage::setResponse(const std::string& sResponseCode, const std::string& sResponseDescription)
{
  m_eType = SIP_RESPONSE;
  m_eResponseCode = stringToResponseCode(sResponseCode);
  m_sResponseCodeString = sResponseCode;
  m_sResponseCodeDescriptionString = sResponseDescription;
}

void SipMessage::setMethod(const std::string& sMethod)
{
  m_sMethod = sMethod;
  m_eMethod = stringToMethod(sMethod);
}

void SipMessage::setResponseCode(ResponseCode eCode)
{
  m_eResponseCode = eCode;
}

// crude method of breaking "To" and "From"  header into URI and display name
void getUriAndDisplayName(const std::string& sHeader, std::string& sUri, std::string& sDisplayName)
{
  std::string header(sHeader);
  size_t pos = header.find(";");
  if (pos != std::string::npos)
  {
    // get rid of tag field
    header = sHeader.substr(0, pos);
  }

  pos = header.find("<");
  if (pos == std::string::npos)
  {
    // no display name
    sUri = header;
    sDisplayName = "";
  }
  else
  {
    size_t pos2 = header.find(">");
    sUri = header.substr(pos + 1, pos2 - pos - 1);
    sDisplayName = header.substr(0, pos);
    boost::algorithm::trim(sDisplayName);
  }
}

std::string SipMessage::getFromUri() const
{
  auto from = getHeaderField(FROM);
  if (from)
  {
    std::string sFromUri, sFromDisplayName;
    getUriAndDisplayName(*from, sFromUri, sFromDisplayName);
    return sFromUri;
  }
  return "";
}

std::string SipMessage::getFromDisplayName() const
{
  auto from = getHeaderField(FROM);
  if (from)
  {
    std::string sFromUri, sFromDisplayName;
    getUriAndDisplayName(*from, sFromUri, sFromDisplayName);
    return sFromDisplayName;
  }
  return "";
}

std::string SipMessage::getContact() const
{
  auto contact = getHeaderField(CONTACT);
  return *contact;
}

void SipMessage::setContact(const std::string& sContact)
{
  setHeaderField(CONTACT, sContact);
}

std::string SipMessage::getContactUri() const
{
  auto contact = getHeaderField(CONTACT);
  if (contact)
  {
    std::string sContactUri, sContactDisplayName;
    getUriAndDisplayName(*contact, sContactUri, sContactDisplayName);
    return sContactUri;
  }
  return "";
}

std::string SipMessage::getContactDisplayName() const
{
  auto contact = getHeaderField(CONTACT);
  if (contact)
  {
    std::string sContactUri, sContactDisplayName;
    getUriAndDisplayName(*contact, sContactUri, sContactDisplayName);
    return sContactDisplayName;
  }
  return "";
}


std::string SipMessage::getToUri() const
{
  auto to = getHeaderField(TO);
  if (to)
  {
    std::string sToUri, sToDisplayName;
    getUriAndDisplayName(*to, sToUri, sToDisplayName);
    return sToUri;
  }
  return "";
}

std::string SipMessage::getToDisplayName() const
{
  auto to = getHeaderField(TO);
  if (to)
  {
    std::string sToUri, sToDisplayName;
    getUriAndDisplayName(*to, sToUri, sToDisplayName);
    return sToDisplayName;
  }
  return "";
}

std::string SipMessage::getTopMostVia() const
{
  VLOG(6) << " TODO: is this the top-most via?";
  boost::optional<std::string> via = getHeaderField(VIA);
  assert(via);
  return *via;
}

void SipMessage::setTopMostVia(const std::string& sVia)
{
  VLOG(6) << " TODO: is this the top-most via?";
  auto it = m_mHeaderMap.find(VIA);
  assert(it != m_mHeaderMap.end());
  it->second = sVia;
}

void SipMessage::insertTopMostVia(const std::string& sVia)
{
  LOG(WARNING) << "TODO: insert top-most via";
  m_mHeaderMap.insert(m_mHeaderMap.begin(), std::make_pair(VIA, sVia));
}

std::string SipMessage::removeTopMostVia()
{
  LOG(WARNING) << "TODO: remove top-most via";
  std::string sTopMostVia;
  auto it = m_mHeaderMap.find(VIA);
  if (it != m_mHeaderMap.end())
  {
    sTopMostVia = it->second;
    m_mHeaderMap.erase(it);
  }
  return sTopMostVia;
}

std::string SipMessage::getSentByInTopMostVia() const
{
  std::string sVia = getTopMostVia();
  std::vector<std::string> tokens = StringTokenizer::tokenize(sVia, ";", true, true);
  for (auto param : tokens)
  {
    // crude parsing, should suffice for now
    if (boost::algorithm::starts_with(param, "SIP/2.0"))
    {
      size_t pos = param.find(" ");
      assert(pos != std::string::npos);
      return param.substr(pos + 1);
    }
  }
  return "";
}

void SipMessage::decrementMaxForwards()
{
  auto it = m_mHeaderMap.find(MAX_FORWARDS);
  if (it != m_mHeaderMap.end())
  {
    bool bDummy;
    uint32_t uiMaxForwards = convert<uint32_t>(it->second, bDummy);
    assert(bDummy);
    if (uiMaxForwards > 0) --uiMaxForwards;
    it->second = ::toString(uiMaxForwards);
  }
}

void SipMessage::addHeaderField(const std::string& sName, const std::string& sValue)
{
  m_mHeaderMap.insert(make_pair(sName, sValue));
}

void SipMessage::setHeaderField(const std::string& sName, const std::string& sValue)
{
  m_mHeaderMap.erase(sName);
  m_mHeaderMap.insert(make_pair(sName, sValue));
}

bool SipMessage::hasHeader(const std::string& sAttributeName) const
{
  auto it = m_mHeaderMap.find(sAttributeName);
  return (it != m_mHeaderMap.end());
}

boost::optional<std::string> SipMessage::getHeaderField(const std::string& sName) const
{
  auto it = m_mHeaderMap.find(sName);
  if (it != m_mHeaderMap.end())
    return boost::optional<std::string>(it->second);
  return boost::optional<std::string>();
}

std::vector<std::string> SipMessage::getHeaderFields(const std::string& sName) const
{
  typedef std::multimap<std::string, std::string> MultiStringMap_t;
  std::vector<std::string> res;
  std::pair<MultiStringMap_t::const_iterator, MultiStringMap_t::const_iterator> ret;
  ret = m_mHeaderMap.equal_range(sName);
  for (MultiStringMap_t::const_iterator it = ret.first; it != ret.second; ++it)
  {
    res.push_back(it->second);
  }
  return res;
}

std::string SipMessage::toString() const
{
  std::ostringstream ostr;
  if (isRequest())
  {
    ostr << m_sMethod << " " << m_sRequestUriString << " SIP/2.0\r\n";
  }
  else
  {
    ostr << "SIP/2.0 " << static_cast<uint32_t>(m_eResponseCode) << " " << responseCodeString(m_eResponseCode) << "\r\n";
  }

  // TODO: order the same as in the RFC for now?
  for (auto& pair : m_mHeaderMap)
  {
    ostr << pair.first << ": " << pair.second << "\r\n";
  }
  if (m_messageBody.getContentLength() > 0)
  {
    ostr << "Content-Length: " << m_messageBody.getContentLength() << "\r\n";
    ostr << "Content-Type: " << m_messageBody.getContentType() << "\r\n";
    // TODO: need to print other fields?
  }
  ostr << "\r\n";
  if (m_messageBody.getContentLength() > 0)
  {
    ostr << m_messageBody.getMessageBody();
  }

  return ostr.str();
}

boost::optional<std::string> SipMessage::getBranchParameter() const
{
  boost::optional<std::string> via = getHeaderField(VIA);
  if (!via) return boost::optional<std::string>();

  std::vector<std::string> vVia = StringTokenizer::tokenize(*via, ";", true, true);
  for (auto& attr : vVia)
  {
    // TOREMOVE: DBG
#if 0
    VLOG(2) << "Attribute: " << attr;
#endif
    if (boost::algorithm::starts_with(attr, "branch="))
    {
      std::string sBranch = attr.substr(7);
      return boost::optional<std::string>(sBranch);
    }
    // TOREMOVE: DBG
#if 0
    else
    {
      VLOG(2) << "No branch";
    }
#endif
  }
  return boost::optional<std::string>();
}

bool SipMessage::copyHeaderField(const std::string& sHeaderField, const SipMessage& sipMessage)
{
  boost::optional<std::string> pHeader = sipMessage.getHeaderField(sHeaderField);
  if (!pHeader) return false;
  addHeaderField(sHeaderField, *pHeader);
  return true;
}

bool SipMessage::copyHeaderFields(const std::string& sHeaderField, const SipMessage& sipMessage)
{
  std::vector<std::string> vHeaders = sipMessage.getHeaderFields(sHeaderField);
  if (vHeaders.empty()) return false;
  for (auto& sHeader : vHeaders)
    addHeaderField(sHeaderField, sHeader);
  return true;
}

uint32_t SipMessage::getRequestSequenceNumber() const
{
  boost::optional<std::string> cseq = getHeaderField(CSEQ);
  assert(cseq);
  size_t pos = cseq->find(" ");
  assert(pos != std::string::npos);
  bool bSuccess = false;
  uint32_t uiSN = convert<uint32_t>(cseq->substr(0, pos), bSuccess);
  assert(bSuccess);
  return uiSN;
}

void SipMessage::setCSeq(uint32_t uiSn)
{
  std::ostringstream ostr;
  ostr << uiSn << " " << m_sMethod;
  m_mHeaderMap.erase(CSEQ);
  addHeaderField(CSEQ, ostr.str());
}

void SipMessage::setCSeq(uint32_t uiSn, const std::string& sMethod)
{
  std::ostringstream ostr;
  ostr << uiSn << " " << sMethod;
  m_mHeaderMap.erase(CSEQ);
  addHeaderField(CSEQ, ostr.str());
}

bool SipMessage::appendToHeaderField(const std::string& sHeaderField, const std::string& sValue)
{
  auto it = m_mHeaderMap.find(sHeaderField);
  if (it == m_mHeaderMap.end()) return false;
  std::string sUpdatedHeader = it->second + ";" + sValue;
  it->second = sUpdatedHeader;
  return true;
}

boost::optional<std::string> SipMessage::getAttribute(const std::string& sHeader, const std::string& sAttribute) const
{
  boost::optional<std::string> header = getHeaderField(sHeader);
  if (!header) return boost::optional<std::string>();

  std::vector<std::string> vAttribs = StringTokenizer::tokenize(*header, ";", true, true);
  for (auto& attr : vAttribs)
  {
    if (boost::algorithm::starts_with(attr, sAttribute))
    {
      std::string sValue = attr.substr(sAttribute.length() + 1); // for "="
      return boost::optional<std::string>(sValue);
    }
  }
  return boost::optional<std::string>();
}

bool SipMessage::hasAttribute(const std::string& sHeader, const std::string& sAttribute) const
{
  boost::optional<std::string> header = getHeaderField(sHeader);
  if (!header) return false;

  std::vector<std::string> vAttribs = StringTokenizer::tokenize(*header, ";", true, true);
  for (auto& attr : vAttribs)
  {
    if (boost::algorithm::starts_with(attr, sAttribute))
    {
      return true;
    }
  }
  return false;
}

bool SipMessage::setAttribute(const std::string& sHeader, const std::string& sAttribute, const std::string& sValue)
{
  auto it = m_mHeaderMap.find(sHeader);
  if (it == m_mHeaderMap.end()) return false;
  std::ostringstream updatedHeader;
  updatedHeader << it->second << ";" << sAttribute << "=" << sValue;
  it->second = updatedHeader.str();
  return true;
}

bool SipMessage::matchHeaderFieldAttribute(const std::string& sHeaderField, const std::string& sAttribute, const SipMessage& sipMessage) const
{
  boost::optional<std::string> attrib1 = getAttribute(sHeaderField, sAttribute);
  if (!attrib1) return false;
  boost::optional<std::string> attrib2 = sipMessage.getAttribute(sHeaderField, sAttribute);
  if (!attrib2) return false;

  return *attrib1 == *attrib2;
}

} // rfc3261
} // rtp_plus_plus
