#pragma once
#include <boost/optional.hpp>
#include <rtp++/network/PortAllocationManager.h>
#include <rtp++/network/UdpSocketWrapper.h>
#include <rtp++/network/TcpRtpConnection.h>
#include <rtp++/rfc2326/RtspClientConnection.h>

namespace rtp_plus_plus
{

/**
 * @brief The ExistingConnectionAdapter class is an adapter for scenarios in which the UDP, TCP or RTSP connections are created
 * before the RtpSession object.
 *
 * It is assumed that there will only be one connection type identified by m_eType.
 * Other members will be ignored.
 *
 * Note: keeping members public for now for easy access
 */
class ExistingConnectionAdapter
{
  enum ConnectionType
  {
    CT_NONE,
    CT_UDP,
    CT_TCP,
    CT_RTSP,
    CT_PORT_MANAGER
  };
public:

  /**
   * @brief Constructor
   */
  ExistingConnectionAdapter()
    :m_eType(CT_NONE),
    m_pPortManager(NULL)
  {

  }
  /**
   * @brief Constructor that takes PortAllocationManager
   */
  ExistingConnectionAdapter(PortAllocationManager* pPortManager)
    :m_eType(CT_PORT_MANAGER),
    m_pPortManager(pPortManager)
  {

  }
  /**
   * @brief Checks if the adapter has been configured to use an existing connection
   */
  bool useExistingConnection() const { return m_eType != CT_NONE; }
  /**
   * @brief Getter for if RTSP connection should be reused
   */
  bool reuseRtspConnection() const { return m_eType == ExistingConnectionAdapter::CT_RTSP; }
  /**
   * @brief Getter for if port manager should be used
   */
  bool usePortManager() const { return m_eType == ExistingConnectionAdapter::CT_PORT_MANAGER; }
  /**
   * @brief Getter for RtspClientConnection object
   *
   * This should only be called of m_eType == CT_RTSP.
   */
  rfc2326::RtspClientConnection::ptr  getRtspClientConnection() const { return m_pRtspClientConnection; }
  /**
   * @brief Setter for RtspClientConnection object
   * 
   * This allows the RtpSession to reuse the TCP connection. Calling this method changes the m_eType to CT_RTSP.
   */
  void setRtspClientConnection(rfc2326::RtspClientConnection::ptr pRtspClientConnection)
  {
    m_eType = ExistingConnectionAdapter::CT_RTSP;
    m_pRtspClientConnection = pRtspClientConnection;
  }
  /**
   * @brief Getter for port manager
   *
   * This method should only be called when m_eType is CT_PORT_MANAGER. This is the case when the Contructor 
   * that takes a port manager has been used
   */
  PortAllocationManager* getPortManager() { return m_pPortManager; }
  /**
   * @brief Setter for port manager
   *
   * 
   */
  void setPortManager(PortAllocationManager* pPortManager)
  {
    m_eType = CT_PORT_MANAGER;
    m_pPortManager = pPortManager;
  }

  ConnectionType m_eType;
  std::vector<UdpSocketWrapper::ptr> m_vUdpSockets;
  std::vector<TcpRtpConnection::ptr> m_vTcpConnections;
  rfc2326::RtspClientConnectionPtr m_pRtspClientConnection;
  PortAllocationManager* m_pPortManager;
};

} //rtp_plus_plus
