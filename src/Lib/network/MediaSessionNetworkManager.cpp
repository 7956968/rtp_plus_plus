#include "CorePch.h"
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/NetworkInterfaceUtil.h>

// This binds single path interfaces to the 0.0.0.0 address
// #define BIND_SINGLE_PATH_INTERFACE_TO_0_0_0_0

namespace rtp_plus_plus
{

const uint32_t MediaSessionNetworkManager::DEFAULT_RTP_PORT = 49170;

MediaSessionNetworkManager::MediaSessionNetworkManager(boost::asio::io_service& ioService, uint16_t uiRtpPort)
  :m_portManager(ioService),
  m_uiRtpPort(uiRtpPort)
{
  VLOG(2) << "No local interfaces specified.";
  // try to detect local interfaces
  std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
  for (const NetworkInterface& networkInterface : networkInterfaces)
  {
    VLOG(2) << "No local interfaces specified. Detected " << networkInterface.Address;
    m_vLocalInterfaces.push_back(networkInterface.Address);
  }
}

MediaSessionNetworkManager::MediaSessionNetworkManager(boost::asio::io_service& ioService,
  const std::vector<std::string>& vLocalInterfaces, uint16_t uiRtpPort)
  :m_portManager(ioService),
  m_vLocalInterfaces(vLocalInterfaces),
  m_uiRtpPort(uiRtpPort)
{
  if (vLocalInterfaces.empty())
  {
    std::ostringstream ostr;
    ostr << "No local interfaces specified. Detected ";
    // try to detect local interfaces
    std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
    for (const NetworkInterface& networkInterface : networkInterfaces)
    {
      ostr << networkInterface.Address << " ";
      m_vLocalInterfaces.push_back(networkInterface.Address);
    }
    VLOG(2) << ostr.str();
  }
  else
  {
    m_lastError = validateLocalInterfaces();
  }
}

void MediaSessionNetworkManager::clear()
{
  DLOG(INFO) << "TODO: free up any managed network info";
}

boost::system::error_code MediaSessionNetworkManager::setLocalInterfaces(const std::vector<std::string>& vLocalInterfaces)
{
  m_vLocalInterfaces.clear();
  if (vLocalInterfaces.empty())
  {
    VLOG(2) << "No local interfaces specified.";
    // try to detect local interfaces
    std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
    for (const NetworkInterface& networkInterface : networkInterfaces)
    {
      VLOG(2) << "No local interfaces specified. Detected " << networkInterface.Address;
      m_vLocalInterfaces.push_back(networkInterface.Address);
    }
    return boost::system::error_code();
  }
  else
  {
    m_vLocalInterfaces = vLocalInterfaces;
    m_lastError = validateLocalInterfaces();
    return m_lastError;
  }
}

boost::system::error_code MediaSessionNetworkManager::validateLocalInterfaces()
{
  // local interfaces were specified, let's make sure they're valid
  for (const std::string& sSpecifiedInterface : m_vLocalInterfaces)
  {
    VLOG(2) << "Local network interface specified " << sSpecifiedInterface;
#if 1
    if (sSpecifiedInterface == "0.0.0.0")
      DLOG(INFO) << "TODO: what happens when the local interface is 0.0.0.0?";
#endif
    bool bFound = false;
    std::vector<NetworkInterface> vDetectedNetworkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
    for (const NetworkInterface& networkInterface : vDetectedNetworkInterfaces)
    {
      if (sSpecifiedInterface == networkInterface.Address)
      {
        bFound = true;
        break;
      }
    }
    if (!bFound)
    {
      LOG(WARNING) << "Failed to match specified interface " << sSpecifiedInterface;
      return boost::system::error_code(boost::system::errc::no_such_device_or_address, boost::system::get_generic_category());
    }
  }
  return boost::system::error_code();
}

boost::system::error_code MediaSessionNetworkManager::getMediaInterfaceDescriptorFromTransportString(const std::string& sMpRtpAddr, MediaInterfaceDescriptor_t& mediaInterfaces, bool bRtcpMux)
{
  MediaInterfaceDescriptor_t interfaces = mprtp::getMpRtpInterfaceDescriptorFromRtspTransportString(sMpRtpAddr, bRtcpMux);
  if (interfaces.empty())
  {
    LOG(WARNING) << "Failed to parse src/dst_mprtp_addr: " << sMpRtpAddr;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  // we need to bind only to the specified interfaces
  // validate interfaces according to local interfaces
  for (AddressTuple_t& desc : interfaces)
  {
    // we're only checking the RTP address: we know that getMpRtpInterfaceDescriptorFromRtspTransportString
    // returns the same address twice with different ports
    const std::string& sAddress = std::get<0>(desc).getAddress();
    auto it = std::find_if(m_vLocalInterfaces.begin(), m_vLocalInterfaces.end(), [&sAddress](const std::string& sLocalInterface)
    {
      return sLocalInterface == sAddress;
    });
    if (it == m_vLocalInterfaces.end())
    {
      LOG(WARNING) << "Invalid interface: " << sAddress;
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    }
  }
  mediaInterfaces.insert(mediaInterfaces.end(), interfaces.begin(), interfaces.end());
  return boost::system::error_code();
}

boost::system::error_code MediaSessionNetworkManager::getMediaInterfaceDescriptorFromLocalInterfaces(MediaInterfaceDescriptor_t& mediaInterfaces)
{
  for (size_t i = 0; i < m_vLocalInterfaces.size(); ++i)
  {
    const std::string& sInterface = m_vLocalInterfaces[i];
    // should use port of 0 to say "any"?
    AddressDescriptor rtp(sInterface, m_uiRtpPort);
    AddressDescriptor rtcp(sInterface, 0); // rtcp port is not important, since allocate uses the RTCP-mux value
    mediaInterfaces.push_back(std::tie(rtp, rtcp, ""));
  }
  return boost::system::error_code();
}

boost::system::error_code MediaSessionNetworkManager::allocatePortsForMediaSession(uint16_t& uiRtpPort,
  bool bRtcpMux, bool bIsMpRtpEnabled, const std::string& sMpRtpAddr)
{
  DLOG(INFO) << "TODO: if something fails, we need to dealloc previously bound ports";
  uint16_t uiStartingRtpPort = uiRtpPort;
  // we set this to zero so that this only gets set by the once the primary interface is bound
  uiRtpPort = 0;
  MediaInterfaceDescriptor_t mediaInterfaces;

  if (bIsMpRtpEnabled)
  {
    MediaInterfaceDescriptor_t mediaInterfaces;
    bool bUseMpRtpAddrTransportString = (sMpRtpAddr != "");
    if (bUseMpRtpAddrTransportString)
    {
      boost::system::error_code ec = getMediaInterfaceDescriptorFromTransportString(sMpRtpAddr, mediaInterfaces, bRtcpMux);
      if (ec) return ec;
    }
    else
    {
      boost::system::error_code ec = getMediaInterfaceDescriptorFromLocalInterfaces(mediaInterfaces);
      if (ec) return ec;
    }

    // now allocate interfaces/ports
    bool bStrictPortAllocation = bUseMpRtpAddrTransportString;
    for (auto& addresses : mediaInterfaces)
    {
      AddressDescriptor& rtp = std::get<0>(addresses);
      AddressDescriptor& rtcp = std::get<1>(addresses);
      uiStartingRtpPort = rtp.getPort();
      // double check that a port was actually set
      if (uiStartingRtpPort == 0) uiStartingRtpPort = m_uiRtpPort;

      VLOG(2) << "Attempting to bind to " << rtp.getAddress() << ":" << uiStartingRtpPort;
      boost::system::error_code ec = allocate(rtp.getAddress(), uiStartingRtpPort, bStrictPortAllocation, bRtcpMux);
      if (!ec)
      {
        // notify caller of first primary interface port
        if (uiRtpPort == 0) uiRtpPort = uiStartingRtpPort;
        // we now need to update the interfaces in the descriptors as these
        // are used to lookup the ports
        rtp.setPort(uiStartingRtpPort);
        uint16_t uiRtcpPort = bRtcpMux ? uiStartingRtpPort : uiStartingRtpPort + 1;
        rtcp.setPort(uiRtcpPort);
      }
      else
      {
        return ec;
      }
    }
    m_interfaceDescriptions.push_back(mediaInterfaces);
    return boost::system::error_code();
  }
  else
  {
    assert(!m_vLocalInterfaces.empty());
    std::string sInterface = m_vLocalInterfaces[0];
#ifdef BIND_SINGLE_PATH_INTERFACE_TO_0_0_0_0
    sInterface = "0.0.0.0";
#endif
    boost::system::error_code ec = allocate(sInterface, uiStartingRtpPort, false, bRtcpMux);
    if (!ec)
    {
      // set this so that the calling class knows what port was allocated
      uiRtpPort = uiStartingRtpPort;
      uint16_t uiRtcpPort = bRtcpMux ? uiRtpPort : uiRtpPort + 1;
      AddressDescriptor rtp(sInterface, uiRtpPort);
      AddressDescriptor rtcp(sInterface, uiRtcpPort);
      mediaInterfaces.push_back(std::tie(rtp, rtcp, ""));
      m_interfaceDescriptions.push_back(mediaInterfaces);
    }
    return ec;
  }
}

boost::system::error_code MediaSessionNetworkManager::allocatePortsForMediaSession(const std::string& sSession, uint16_t& uiRtpPort,
  bool bRtcpMux, bool bIsMpRtpEnabled, const std::string& sMpRtpAddr)
{
  DLOG(INFO) << "TODO: if something fails, we need to dealloc previously bound ports";
  uint16_t uiStartingRtpPort = uiRtpPort;
  // we set this to zero so that this only gets set by the once the primary interface is bound
  uiRtpPort = 0;
  MediaInterfaceDescriptor_t mediaInterfaces;

  if (bIsMpRtpEnabled)
  {
    MediaInterfaceDescriptor_t mediaInterfaces;
    bool bUseMpRtpAddrTransportString = (sMpRtpAddr != "");
    if (bUseMpRtpAddrTransportString)
    {
      boost::system::error_code ec = getMediaInterfaceDescriptorFromTransportString(sMpRtpAddr, mediaInterfaces, bRtcpMux);
      if (ec) return ec;
    }
    else
    {
      boost::system::error_code ec = getMediaInterfaceDescriptorFromLocalInterfaces(mediaInterfaces);
      if (ec) return ec;
    }

    // now allocate interfaces/ports
    bool bStrictPortAllocation = bUseMpRtpAddrTransportString;
    for (auto& addresses : mediaInterfaces)
    {
      AddressDescriptor& rtp = std::get<0>(addresses);
      AddressDescriptor& rtcp = std::get<1>(addresses);
      uiStartingRtpPort = rtp.getPort();
      // double check that a port was actually set
      if (uiStartingRtpPort == 0) uiStartingRtpPort = m_uiRtpPort;

      VLOG(2) << "Attempting to bind to " << rtp.getAddress() << ":" << uiStartingRtpPort;
      boost::system::error_code ec = allocate(rtp.getAddress(), uiStartingRtpPort, bStrictPortAllocation, bRtcpMux);
      if (!ec)
      {
        // notify caller of first primary interface port
        if (uiRtpPort == 0) uiRtpPort = uiStartingRtpPort;
        // we now need to update the interfaces in the descriptors as these
        // are used to lookup the ports
        rtp.setPort(uiStartingRtpPort);
        uint16_t uiRtcpPort = bRtcpMux ? uiStartingRtpPort : uiStartingRtpPort + 1;
        rtcp.setPort(uiRtcpPort);
      }
      else
      {
        return ec;
      }
    }
    m_mInterfaceDescriptions[sSession].push_back(mediaInterfaces);
    return boost::system::error_code();
  }
  else
  {
    assert(!m_vLocalInterfaces.empty());
    std::string sInterface = m_vLocalInterfaces[0];
#ifdef BIND_SINGLE_PATH_INTERFACE_TO_0_0_0_0
    sInterface = "0.0.0.0";
#endif
    boost::system::error_code ec = allocate(sInterface, uiStartingRtpPort, false, bRtcpMux);
    if (!ec)
    {
      // set this so that the calling class knows what port was allocated
      uiRtpPort = uiStartingRtpPort;
      uint16_t uiRtcpPort = bRtcpMux ? uiStartingRtpPort : uiStartingRtpPort + 1;
      AddressDescriptor rtp(sInterface, uiRtpPort);
      AddressDescriptor rtcp(sInterface, uiRtcpPort);
      mediaInterfaces.push_back(std::tie(rtp, rtcp, ""));
      m_mInterfaceDescriptions[sSession].push_back(mediaInterfaces);
    }
    return ec;
  }
}

inline boost::system::error_code MediaSessionNetworkManager::allocate(const std::string& sInterface, 
  uint16_t& uiRtpPort, bool bStrictPort, bool bRtcpMux)
{
  if (bRtcpMux)
  {
    return m_portManager.allocateUdpPort(sInterface, uiRtpPort, bStrictPort);
  }
  else
  {
    return m_portManager.allocateUdpPortsForRtpRtcp(sInterface, uiRtpPort, bStrictPort);
  }
}

} // rtp_plus_plus
