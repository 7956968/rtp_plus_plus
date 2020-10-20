#pragma once
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <rtp++/network/PortAllocationManager.h>

namespace rtp_plus_plus
{

/**
 * @brief This class manages the information related to network configuration.
 *
 * This includes 
 * \li detection of local interfaces
 * \li managing which local interfaces are to be used
 * \li port management
 * \li constructing src_mprtp_addr or dest_mprtp_addr based on configuration
 */
class MediaSessionNetworkManager
{
public:
  /**
   * @brief Constructor
   */
  MediaSessionNetworkManager(boost::asio::io_service& ioService, uint16_t uiRtpPort = DEFAULT_RTP_PORT);
  /**
   * @brief Constructor
   */
  MediaSessionNetworkManager(boost::asio::io_service& ioService,
    const std::vector<std::string>& vLocalInterfaces, uint16_t uiRtpPort = DEFAULT_RTP_PORT);
  /**
   * @brief Clears all previously managed network information.
   *
   * Note that this frees up any ports/sockets allocated by this component
   * that have not been retrieved from the PortAllocationManager yet.
   */
  void clear();
  /**
   * @brief hasInterface
   * @param sInterface
   * @return
   */
  bool hasInterface(const std::string& sInterface) const
  {
    for (const std::string& sInt : m_vLocalInterfaces )
    {
      if (sInt == sInterface) return true;
    }
    return false;
  }
  /**
   * @brief Getter for local interfaces
   */
  const std::vector<std::string>& getLocalInterfaces() const { return m_vLocalInterfaces; }
  /** 
   * @brief Getter for primary local interfaces
   */
  std::string getPrimaryInterface() const { return m_vLocalInterfaces.at(0); }
  /**
   * @brief This method sets the local interface post-construction. Any previously discovered/configured interfaces
   * are discarded.
   */
  boost::system::error_code setLocalInterfaces(const std::vector<std::string>& vLocalInterfaces);
  /**
   * @brief Getter for port allocation manager
   */
  PortAllocationManager& getPortAllocationManager() { return m_portManager; }
  /**
   * @brief Getter for interface descriptor
   */
  InterfaceDescriptions_t& getInterfaceDescriptors() { return m_interfaceDescriptions; }
  /**
   * @brief Getter for media descriptor at specified index
   */
  MediaInterfaceDescriptor_t& getMediaDescriptor(uint32_t uiIndex) { return m_interfaceDescriptions.at(uiIndex); }
  /**
   * @brief Getter for media descriptor at specified index for the specified session
   */
  MediaInterfaceDescriptor_t& getMediaDescriptor(const std::string& sSession, uint32_t uiIndex)
  {
    return m_mInterfaceDescriptions.at(sSession).at(uiIndex);
  }
  /**
   * @brief Getter for last error
   */
  boost::system::error_code getLastError() { return m_lastError; }
  /**
   * @brief This method allocates allocates and binds ports for the requested media session.
   *
   * If the method fails, any changes are undone, and an error_code is returned.
   * @param uiRtpPort[in,out] The starting port at which port allocation occurs. The port of the 
   * primary interfacefound is returned if successful.
   * @param bRtcpMux Should RTP and RTCP be muxed on one port
   * @param bIsMpRtpEnabled If MPRTP transport is enabled
   * @param sMpRtpAddr This string maps onto either the src_mprtp_addr or dest_mprtp_addr fields.
   * If specified, the class tries to bind to the specified transport address. This string should be
   * of the form "1 146.64.28.171 49170; 2 146.64.19.110 49170"
   */
  boost::system::error_code allocatePortsForMediaSession(uint16_t& uiRtpPort, bool bRtcpMux, bool bIsMpRtpEnabled,
    const std::string& sMpRtpAddr = "");
  /**
   * @brief This method allocates allocates and binds ports for the requested media session.
   *
   * If the method fails, any changes are undone, and an error_code is returned.
   * @param sSession The session id of the RTSP session.
   * @param uiRtpPort[in,out] The starting port at which port allocation occurs. The port of the 
   * primary interfacefound is returned if successful.
   * @param bRtcpMux Should RTP and RTCP be muxed on one port
   * @param bIsMpRtpEnabled If MPRTP transport is enabled
   * @param sMpRtpAddr This string maps onto either the src_mprtp_addr or dest_mprtp_addr fields.
   * If specified, the class tries to bind to the specified transport address. This string should be
   * of the form "1 146.64.28.171 49170; 2 146.64.19.110 49170"
   */
  boost::system::error_code allocatePortsForMediaSession(const std::string& sSession, uint16_t& uiRtpPort, bool bRtcpMux, bool bIsMpRtpEnabled,
    const std::string& sMpRtpAddr = "");

private:
  /**
   * @brief validates the local interfaces if specified
   */
  boost::system::error_code validateLocalInterfaces();
  /**
   * @brief Helper method to allocate ports.
   *
   * @param[in] sInterface The interface to bind to
   * @param[in/out] uiRtpPort The RTP port that should be bound to. 
   * This may change depending on if bStrictPort = false. In this case the port manager
   * will find the next higher suitable port(s).
   * @param bStrictPort[in] If true, the method returns an error if it can't bind to 
   * the specified port
   * @param bRtcpMux[in] If RTP and RTCP should be muxed on the same port
   */
  boost::system::error_code allocate(const std::string& sInterface, uint16_t& uiRtpPort, 
    bool bStrictPort, bool bRtcpMux);
  /**
   * @brief Helper method to retrieve MediaInterfaceDescriptor_t from src/dst_mprtp_addr string
   */
  boost::system::error_code getMediaInterfaceDescriptorFromTransportString(const std::string& sMpRtpAddr,
    MediaInterfaceDescriptor_t& mediaInterfaces, bool bRtcpMux);
  /**
   * @brief Helper method to retrieve MediaInterfaceDescriptor_t from local interfaces
   */
  boost::system::error_code getMediaInterfaceDescriptorFromLocalInterfaces(MediaInterfaceDescriptor_t& mediaInterfaces);

private:
  static const uint32_t DEFAULT_RTP_PORT;

  PortAllocationManager m_portManager;
  //! Local network interfaces
  std::vector<std::string> m_vLocalInterfaces;
  //! Preferred RTP starting port
  uint16_t m_uiRtpPort;
  //! error code
  boost::system::error_code m_lastError;  
  //! Stores bound interfaces
  InterfaceDescriptions_t m_interfaceDescriptions;
  // map to store the interface descriptions of many media sessions
  std::map<std::string, InterfaceDescriptions_t> m_mInterfaceDescriptions;
};

} // rtp_plus_plus
