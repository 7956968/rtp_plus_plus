#pragma once
#include <map>
#include <sstream>
#include <string>
#include <boost/optional.hpp>
#include <boost/unordered_set.hpp>

#define RTSP_1_0

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief The Transport class manages the information in the RTSP Transport header.
 *
 * The general syntax for the transport specifier is a list of slash
 * separated tokens Value1/Value2/Value3...
 * Which for RTP transports take the form RTP/profile/lower-transport
 */
class Transport
{
public:
  typedef boost::unordered_set<std::string> unordered_set;
  typedef std::pair<std::string, unsigned short> Address_t;

public:
  /**
   * @brief Constructor
   */
  Transport();
  /**
   * @brief Constructor parses sTransportHeader and sets all the member values.
   */
  Transport(const std::string& sTransportHeader);
  /**
   * @brief Destructor
   */
  ~Transport();
  /**
   * @brief checks if the class was able to parse the transport header
   */
  bool isValid() const { return m_bValid; }
  /**
   * @brief Getter for transport string that was passed into the contructor
   *
   * In order to get a string representation of the Transport header, the toString()
   * method should be called.
   */
  std::string getTransportHeader() const { return m_sTransportHeader; }
  /**
   * @brief returns a string representation of the Transport header
   *
   */
  std::string toString() const;
  /**
   * @brief Getter for if Transport is unicast
   */
  bool isUnicast() const { return m_bUnicast; }
  /**
   * @brief Setter for if Transport is unicast
   */
  void setUnicast() { m_bUnicast = true; }
  /**
   * @brief Getter for if Transport is multicast
   */
  bool isMulticast() const { return !m_bUnicast; }
  /**
   * @brief Setter for if Transport is multicast
   */
  void setMulticast() { m_bUnicast = false; }
  /**
   * @brief Getter for if Transport is interleaved
   */
  bool isInterleaved() const { return m_bInterleaved; }
  /**
   * @brief Setter for Transport specifier: Transport/Profile/Lower Transport e.g. RTP/AVP or RTP/AVP/TCP
   * @return
   */
  std::string getSpecifier() const
  {
    std::ostringstream ostr;
    ostr << m_sTransport << "/" << m_sProfile;
    if (!m_sLowerTransport.empty())
      ostr << "/" << m_sLowerTransport;
    return ostr.str();
  }
  /**
   * @brief Setter for Transport specifier: Transport/Profile/Lower Transport 
   */
  bool setTransportSpecifier(const std::string &val); 
  /**
   * @brief Getter for Transport
   */
  std::string getTransport() const { return m_sTransport; }
  /**
   * @brief Setter for Transport
   */
  void setTransport(const std::string& val) { m_sTransport = val; }
  /**
   * @brief Getter for Profile e.g. AVP or AVPF
   */
  std::string getProfile() const { return m_sProfile; }
  /**
   * @brief Setter for Profile e.g. AVP or AVPF
   */
  void setProfile(const std::string& val) { m_sProfile = val; }
  /**
   * @brief Getter for lower transport e.g. TCP
   */
  std::string getLowerTransport() const { return m_sLowerTransport; }
  /**
   * @brief Setter for lower transport e.g. TCP
   */
  void setLowerTransport(const std::string& val) { m_sLowerTransport = val; }
#ifdef RTSP_1_0
  unsigned short getClientPortStart() const { return m_uiClientStartPort; }
  void setClientPortStart(unsigned short val) { m_uiClientStartPort = val; }
  unsigned short getServerPortStart() const { return m_uiServerStartPort; }
  void setServerPortStart(unsigned short val) { m_uiServerStartPort = val; }
#endif
  unsigned getInterleavedStart() const { return m_uiInterleavedStart; }
  void setInterleavedStart(unsigned val) { m_uiInterleavedStart = val; m_bInterleaved = true; }

//#ifdef RTSP_2_0
  std::vector<Address_t> getSrcAddresses() const { return m_SrcAddresses; }
  std::vector<Address_t> getDestAddresses() const { return m_vDestAddresses; }
//#endif

  /**
   * @brief Getter for Transport "source" attribute.
   *
   * RTSP 1.0
   */
  std::string getSource() const { return m_sSource; }
  /**
   * @brief Setter for Transport "source" attribute.
   *
   * RTSP 1.0
   */
  void setSource(const std::string& sSource) { m_sSource = sSource; }
  /**
   * @brief Getter for Transport "destination" attribute.
   *
   * RTSP 1.0
   */
  std::string getDestination() const { return m_sDestination; }
  /**
   * @brief Setter for Transport "destination" attribute.
   *
   * RTSP 1.0
   */
  void setDestination(const std::string& sDestination) { m_sDestination = sDestination; }

