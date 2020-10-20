#pragma once
#include <cstdint>
#include <string>
#include <sstream>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief Struct containing local SIP context info.
 */
struct SipContext
{
  /// e.g. Alice
  std::string User;
  /// e.g. pc.somwhere.com
  std::string FQDN;
  /// somewhere.com
  std::string Domain;
  /// if should be secure
  bool Secure;
  /// SIP port
  uint16_t SipPort;

  /**
   * @brief Constructor
   */
  SipContext()
    :User("Anonymous"),
    Secure(false),
    SipPort(5060)
  {

  }
  /**
   * @brief Constructor
   */
  SipContext(const std::string& sUser, const std::string& sFQDN, bool bSecure = false, uint16_t uiSipPort = 5060)
    :User(sUser),
    FQDN(sFQDN),
    Secure(bSecure),
    SipPort(uiSipPort)
  {

  }
  /**
   * @brief Constructor
   */
  SipContext(const std::string& sUser, const std::string& sFQDN, const std::string& sDomain, bool bSecure = false, uint16_t uiSipPort = 5060)
    :User(sUser),
    FQDN(sFQDN),
    Domain(sDomain),
    Secure(bSecure),
    SipPort(uiSipPort)
  {

  }
  /**
   * @brief returns contact header based on SIP context
   */
  std::string getContact() const
  {
    std::ostringstream contact;
    if (Secure)
      contact << "sips:";
    else
      contact << "sip:";
    contact << User << "@" << FQDN;
    if (SipPort != 5060)
    {
      contact << ":" << SipPort;
    }
    return contact.str();
  }
  /**
   * @brief returns user@domain
   */
  std::string getUserSipUri() const
  {
    std::ostringstream uri;
    if (Secure)
      uri << "sips:";
    else
      uri << "sip:";
    uri << User << "@" << Domain;
    return uri.str();
  }
};

} // rfc3261
} // rtp_plus_plus
