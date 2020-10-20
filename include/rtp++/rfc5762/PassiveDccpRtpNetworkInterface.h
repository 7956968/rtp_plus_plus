#pragma once
#ifdef ENABLE_ASIO_DCCP
#include <rtp++/rfc5762/DccpRtpNetworkInterface.h>

namespace rtp_plus_plus
{
namespace rfc5762
{

class PassiveDccpRtpNetworkInterface : public DccpRtpNetworkInterface
{
public:

  PassiveDccpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rDccpEp );

  virtual void shutdown();
  virtual DccpRtpConnection::ptr getConnection(const EndPoint& ep, bool bRtp);

protected:
    /// helper method
    void initAccept(boost::asio::ip::dccp::acceptor& acceptor, const EndPoint& ep, DccpRtpConnection::ptr& pNewConnection);
    /// Initiate an asynchronous accept operation.
    void startAccept(boost::asio::ip::dccp::acceptor& acceptor, DccpRtpConnection::ptr& pNewConnection);

    /// Handle completion of an asynchronous accept operation.
    void handleAccept(const boost::system::error_code& e, boost::asio::ip::dccp::acceptor& acceptor, DccpRtpConnection::ptr pNewConnection);

private:
  /// The signal_set is used to register for process termination notifications.
  //boost::asio::signal_set signals_;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::dccp::acceptor m_acceptorRtp;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::dccp::acceptor m_acceptorRtcp;

//  EndPoint m_localRtpEp;
//  EndPoint m_localRDccpEp;

  /// The next connection to be accepted.
  DccpRtpConnection::ptr m_pNewConnectionRtp;
  /// The next connection to be accepted.
  DccpRtpConnection::ptr m_pNewConnectionRtcp;

  std::set<DccpRtpConnection::ptr> m_serverConnections;
};

}
}
#endif