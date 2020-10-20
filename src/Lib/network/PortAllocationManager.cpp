#include "CorePch.h"
#include <rtp++/network/PortAllocationManager.h>

namespace rtp_plus_plus
{

PortAllocationManager::PortAllocationManager(boost::asio::io_service& ioService)
  :m_ioService(ioService)
{

}

PortAllocationManager::~PortAllocationManager()
{
  VLOG(10) << "Destructor: UDP socket count: " << m_mUdpSockets.size() << " TCP: " << m_mTcpSockets.size();
}

boost::system::error_code PortAllocationManager::allocateUdpPort(const std::string& sAddress, uint16_t& uiPort, bool bIsPortMandatory)
{
  boost::system::error_code ec;
  std::unique_ptr<boost::asio::ip::udp::socket> pSocket = std::unique_ptr<boost::asio::ip::udp::socket>(new boost::asio::ip::udp::socket(m_ioService));
  pSocket->open(boost::asio::ip::udp::v4());
  // Binding to port 0 should result in the OS selecting the port
  VLOG(10) << "[" << this << "] Binding socket [" << sAddress << ":" << uiPort << "]";

  while (true)
  {
    /// Local end point that the socket should be bound to
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(sAddress), uiPort);
    pSocket->bind(endpoint, ec);
    if (ec)
    {
      LOG(WARNING) << "Failed to bind socket to port " << uiPort;
      if (bIsPortMandatory)
      {
        return ec;
      }
      else
      {
        if (ec == boost::asio::error::address_in_use)
        {
          // try next port
          ++uiPort;
          continue;
        }
        else
        {
          return ec;
        }
      }
    }
    else
    {
      std::ostringstream ostr;
      ostr << sAddress << ":" << uiPort;
      std::string id = ostr.str();
      // store socket
      m_mUdpSockets[id] = std::move(pSocket);
      return boost::system::error_code();
    }
  }
}

boost::system::error_code PortAllocationManager::allocateUdpPortsForRtpRtcp(const std::string& sAddress, uint16_t& uiPort, bool bArePortsMandatory)
{
  // make sure RTP port is even
  if (uiPort % 2 != 0)
  {
    LOG(WARNING) << "RTP port must be even: " << uiPort;
    return boost::system::error_code(boost::asio::error::invalid_argument);
  }

  boost::system::error_code ec;

  std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket = std::unique_ptr<boost::asio::ip::udp::socket>(new boost::asio::ip::udp::socket(m_ioService));
  std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket = std::unique_ptr<boost::asio::ip::udp::socket>(new boost::asio::ip::udp::socket(m_ioService));
  pRtpSocket->open(boost::asio::ip::udp::v4());
  pRtcpSocket->open(boost::asio::ip::udp::v4());

  // Binding to port 0 should result in the OS selecting the port
  VLOG(10) << "[" << this << "] Binding socket [" << sAddress << ":" << uiPort << "]";

  while (true)
  {
    /// Local end point that the socket should be bound to
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(sAddress), uiPort);
    pRtpSocket->bind(endpoint, ec);
    if (ec)
    {
      VLOG(5) << "Failed to bind RTP socket to port " << uiPort;
      if (bArePortsMandatory)
      {
        return ec;
      }
      else
      {
        if (ec == boost::asio::error::address_in_use)
        {
          // try next port
          uiPort += 2;
          VLOG(5) << "Trying next ports " << uiPort;
          continue;
        }
        else
        {
          LOG(WARNING) << "Error allocating RTP port: " << ec.message();
          return ec;
        }
      }
    }
    else
    {
      VLOG(5) << "RTP socket bound to port " << uiPort;

      // try allocate bind RTCP socket
      boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(sAddress), uiPort + 1);
      pRtcpSocket->bind(endpoint, ec);
      if (ec)
      {
        VLOG(5) << "Failed to bind RTCP socket to port " << uiPort + 1;
        if (bArePortsMandatory)
        {
          return ec;
        }
        else
        {
          if (ec == boost::asio::error::address_in_use)
          {
            // try next port
            uiPort += 2;
            VLOG(5) << "Trying next ports " << uiPort;
            // allocate new socket otherwise next bind will fail
            pRtpSocket = std::unique_ptr<boost::asio::ip::udp::socket>(new boost::asio::ip::udp::socket(m_ioService));
            pRtpSocket->open(boost::asio::ip::udp::v4());
            continue;
          }
          else
          {
            LOG(WARNING) << "Error allocating RTCP port: " << ec.message();
            return ec;
          }
        }
      }
      else
      {
        VLOG(5) << "RTCP socket bound to port " << uiPort + 1;
        // Success: store RTP and RTCP sockets
        std::ostringstream ostr;
        ostr << sAddress << ":" << uiPort;
        std::string rtp_id = ostr.str();
        std::ostringstream ostr2;
        ostr2 << sAddress << ":" << (uiPort + 1);
        std::string rtcp_id = ostr2.str();
        // store socket
        m_mUdpSockets[rtp_id] = std::move(pRtpSocket);
        m_mUdpSockets[rtcp_id] = std::move(pRtcpSocket);
        return boost::system::error_code();
      }
    }
  }
}

std::unique_ptr<boost::asio::ip::udp::socket> PortAllocationManager::getBoundUdpSocket(const std::string& sAddress, const uint16_t& uiPort)
{
  std::ostringstream ostr;
  ostr << sAddress << ":" << uiPort;
  std::string id = ostr.str();
  auto it = m_mUdpSockets.find(id);
  if (it == m_mUdpSockets.end())
    return std::unique_ptr<boost::asio::ip::udp::socket>();
  else
  {
    std::unique_ptr<boost::asio::ip::udp::socket> pSocket = std::move(it->second);
    // remove from map
    std::size_t uiRemoved = m_mUdpSockets.erase(id);
    assert(uiRemoved > 0);
    return pSocket;
  }
}

} // rtp_plus_plus
