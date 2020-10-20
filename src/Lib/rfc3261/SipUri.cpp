#include "CorePch.h"
#include <rtp++/rfc3261/SipUri.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipMessage.h>

#define COMPONENT_LEVEL_VERBOSITY 10

namespace rtp_plus_plus
{
namespace rfc3261
{

boost::optional<SipUri> SipUri::parse(const std::string& sUri)
{
  if (sUri.empty()) return boost::optional<SipUri>();

  std::string sUriCopy = sUri;
  std::string  sExtractedUri, sDisplayName;
  getUriAndDisplayName(sUri, sExtractedUri, sDisplayName);
  VLOG(COMPONENT_LEVEL_VERBOSITY) << "URI: " << sUri << " copy: " << sExtractedUri;
  Scheme eScheme;
  if (boost::algorithm::starts_with(sUriCopy, SIP))
  {
    if (boost::algorithm::starts_with(sUriCopy, SIPS))
    {
      eScheme = S_SIPS;
      sUriCopy = sUriCopy.substr(5);
    }
    else
    {
      eScheme = S_SIP;
      sUriCopy = sUriCopy.substr(4);
    }
  }
  else
  {
    VLOG(2) << "Unsupported SIP scheme: " << sUriCopy;
    return boost::optional<SipUri>();
  }

  std::string sUser, sPassword, sHost, sUriParameters, sHeaderFields;

  size_t pos = sUriCopy.find("@");
  if (pos != std::string::npos)
  {
    // parse user and password
    std::string sUserAndPassword = sUriCopy.substr(0, pos);
    sUriCopy = sUriCopy.substr(pos + 1);

    pos = sUserAndPassword.find(":");
    if (pos != std::string::npos)
    {
      sUser = sUserAndPassword.substr(0, pos);
      sPassword = sUserAndPassword.substr(pos + 1);
    }
    else
    {
      sUser = sUserAndPassword;
    }
  }

  // strip header fields
  pos = sUriCopy.find("?");
  if (pos != std::string::npos)
  {
    sHeaderFields = sUriCopy.substr(pos + 1);
    sUriCopy = sUriCopy.substr(0, pos);
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "Header fields: " << sHeaderFields << " URI COPY: " << sUriCopy;
  }
  else
  {
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "No header fields";
  }

  // strip URI parameters
  pos = sUriCopy.find(";");
  if (pos != std::string::npos)
  {
    sUriParameters = sUriCopy.substr(pos + 1);
    sUriCopy = sUriCopy.substr(0, pos);
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "URI parameters: " << sHeaderFields << " URI COPY: " << sUriCopy;
  }
  else
  {
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "No URI parameters";
  }

  // host and port
  uint16_t uiPort = 0;
  pos = sUriCopy.find(":");
  if (pos != std::string::npos)
  {
    bool bSuccess;
    uiPort = convert<uint16_t>(sUriCopy.substr(pos + 1), bSuccess);
    if (!bSuccess)
    {
      VLOG(2) << "Invalid SIP URI: " << sUri;
      return boost::optional<SipUri>();
    }

    sHost = sUriCopy.substr(0, pos);
  }
  else
  {
    sHost = sUriCopy;
  }

  VLOG(COMPONENT_LEVEL_VERBOSITY) << "SIP URI host: " << sHost << " port: " << uiPort << " user: " << sUser << " pass: " << sPassword << " scheme: " << eScheme << " URI params: " << sUriParameters;
  SipUri sipUri(sHost, uiPort, sUser, sPassword, eScheme);

  std::vector<std::string> vUriParameters = StringTokenizer::tokenize(sUriParameters, ";");
  for (auto& nvp : vUriParameters)
  {
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "Processing URI parameter: " << nvp;
    std::vector<std::string> vNvp = StringTokenizer::tokenize(nvp, "=");
    if (vNvp.size() != 2)
    {
      // TODO: check if this is correct?
      LOG(WARNING) << "Invalid URI parameter: " << nvp;
      return boost::optional<SipUri>();
    }
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "Adding URI parameter: " << vNvp[0] << ":" << vNvp[1];
    sipUri.addUriParameter(vNvp[0], vNvp[1]);
  }

  std::vector<std::string> vHeaderFields = StringTokenizer::tokenize(sHeaderFields, "&");
  for (auto& nvp : vHeaderFields)
  {
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "Processing header field: " << nvp;
    std::vector<std::string> vNvp = StringTokenizer::tokenize(nvp, "=");
    if (vNvp.size() != 2)
    {
      // TODO: check if this is correct?
      LOG(WARNING) << "Invalid header field: " << nvp;
      return boost::optional<SipUri>();
    }
    sipUri.addHeaderField(vNvp[0], vNvp[1]);
  }

  return boost::optional<SipUri>(sipUri);
}

SipUri::SipUri()
  :m_eScheme(S_SIP),
  m_uiPort(0)
{

}

SipUri::SipUri(const std::string& sHost, const uint16_t uiPort, const std::string& sUser, const std::string& sPassword, Scheme eScheme)
  :m_eScheme(eScheme),
  m_sUser(sUser),
  m_sPassword(sPassword),
  m_sHost(sHost),
  m_uiPort(uiPort)
{

}

void SipUri::addUriParameter(const std::string& sName, const std::string& sValue)
{
  VLOG(10) << "TODO: escaping of string values";
  m_mUriParameters[sName] = sValue;
  m_vUriParameters.push_back(sName);
}

void SipUri::addHeaderField(const std::string& sName, const std::string& sValue)
{
  VLOG(10) << "TODO: escaping of string values";
  m_mHeaderFields[sName] = sValue;
  m_vHeaderFields.push_back(sName);
}

boost::optional<std::string> SipUri::toString() const
{
  std::ostringstream ostr;
  
  switch (m_eScheme)
  {
  case S_SIP:
  {
    ostr << "sip:";
    break;
  }
  case S_SIPS:
  {
    ostr << "sips:";
    break;
  }
  }

  if (!m_sUser.empty())
  {
    ostr << m_sUser;
    if (!m_sPassword.empty())
    {
      ostr << ":" << m_sPassword;
    }
    ostr << "@";
  }

  // could add stronger validation
  if (m_sHost.empty()) return boost::optional<std::string>();
  ostr << m_sHost;

  if (m_uiPort != 0)
  {
    ostr << ":" << m_uiPort;
  }
  // This code would change the URI that was input.
  // instead getPort now returns the default port for the scheme
#if 0
  else
  {
    switch (m_eScheme)
    {
    case S_SIP:
    {
      ostr << ":" << DEFAULT_SIP_PORT;
      break;
    }
    case S_SIPS:
    {
      ostr << ":" << DEFAULT_SIPS_PORT;
      break;
    }
    }
  }
#endif

#define ORDERED
#ifdef ORDERED
  if (!m_vUriParameters.empty())
  {
    for (auto& sName : m_vUriParameters)
    {
      const auto it2 = m_mUriParameters.find(sName);
      assert(it2 != m_mUriParameters.end());
      ostr << ";" << sName << "=" << it2->second;
    }
  }
#else
  if (!m_mUriParameters.empty())
  {
    for (auto& pair : m_mUriParameters)
    {
      ostr << ";" << pair.first << "=" << pair.second;
    }
  }
#endif

#ifdef ORDERED
  if (!m_vHeaderFields.empty())
  {
    ostr << "?";
    auto it = m_vHeaderFields.begin();
    const auto it2 = m_mHeaderFields.find(*it);
    assert(it2 != m_mHeaderFields.end());
    ostr << *it << "=" << it2->second;
    ++it;
    while (it != m_vHeaderFields.end())
    {
      const auto it2 = m_mHeaderFields.find(*it);
      assert(it2 != m_mHeaderFields.end());
      ostr << "&" << *it << "=" << it2->second;
      ++it;
    }
  }
#else
  if (!m_mHeaderFields.empty())
  {
    ostr << "?";
    auto& it = m_mHeaderFields.begin();
    ostr << it->first << "=" << it->second;
    ++it;
    while (it != m_mHeaderFields.end())
    {
      ostr << "&" << it->first << "=" << it->second;
      ++it;
    }
  }
#endif  
  return boost::optional<std::string>(ostr.str());
}

std::string SipUri::validateStringEncoding(const std::string& sValue)
{
  // TODO
  return std::string();
}

} // rfc3261
} // rtp_plus_plus
