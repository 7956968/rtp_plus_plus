#pragma once
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <cpputil/StringTokenizer.h>

namespace rtp_plus_plus
{

/**
 * An Address descriptor has the format
 * AddressDescriptor = bind_address:bind_port:address:port
 *
 * MediaInterfaceDescriptor_t
 * There can be multiple interfaces per media line, each
 * separated by a comma
 * This must be surrounded by {}
 *
 * InterfaceDescriptionsList_t
 * Multiple media media interface descriptors can be separated
 * by a comma
*/

/// fwd
class AddressDescriptor;
/// combination of data and control descriptor: can be the same!
/// The std::string can be there as an optional id
typedef std::tuple<AddressDescriptor, AddressDescriptor, std::string> AddressTuple_t;
/// multiple address pairs per media channel
typedef std::vector<AddressTuple_t> MediaInterfaceDescriptor_t;
/// multiple media
typedef std::vector<MediaInterfaceDescriptor_t> InterfaceDescriptions_t;

// Descriptor for single address
class AddressDescriptor
{
public:
  AddressDescriptor()
    :m_uiBindPort(0),
    m_sBindAddress("0.0.0.0"),
    m_uiPort(0),
    m_sAddress("0.0.0.0")
  {

  }

  // simple constructor where no binding is required
  AddressDescriptor(const std::string& sAddress, uint16_t uiPort)
    :m_uiBindPort(0),
    m_sBindAddress("0.0.0.0"),
    m_uiPort(uiPort),
    m_sAddress(sAddress)
  {

  }

  AddressDescriptor(const std::string& sBindAddress, uint16_t uiBindPort, const std::string& sAddress, uint16_t uiPort)
    :m_uiBindPort(uiBindPort),
    m_sBindAddress(sBindAddress),
    m_uiPort(uiPort),
    m_sAddress(sAddress)
  {

  }

  std::string getAddress() const
  {
    return m_sAddress;
  }

  void setAddress(const std::string& sAddress)
  {
    m_sAddress = sAddress;
  }

  std::string getBindAddress() const
  {
    return m_sBindAddress;
  }
  void setBindAddress(const std::string& sAddress)
  {
    m_sBindAddress = sAddress;
  }

  uint16_t getPort() const
  {
    return m_uiPort;
  }
  void setPort(uint16_t uiPort)
  {
    m_uiPort = uiPort;
  }

  uint16_t getBindPort() const
  {
    return m_uiBindPort;
  }
  void setBindPort(uint16_t uiPort)
  {
    m_uiBindPort = uiPort;
  }

  std::string toString() const
  {
    std::ostringstream ostr;
    ostr << m_sBindAddress << ":" << m_uiBindPort << ":" << m_sAddress << ":" << m_uiPort;
    return ostr.str();
  }

private:
  // this can be used to bind to a local port before sending
  uint16_t m_uiBindPort;
  // this can be used to bind to a local interface or to bind the multicast address
  std::string m_sBindAddress;
  // local or remote port or multicast port
  uint16_t m_uiPort;
  // local or remote address or multicast address
  std::string m_sAddress;
};

static std::string toString(MediaInterfaceDescriptor_t mediaDescriptor)
{
  std::ostringstream ostr;
  ostr << "{\r\n";
  for (AddressTuple_t& item : mediaDescriptor)
  {
    AddressDescriptor desc1 = std::get<0>(item);
    AddressDescriptor desc2 = std::get<1>(item);
    ostr << desc1.toString() << ","
         << desc2.toString() << "\r\n";
  }
  ostr << "}";
  return ostr.str();
}

}
