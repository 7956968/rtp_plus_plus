#pragma once
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @class RtspUri abstracts the information needed to represent an RTSP URI.
 *
 * An RTSP URI is of the form rtsp://<hostname>[:rtsp_port]/[<relative_path>/]<name>
 */
class RtspUri
{
public:

  /**
   * @brief Default constructor
   */
  RtspUri()
    :m_sName(""),
    m_sRelativePath(""),
    m_sServerIp(""),
    m_uiPort(554)
  {

  }
  /**
   * @brief Constructor
   * @param[in] sUrl The string that is to be parsed into the components of the RTSP URI
   */
  RtspUri(const std::string& sUrl)
    :m_sName(""),
    m_sRelativePath(""),
    m_sServerIp(""),
    m_uiPort(554)
  {
    parseUrl(sUrl, m_sServerIp, m_uiPort, m_sRelativePath, m_sName);
  }
  /**
   * @brief Constructor
   */
  RtspUri(const std::string& sName, const std::string& sRelativePath, const std::string& sServerIp, const unsigned uiPort )
    :m_sName(sName),
    m_sRelativePath(sRelativePath),
    m_sServerIp(sServerIp),
    m_uiPort(uiPort)
  {
  }

  static void parseUrl(const std::string& sUri, std::string& sServer, unsigned& uiPort, std::string& sRelativePath, std::string& sResource)
  {
    std::string sUrlCopy = (boost::starts_with(sUri, "rtsp://")) ? sUri.substr(7) : sUri;

    size_t posLastSeperator = sUrlCopy.rfind("/");
    if ( posLastSeperator != std::string::npos )
    {
      sResource = sUrlCopy.substr( posLastSeperator + 1 );
      size_t posFirstSeperator = sUrlCopy.find("/");
      if ( posFirstSeperator == posLastSeperator )
      {
        // There's no relative path
        sServer = sUrlCopy.substr( 0, posFirstSeperator );
        sRelativePath = "";
      }
      else
      {
        sServer = sUrlCopy.substr( 0, posFirstSeperator );
        sRelativePath = sUrlCopy.substr( posFirstSeperator + 1, posLastSeperator - posFirstSeperator - 1);
      }

      // Check if we have a port
      size_t posColon = sServer.find(":");
      if ( posColon != std::string::npos )
      {
        std::string sPort = sServer.substr( posColon + 1);
        std::istringstream istr(sPort);
        istr >> uiPort;
        sServer = sServer.substr(0, posColon);
      }
    }
    else
    {
      // Couldn't find the last separator:
      // Need to check if we're dealing with a server + port or with just a resource name
      // We're prob dealing with a URL if the URI starts with rtsp:// or if there's a port
      // Check if we have a port
      if ((boost::starts_with(sUri, "rtsp://")))
      {
        size_t posColon = sUrlCopy.find(":");
        if ( posColon != std::string::npos )
        {
          std::string sPort = sUrlCopy.substr( posColon + 1);
          std::istringstream istr(sPort);
          istr >> uiPort;
          sServer = sUrlCopy.substr(0, posColon);
        }
        else
        {
          sServer = sUrlCopy;
        }
      }
      else
      {
        sResource = sUri;
      }
    }
  }

  static RtspUri parseUri(const std::string& sUri)
  {
    return RtspUri(sUri);
  }

  std::string getName() const { return m_sName; }
  void setName(const std::string& val) { m_sName = val; }
  std::string getServerIp() const { return m_sServerIp; }
  void setServerIp(const std::string& val) { m_sServerIp = val; }
  std::string getRelativePath() const { return m_sRelativePath; }
  void setRelativePath(const std::string& val) { m_sRelativePath = val; }
  unsigned getPort() const { return m_uiPort; }
  void setPort(unsigned val) { m_uiPort = val; }

  // Returns absolute path to resource
  std::string fullUri()
  {
    std::ostringstream ostr;
    if ( m_sServerIp != "" )
    {
      ostr << "rtsp://" << m_sServerIp;

      if ( m_uiPort > 0 )
        ostr << ":" << m_uiPort;
    }

    if (m_sRelativePath != "")
      ostr << "/" << m_sRelativePath;

    ostr << "/" << m_sName;
    return ostr.str();
  }

  // Returns the Url to the resource without server ip
  std::string relativeUri()
  {
    return (m_sRelativePath != "") ? "/" + m_sRelativePath + "/" + m_sName : m_sName;
  }

  std::string hostUri()
  {
    std::ostringstream ostr;
    if ( m_sServerIp != "" )
    {
      ostr << "rtsp://" << m_sServerIp;

      if ( m_uiPort > 0 )
        ostr << ":" << m_uiPort;
    }
    return ostr.str();
  }

protected:

  /// Name of item
  std::string m_sName;
  /// Relative path from server root
  std::string m_sRelativePath;
  /// Server
  std::string m_sServerIp;
  /// Port
  unsigned m_uiPort;
};

} // rfc2326
} // rtp_plus_plus
