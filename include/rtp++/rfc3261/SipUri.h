#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <boost/optional.hpp>
#include <rtp++/rfc3261/Rfc3261.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief Scheme of URI.
 */
enum Scheme
{
  S_SIP,
  S_SIPS
};

/**
 * @brief SipUri class storing information about URI
 *
 * @todo: URI: sip:user:password@host:port;uri-parameters?headers
 * @todo: URI comparisons
 */
class SipUri
{
public:
  /**
   * @brief Named constructor
   */
  static boost::optional<SipUri> parse(const std::string& sUri);
  /**
   * @brief Constructor
   */
  SipUri();
  /**
   * @brief Constructor
   */
  SipUri(const std::string& sHost, const uint16_t uiPort = 0, const std::string& sUser = "", const std::string& sPassword = "", Scheme eScheme = S_SIP);
  /**
   * @brief Getter for 
   */
  std::string getRawUri() const { return m_sRawUri; }
  /**
   * @brief Getter for scheme
   */
  Scheme getScheme() const { return m_eScheme; }
  /**
   * @brief Getter for user
   */
  std::string getUser() const { return m_sUser; }
  /**
   * @brief Getter for password
   */
  std::string getPassword() const { return m_sPassword; }
  /**
   * @brief Getter for host
   */
  std::string getHost() const { return m_sHost; }
  /**
   * @brief Getter for port. If port is zero, the default port for the scheme is returned.
   */
  uint16_t getPort() const  { return (m_uiPort != 0) ? m_uiPort : ( (m_eScheme == S_SIP) ? DEFAULT_SIP_PORT : DEFAULT_SIPS_PORT); }
  /**
   * @brief Getter for URI parameters
   */
  const std::map<std::string, std::string>& getUriParameters() const { return m_mUriParameters; }
  /**
   * @brief Setter for URI parameters
   */
  void addUriParameter(const std::string& sName, const std::string& sValue);
  /**
   * @brief Getter for header fields
   */
  const std::map<std::string, std::string>& getHeaderFields() const { return m_mHeaderFields; }
  /**
   * @brief Setter for header fields
   */
  void addHeaderField(const std::string& sName, const std::string& sValue);
  /**
   * @brief Returns the string representation of the URI if the URI is valid.
   */
  boost::optional<std::string> toString() const;

private:
  /**
   * @brief Helper method to escape chars in a string
   *
   * @todo
   */
  std::string validateStringEncoding(const std::string& sValue);
private:
  
  /**
   * @brief Raw SIP Uri.
   */
  std::string m_sRawUri;
  /**
   * @brief SIP scheme
   */
  Scheme m_eScheme;
  /**
   * @brief The identifier of a particular resource at the host being
   * addressed.  The term "host" in this context frequently refers
   * to a domain.  The "userinfo" of a URI consists of this user
   * field, the password field, and the @ sign following them.  The
   * userinfo part of a URI is optional and MAY be absent when the
   * destination host does not have a notion of users or when the
   * host itself is the resource being identified.  If the @ sign is
   * present in a SIP or SIPS URI, the user field MUST NOT be empty.
   */
  std::string m_sUser;
  /**
   * @brief A password associated with the user.  While the SIP and
   * SIPS URI syntax allows this field to be present, its use is NOT
   * RECOMMENDED, because the passing of authentication information
   * in clear text (such as URIs) has proven to be a security risk
   * in almost every case where it has been used.  For instance,
   * transporting a PIN number in this field exposes the PIN.
   * Note that the password field is just an extension of the user
   * portion.  Implementations not wishing to give special
   * significance to the password portion of the field MAY simply
   * treat "user:password" as a single string.
   */
  std::string m_sPassword;
  /**
   * @brief The host providing the SIP resource.  The host part contains
   * either a fully-qualified domain name or numeric IPv4 or IPv6
   * address.  Using the fully-qualified domain name form is
   * RECOMMENDED whenever possible.
   */
  std::string m_sHost;
  /**
   * @brief The port number where the request is to be sent.
   */
  uint16_t m_uiPort;
  /**
   * @brief Parameters affecting a request constructed from
   * the URI.
   * URI parameters are added after the hostport component and are
   * separated by semi-colons.
   * URI parameters take the form:
   *    parameter-name "=" parameter-value
   * Even though an arbitrary number of URI parameters may be
   * included in a URI, any given parameter-name MUST NOT appear
   * more than once.
   * This extensible mechanism includes the transport, maddr, ttl,
   * user, method and lr parameters.
   */
  std::map<std::string, std::string> m_mUriParameters;
  /**
   * @brief Used to remember the order of parameters
   */
  std::vector<std::string> m_vUriParameters;
  /**
   * @brief Header fields to be included in a request constructed
   * from the URI.
   * Headers fields in the SIP request can be specified with the "?"
   * mechanism within a URI.  The header names and values are
   * encoded in ampersand separated hname = hvalue pairs.  The
   * special hname "body" indicates that the associated hvalue is
   * the message-body of the SIP request.
   */
  std::map<std::string, std::string> m_mHeaderFields;
  /**
   * @brief Used to remember the order of header fields
   */
  std::vector<std::string> m_vHeaderFields;
};


} // rfc3261
} // rtp_plus_plus
