#pragma once
#include <string>
#include <vector>

namespace rtp_plus_plus
{

/**
 * A local network interface is described by IP and netmask.
 * Note: we cannot calculate the mask from the IP + address
 * class since the mask may be assigned via DHCP.
 */
struct NetworkInterface
{
  std::string Address;
  std::string Mask;
};

/**
 * @brief NetworkInterfaceUtil is a helper class to determine the local network interfaces
 * There is no cross platform method to determine this reliable, and it is further complicated
 * by there being multiple types if network interfaces.
 */
class NetworkInterfaceUtil
{
public:

  /**
   * gets a list of local network interfaces with their address masks.
   * @param bRemoveZeroAddresses removes addresses 0.0.0.0 from the list
   * @param bFilterOutLinkLocal removes link local ("169.254.x.x") addresses from the list. This is only relevant on Windows
   */
  static std::vector<NetworkInterface> getLocalInterfaces(bool bRemoveZeroAddresses, bool bFilterOutLinkLocal);
};

} // rtp_plus_plus
