#pragma once
#include <boost/function.hpp>
#include <rtp++/rfc2326/RtspServerConnection.h>

namespace rtp_plus_plus
{

/**
 * @brief SelfRegisteringRtspServerConnection extends RtspServerConnection to
 * send REGISTER requests to an RTSP proxy or client.
 */
class SelfRegisteringRtspServerConnection : public rfc2326::RtspServerConnection
{
public:
  typedef boost::shared_ptr<SelfRegisteringRtspServerConnection> ptr;
  typedef boost::function<void(const boost::system::error_code&, SelfRegisteringRtspServerConnection::ptr)> ConnectErrorHandler;

  static ptr create(boost::asio::io_service& io_service, const std::string& sIpOrHostName, uint16_t uiPort);
  SelfRegisteringRtspServerConnection(boost::asio::io_service& io_service, const std::string& sIpOrHostName, uint16_t uiPort);
  ~SelfRegisteringRtspServerConnection();

  void setConnectErrorHandler(ConnectErrorHandler val) { m_connectErrorHandler = val; }

  // starts reading
  boost::system::error_code connect();

protected:

  void handleConnect(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpointIterator);

private:
  std::string m_sIpOrHostName;
  uint16_t m_uiPort;

  ConnectErrorHandler m_connectErrorHandler;
};

} // rtp_plus_plus