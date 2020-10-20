#pragma once
#include <string>
#include <utility>
#include <cpputil/Conversion.h>
#include <rtp++/network/AddressDescriptor.h>

namespace rtp_plus_plus
{

class EndPoint;
typedef std::pair<EndPoint, EndPoint> EndPointPair_t;

/**
 * @brief The EndPoint class represents an IP end point.
 *
 * The initial version will only support IP4. Support for
 * IPv6 may be added at a later stage if required.
 */
class EndPoint
{
public:
  /**
   * @brief Constructor
   */
  EndPoint()
    :m_sAddress("0.0.0.0"),
      m_uiPort(0),
      m_uiId(0)
  {

  }

  /**
   * @brief Constructor
   */
  EndPoint(const std::string& sAddress, uint16_t uiPort)
    :m_sAddress(sAddress),
    m_uiPort(uiPort),
    m_uiId(0)
  {

  }

  /**
   * @brief Convenience constructor
   */
  EndPoint(const AddressDescriptor& rAddress)
      :m_sAddress(rAddress.getAddress()),
      m_uiPort(rAddress.getPort()),
      m_uiId(0)
  {

  }

  bool isValid() const { return m_uiPort != 0; }

  std::string getAddress() const { return m_sAddress; } 
  uint16_t getPort() const { return m_uiPort; }
  uint32_t getId() const { return m_uiId; }

  void setAddress(const std::string& sVal) { m_sAddress = sVal; }
  void setPort(const uint16_t uiPort) { m_uiPort = uiPort; }
  void setId(uint32_t uiId) { m_uiId = uiId; } 

  std::string toString() const { return m_sAddress + ":" + ::toString(m_uiPort); }
private:
  std::string m_sAddress;
  uint16_t m_uiPort;
  // optional ID 
  uint32_t m_uiId;
};

inline std::ostream& operator<< (std::ostream& ostr, const EndPoint& ep)
{
  ostr << ep.getAddress() << ":" << ep.getPort();
  return ostr;
}

} // rtp_plus_plus
