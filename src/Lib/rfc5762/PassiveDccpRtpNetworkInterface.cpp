#include "CorePch.h"
#include <rtp++/rfc5762/PassiveDccpRtpNetworkInterface.h>
#ifdef ENABLE_ASIO_DCCP
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc5762
{

PassiveDccpRtpNetworkInterface::PassiveDccpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp )
  : DccpRtpNetworkInterface(rIoService, rtpEp, rtcpEp), // for the passive connection we don't specify remote end-point
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
//  signals_.async_wait(boost::bind(&PassiveDccpRtpNetworkInterface::shutdown, this));

  initAccept(m_acceptorRtp, m_rtpEp, m_pNewConnectionRtp);
  initAccept(m_acceptorRtcp, m_rtcpEp, m_pNewConnectionRtcp);
}

DccpRtpConnection::ptr PassiveDccpRtpNetworkInterface::getConnection(const EndPoint& ep, bool bRtp)
{
#if 1
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

  void PassiveDccpRtpNetworkInterface::initAccept(boost::asio::ip::dccp::acceptor& acceptor, const EndPoint& ep, DccpRtpConnection::ptr &pNewConnection)
  {
    boost::asio::ip::dccp::resolver resolver(m_rIoService);
    boost::asio::ip::dccp::resolver::query query(ep.getAddress(), ::toString(ep.getPort()));
    boost::asio::ip::dccp::endpoint endpoint = *resolver.resolve(query);
    acceptor.open(endpoint.protocol());
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    acceptor.set_option(boost::asio::ip::dccp::acceptor::reuse_address(true));
    VLOG(2) << "Binding acceptor to endpoint " << ep;
    acceptor.bind(endpoint);
    acceptor.listen();
    startAccept(acceptor, pNewConnection);
  }

void PassiveDccpRtpNetworkInterface::shutdown()
{
  DccpRtpNetworkInterface::shutdown();

  m_acceptorRtp.close();
  m_acceptorRtcp.close();

    // shutdown all connections
  for (auto& pConnection: m_serverConnections)
  {
    pConnection->close();
  }
  m_serverConnections.clear();

}

void PassiveDccpRtpNetworkInterface::startAccept(boost::asio::ip::dccp::acceptor& acceptor, DccpRtpConnection::ptr &pNewConnection)
{
  pNewConnection.reset(new DccpRtpConnection(m_rIoService));

  acceptor.async_accept(pNewConnection->socket(),
      boost::bind(&PassiveDccpRtpNetworkInterface::handleAccept, this,
      boost::asio::placeholders::error, boost::ref(acceptor), pNewConnection));
}

void PassiveDccpRtpNetworkInterface::handleAccept(const boost::system::error_code& e, boost::asio::ip::dccp::acceptor& acceptor, DccpRtpConnection::ptr pNewConnection)
{
  // Check whether the server was stopped by a signal before this completion
  // handler had a chance to run.
  if (!acceptor.is_open())
  {
    return;
  }

  if (!e)
  {
    pNewConnection->start();
  }

  // TOREMOVE
  // TODO: we could not start a new accept here since we only expect one TCP connection per RTP session
  // start next accept
#if 1
  if (&acceptor == &m_acceptorRtp)
  {
    pNewConnection->onRecv(boost::bind(&PassiveDccpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveDccpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
//    startAccept(acceptor, m_pNewConnectionRtp);
  }
  else
  {
    pNewConnection->onRecv(boost::bind(&PassiveDccpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
    pNewConnection->onSendComplete(boost::bind(&PassiveDccpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
//    startAccept(acceptor, m_pNewConnectionRtcp);
  }
#endif
}

}
}
#endif