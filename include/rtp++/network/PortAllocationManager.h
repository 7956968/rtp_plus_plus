#pragma once
#include <cstdint>
#include <unordered_map>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/noncopyable.hpp>
#include <rtp++/network/EndPoint.h>

namespace rtp_plus_plus
{

/**
 * @brief PortAllocationManager is used to allocate UDP and TCP ports.
 *
 * Once binding the underlying socket, the socket is stored and can 
 * be retrieved with the retrieveSocket method. 
 */
class PortAllocationManager : public boost::noncopyable
{
public:
  /**
   * @brief Constructor
   * @param[in] ioService The io service to be used for socket operations.
   */
  PortAllocationManager(boost::asio::io_service& ioService);
  /**
   * @brief Destructor
   */
  ~PortAllocationManager();
  /**
   * @brief clears the allocated sockets.
   *
   */
  void clear()
  {
    m_mUdpSockets.clear();
    m_mTcpSockets.clear();
  }
  /**
   * @brief attempts to allocate a UDP port for use. 
   * @param[in, out] sAddress The interface address that should be attempted to be bound.
   * @param[in, out] uiPort The start port that should be attempted to be bound. If the method is
   * successful, this is set to the port number that was successfully bound.
   * If (bIsPortMandatory == true) the method returns false, if the manager was not able to bind
   * the requested port. If (bIsPortMandatory == false) then the port allocation manager attempts
   * to bind ports by incrementing uiPort. Once a port is found, uiPort is set to this value and 
   * the method returns true.
   * @param[in] bIsPortMandatory If this is mandatory, the method will fail if it is unable to bind 
   * to the specified port
   * @return returns no error if the method was successful in binding to the requested UDP port.
   */
  boost::system::error_code allocateUdpPort(const std::string& sAddress, uint16_t& uiPort, bool bIsPortMandatory);
  /**
   * @brief attempts to allocates two UDP port for use with the added restrictions that the RTP port
   * must be on an even port number, and the RTCP port is the next higher port.
   * @param[in, out] sAddress The interface address that should be attempted to be bound.
   * @param[in, out] uiPort The start port that should be attempted to be bound. If the method is successful, 
   * this is set to the RTP port that was successfully bound.
   * If (bIsPortMandatory == true) the method returns false, if the manager was not able to bind
   * the requested ports where the RTCP port is one bigger than uiPort. 
   * If (bIsPortMandatory == false) then the port allocation manager attempts
   * to bind ports by incrementing uiPort and looking for two subsequent available ports. 
   * Once ports are found, uiPort is set to the RTP port and the method returns true.
   * @param[in] bIsPortMandatory If this is mandatory, the method will fail if it is unable to bind 
   * to the specified ports
   * @return no error if the method was successful in binding to the requested UDP ports.
   */
  boost::system::error_code allocateUdpPortsForRtpRtcp(const std::string& sAddress, uint16_t& uiPort, bool bArePortsMandatory);
  /**
   * @brief retrieves a previously allocated port. This removes ownership of the socket from the 
   * PortAllocationManager.
   *
   * @param uiPort The port that had previously been allocated and bound by the PortAllocationManager.
   * @return A unique_ptr to the previously allocated socket if it exists and a null ptr otherwise. 
   */
  std::unique_ptr<boost::asio::ip::udp::socket> getBoundUdpSocket(const std::string& sAddress, const uint16_t& uiPort);

private:

  boost::asio::io_service& m_ioService;

  std::unordered_map < std::string, std::unique_ptr<boost::asio::ip::udp::socket> > m_mUdpSockets;
  std::unordered_map < std::string, std::unique_ptr<boost::asio::ip::tcp::socket> > m_mTcpSockets;

};

} // rtp_plus_plus
