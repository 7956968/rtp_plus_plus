#pragma once
#include <rtp++/rfc4571/TcpRtpNetworkInterface.h>

namespace rtp_plus_plus {
namespace rfc4571 {

class PassiveTcpRtpNetworkInterface : public TcpRtpNetworkInterface
{
public:

  PassiveTcpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp );

  virtual void shutdown();
  virtual TcpRtpConnection::ptr getConnection(const EndPoint& ep, bool bRtp);

protected:
    /// helper method
    void initAccept(boost::asio::ip::tcp::acceptor& acceptor, const EndPoint& ep, TcpRtpConnection::ptr &pNewConnection);
    /// Initiate an asynchronous accept operation.
    void startAccept(boost::asio::ip::tcp::acceptor& acceptor, TcpRtpConnection::ptr &pNewConnection);

    /// Handle completion of an asynchronous accept operation.
    void handleAccept(const boost::system::error_code& e, boost::asio::ip::tcp::acceptor& acceptor, TcpRtpConnection::ptr pNewConnection);

private:
  /// The signal_set is used to register for process termination notifications.
  //boost::asio::signal_set signals_;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor m_acceptorRtp;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor m_acceptorRtcp;

//  EndPoint m_localRtpEp;
//  EndPoint m_localRtcpEp;

  /// The next connection to be accepted.
  TcpRtpConnection::ptr m_pNewConnectionRtp;
  /// The next connection to be accepted.
  TcpRtpConnection::ptr m_pNewConnectionRtcp;

  std::set<TcpRtpConnection::ptr> m_serverConnections;
};

} // rfc4571
} // rtp_plus_plus