  /**
   * @brief hasExtmaps returns if any extmaps have been set on the transport.
   *
   * CAUTION: this is not specified by IETF
   */
  bool hasExtmaps() const
  {
    return !m_mExtmaps.empty();
  }
  /**
   * @brief setExtmap sets an extmap value
   *
   * CAUTION: this is not specified by IETF
   */
  void setExtmap(uint32_t uiId, const std::string& sExtmap)
  {
    m_mExtmaps[uiId] = sExtmap;
  }
  /**
   * @brief lookupExtmap looks up the extension name based on the id
   *
   * CAUTION: this is not specified by IETF
   */
  boost::optional<std::string> lookupExtmap(const uint32_t uiId) const
  {
    auto it = m_mExtmaps.find(uiId);
    if (it != m_mExtmaps.end())
      return boost::optional<std::string>(it->second);
    return boost::optional<std::string>();
  }
  /**
   * @brief lookupExtmapId looks up the id of the specified extension
   *
   * CAUTION: this is not specified by IETF
   */
  boost::optional<uint32_t> lookupExtmapId(const std::string& sExtmap) const
  {
    for (auto it = m_mExtmaps.begin(); it != m_mExtmaps.end(); ++it)
    {
      if (it->second == sExtmap)
        return boost::optional<uint32_t>(it->first);
    }
    return boost::optional<uint32_t>();
  }
  /**
   * @brief getExtmaps returns a string representing the extmaps for the RTP session
   *
   * CAUTION: this is not specified by IETF
   */
  std::string getExtmaps() const
  {
    if (m_mExtmaps.empty()) return "";
    std::ostringstream ostr;
    for (auto mapEntry : m_mExtmaps)
    {
      ostr << mapEntry.first << " " << mapEntry.second << ";";
    }
    std::string sExtmap = ostr.str();
    // remove trailing ';'
    return sExtmap.substr(0, sExtmap.length() - 1);
  }
  /**
   * This is defined in the RTSP 2.0 specification and is needed for MPRTP
   *
   * RTSP 2.0
   */
  bool getRtcpMux() const
  {
    return m_bRtcpMux;
  }
  /**
   * This is defined in the RTSP 2.0 specification and is needed for MPRTP
   *
   * RTSP 2.0
   */
  void setRtcpMux(bool bRtcpMux)
  {
    m_bRtcpMux = bRtcpMux;
  }
  /**
   * @brief utility method to check if MPRTP source and.or destination addresses have been set
   *
   * RTSP 2.0 & MPRTP SDP draft
   */
  bool isUsingMpRtp() const
  {
    // validation has been added  in the set methods
    return !m_sDestMpRtpAddr.empty() || !m_sSrcMpRtpAddr.empty();
  }
  /**
   * @brief This is defined in MPRTP SDP draft
   *
   * RTSP 2.0 & MPRTP SDP draft
   */
  std::string getSrcMpRtpAddr() const
  {
    return m_sSrcMpRtpAddr;
  }
  /**
   * This is defined in MPRTP SDP draft
   * @param[in] sSrcMpRtpAddr The src_mprtp_addr field to be set.
   * @return true if basic validation of sSrcMpRtpAddr passes, false otherwise.
   *
   * RTSP 2.0 & MPRTP SDP draft
   */
  bool setSrcMpRtpAddr(const std::string& sSrcMpRtpAddr);
  /**
   * This is defined in MPRTP SDP draft
   *
   * RTSP 2.0 & MPRTP SDP draft
   */
  std::string getDestMpRtpAddr() const
  {
    return m_sDestMpRtpAddr;
  }
  /**
   * This is defined in MPRTP SDP draft
   * @param[in] sDestMpRtpAddr The dest_mprtp_addr field to be set.
   * @return true if basic validation of sDestMpRtpAddr passes, false otherwise.
   *
   * RTSP 2.0 & MPRTP SDP draft
   */
  bool setDestMpRtpAddr(const std::string& sDestMpRtpAddr);

private:

  void parseTransportString(const std::string& sTransport);
  bool parseAddresses(std::string& sTransport, std::vector<Address_t>& vAddresses);
  bool extractAddress( const std::string &sTransport, std::vector<Address_t> &vAddresses );

  std::string m_sTransportHeader;

  std::vector<std::string> m_vParams;
  bool m_bValid;
  bool m_bUnicast;
  bool m_bInterleaved;

  std::string m_sTransport;
  std::string m_sProfile;
  std::string m_sLowerTransport;

  /// RTSP 1.0
  unsigned short m_uiClientStartPort;
  /// RTSP 1.0
  unsigned short m_uiServerStartPort;

  /// RTSP 2.0
  std::vector<Address_t> m_SrcAddresses;
  /// RTSP 2.0
  std::vector<Address_t> m_vDestAddresses;

  /// RTSP 1.0 attribute
  std::string m_sSource;
  /// RTSP 1.0 attribute
  std::string m_sDestination;

  unsigned m_uiInterleavedStart;

  /// RTSP 2.0 attribute
  bool m_bRtcpMux;

  /// MPRTP SDP draft attribute
  std::string m_sDestMpRtpAddr;
  /// MPRTP SDP draft attribute
  std::string m_sSrcMpRtpAddr;

  /// Not specified by IETF
  std::map<uint32_t, std::string> m_mExtmaps;
};

} // rfc2326
} // rtp_plus_plus
