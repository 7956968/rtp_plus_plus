#include "CorePch.h"
#include <rtp++/rfc4571/PassiveTcpRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc4571
{

PassiveTcpRtpNetworkInterface::PassiveTcpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp )
  : TcpRtpNetworkInterface(rIoService, rtpEp, rtcpEp), // for the passive connection we don't specify remote end-point
    m_acceptorRtp(rIoService),
    m_acceptorRtcp(rIoService)
{
//  // Register to handle the signals that indicate when the server should exit.
//  // It is safe to register for the same signal multiple times in a program,
//  // provided all registration for the specified signal is made through Asio.
//  signals_.add(SIGINT);
//  signals_.add(SIGTERM);
//#if defined(SIGQUIT)
//  signals_.add(SIGQUIT);
//#endif // defined(SIGQUIT)
//  signals_.async_wait(boost::bind(&PassiveTcpRtpNetworkInterface::shutdown, this));

  VLOG(2) << "RTP connection: " << m_pNewConnectionRtp << " RTCP: " << m_pNewConnectionRtcp;
  initAccept(m_acceptorRtp, m_rtpEp, m_pNewConnectionRtp);
  initAccept(m_acceptorRtcp, m_rtcpEp, m_pNewConnectionRtcp);
}

TcpRtpConnection::ptr PassiveTcpRtpNetworkInterface::getConnection(const EndPoint& ep, bool bRtp)
{
#if 0
  VLOG(2) << "DEBUG: calling PassiveTcpRtpNetworkInterface::getConnection EP: " << ep << " RTP: " << bRtp;
  if (!m_pNewConnectionRtp)
  {
    LOG(WARNING) << "RTP null connection";
  }
  if (!m_pNewConnectionRtcp)
  {
    LOG(WARNING) << "RTCP null connection";
  }
#endif
  // HACK: this only handles a single participant.
  // If the connection has taken place, then return the connection.
  if (bRtp)
  {
    return m_pNewConnectionRtp;
  }
  else
  {
    return m_pNewConnectionRtcp;
  }
}

void PassiveTcpRtpNetworkInterface::initAccept(boost::asio::ip::tcp::acceptor& acceptor, const EndPoint& ep, TcpRtpConnection::ptr& pNewConnection)
{
  boost::asio::ip::tcp::resolver resolver(m_rIoService);
#define BIND_TO_0_0_0_0_FOR_EC2_NAT_TRAVERSAL
#ifdef BIND_TO_0_0_0_0_FOR_EC2_NAT_TRAVERSAL
  // overriding local IP with 0.0.0.0 to get around EC2 cloud NAT traversal
  EndPoint& tempEp = const_cast<EndPoint&>(ep);
  tempEp.setAddress("0.0.0.0");
#endif
  boost::asio::ip::tcp::resolver::query query(ep.getAddress(), ::toString(ep.getPort()));
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor.open(endpoint.protocol());
  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  VLOG(2) << "Binding acceptor to endpoint " << ep;
  acceptor.bind(endpoint);
  acceptor.listen();
  startAccept(acceptor, pNewConnection);
}

void PassiveTcpRtpNetworkInterface::shutdown()
{
  TcpRtpNetworkInterface::shutdown();

  m_acceptorRtp.close();
  m_acceptorRtcp.close();

    // shutdown all connections
  for (auto& pConnection: m_serverConnections)
  {
    pConnection->close();
  }
  m_serverConnections.clear();

}

void PassiveTcpRtpNetworkInterface::startAccept(boost::asio::ip::tcp::acceptor& acceptor, TcpRtpConnection::ptr& pNewConnection)
{
  pNewConnection.reset(new TcpRtpConnection(m_rIoService));

  // configure handlers here so that we don't need an "accept" to end the application cleanly
  if (&acceptor == &m_acceptorRtp)
  {
    pNewConnection->onRecv(boost::bind(&PassiveTcpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveTcpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
  }
  else
  {
    pNewConnection->onRecv(boost::bind(&PassiveTcpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveTcpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
  }


  acceptor.async_accept(pNewConnection->socket(),
      boost::bind(&PassiveTcpRtpNetworkInterface::handleAccept, this,
      boost::asio::placeholders::error, boost::ref(acceptor), pNewConnection));
}

void PassiveTcpRtpNetworkInterface::handleAccept(const boost::system::error_code& e, boost::asio::ip::tcp::acceptor& acceptor, TcpRtpConnection::ptr pNewConnection)
{
  // Check whether the server was stopped by a signal before this completion
  // handler had a chance to run.
  if (!acceptor.is_open())
  {
    LOG(WARNING) << "Error: acceptor is not open";
    return;
  }

  if (!e)
  {
    VLOG(2) << "Starting connection";
    pNewConnection->start();
  }
  else
  {
    LOG(WARNING) << "Error in handleAccept";
  }

  // TOREMOVE
  // TODO: we could not start a new accept here since we only expect one TCP connection per RTP session
  // start next accept
#if 1
  if (&acceptor == &m_acceptorRtp)
  {
    pNewConnection->onRecv(boost::bind(&PassiveTcpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveTcpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
//    startAccept(acceptor, m_pNewConnectionRtp);
  }
  else
  {
    pNewConnection->onRecv(boost::bind(&PassiveTcpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveTcpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
//    startAccept(acceptor, m_pNewConnectionRtcp);
  }
#endif
}

}
}
