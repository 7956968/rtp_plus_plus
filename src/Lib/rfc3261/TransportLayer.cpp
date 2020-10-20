#include "CorePch.h"
#include <rtp++/rfc3261/TransportLayer.h>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <rtp++/rfc3261/Rfc3261.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

#define REQUIRED_BUFFER_SIZE 200
#define MAX_UNKNOWN_MTU 1300

enum OUTGOING_CONNECTION_TYPE
{
  TCP_SERVER_SOCKET = 0,
  TCP_CLIENT_SOCKET = 1,
  UDP_SOCKET        = 2
};

using boost::asio::ip::tcp;

TransportLayer::TransportLayer(boost::asio::io_service& ioService, SipMessageHandler_t messageHandler, const std::string& sFQDN, uint16_t uiSipPort, uint32_t uiMtu)
  :m_ioService(ioService),
  m_messageHandler(messageHandler),
  m_sFQDN(sFQDN),
  m_uiSipPort(uiSipPort),
  m_uiUdpMaxMessageSize(uiMtu == 0 ? MAX_UNKNOWN_MTU : uiMtu - REQUIRED_BUFFER_SIZE)
{
  assert(uiMtu > REQUIRED_BUFFER_SIZE);
  // TODO: if not set, try detect and just insert any IP of host
  assert(!m_sFQDN.empty());
  VLOG(6) << "TODO: detect/verify IP/FQDN?!?";
}

TransportLayer::~TransportLayer()
{
  VLOG(10) << "Destructor";
}

boost::system::error_code TransportLayer::addLocalTransport(const std::string& sInterface, uint16_t uiPort,
  TransactionProtocol eProtocol)
{
  for (Transport_t& t : m_vTransports)
  {
    if ((std::get<0>(t) == sInterface) &&
      (std::get<1>(t) == uiPort) &&
      (std::get<2>(t) == eProtocol))
    {
      return boost::system::error_code(boost::system::errc::already_connected, boost::system::generic_category());
    }
    // check for binding to 0.0.0.0 in which case interface is already bound anyway
    if ((std::get<1>(t) == uiPort) &&
      (std::get<2>(t) == eProtocol))
    {
      if ((std::get<0>(t) == "0.0.0.0") ||
        (sInterface == "0.0.0.0"))
      {
        return boost::system::error_code(boost::system::errc::already_connected, boost::system::generic_category());
      }
    }
  }

  Transport_t t = std::make_tuple(sInterface, uiPort, eProtocol);
  ConnectionId_t connectionId;
  boost::system::error_code ec = createLocalTransportSocket(t);
  if (ec)
  {
    LOG(WARNING) << "Error creating transport: " << ec.message();
    return ec;
  }
  else
  {
    m_vTransports.push_back(t);
    m_mConnectionMap[t] = connectionId;
  }

  return boost::system::error_code();
}

bool TransportLayer::hasTransport(const std::string& sInterface, uint16_t uiPort, TransactionProtocol eProtocol) const
{
  for (const Transport_t& t : m_vTransports)
  {
    if ((std::get<0>(t) == sInterface) &&
      (std::get<1>(t) == uiPort) &&
      (std::get<2>(t) == eProtocol))
    {
      return true;
    }
  }
  return false;
}

boost::system::error_code TransportLayer::start()
{
  if (m_vTransports.empty())
  {
    boost::system::error_code  ec = addLocalTransport("0.0.0.0", m_uiSipPort, TP_TCP);
    if (ec)
    {
      LOG(WARNING) << "Error adding TCP transport: " << ec.message();
      return ec;
    }
    ec = addLocalTransport("0.0.0.0", m_uiSipPort, TP_UDP);
    if (ec)
    {
      LOG(WARNING) << "Error adding TCP transport: " << ec.message();
      return ec;
    }
  }

#if 0
  // initialise all registered transports
  for (Transport_t& t : m_vTransports)
  {
    boost::system::error_code ec = createTransportSocket(t);
    if (ec)
    {
      LOG(WARNING) << "Error creating transport: " << ec.message();
      return ec;
    }
  }
#endif

  return boost::system::error_code();
}

boost::system::error_code TransportLayer::createLocalTransportSocket(const Transport_t& t)
{
  std::string sInterface = std::get<0>(t);
  uint16_t uiPort = std::get<1>(t);
  TransactionProtocol tp = std::get<2>(t);

  switch (tp)
  {
  case TP_TCP:
  {
    boost::system::error_code ec;
    boost::shared_ptr<tcp::acceptor> pTcpAcceptor = boost::make_shared<tcp::acceptor>(m_ioService);
    // bind to port
    boost::asio::ip::tcp::endpoint endpoint = tcp::endpoint(tcp::v4(), uiPort);
    pTcpAcceptor->open(endpoint.protocol(), ec);
    if (ec)
    {
      LOG(WARNING) << "Error creating TCP acceptor for " << sInterface << ":" << uiPort << " - " << ec.message();
      return ec;
    }
    pTcpAcceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
      LOG(WARNING) << "Error setting reuse_address for TCP acceptor for " << sInterface << ":" << uiPort << " - " << ec.message();
      return ec;
    }
    pTcpAcceptor->bind(endpoint, ec);
    if (ec)
    {
      LOG(WARNING) << "Error binding to socket reuse_address for TCP acceptor for " << sInterface << ":" << uiPort << " - " << ec.message();
      return ec;
    }
    pTcpAcceptor->listen();

    // pass through index in vector
    m_vAcceptors.push_back(std::move(pTcpAcceptor));
    startAccept(t, m_vAcceptors.size() - 1);
    return boost::system::error_code();
    break;
  }
  case TP_UDP:
  {
    boost::system::error_code ec;
    UdpSocketWrapper::ptr pUdpSocket = UdpSocketWrapper::create(m_ioService, sInterface, uiPort, ec);
    if (ec)
    {
      LOG(WARNING) << "Error creating UDP socket for " << sInterface << ":" << uiPort << " - " << ec.message();
      return ec;
    }
    // set callbacks
    pUdpSocket->onRecv(boost::bind(&TransportLayer::handleIncomingUdpPacket,
      this, _1, _2, _3, _4));
    pUdpSocket->onSendComplete(boost::bind(&TransportLayer::handleSentUdpPacket,
      this, _1, _2, _3, _4));

    pUdpSocket->recv();
    m_vUdpSockets.push_back(pUdpSocket);
    return boost::system::error_code();
  }
  default:
  {
    LOG(WARNING) << "Unsupported protocol";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }
  }
}

boost::system::error_code TransportLayer::stop()
{
  VLOG(6) << "TransportLayer::stop()";
  // close all TCP acceptors
  for (auto pAcceptor : m_vAcceptors)
  {
    VLOG(5) << "Closing acceptors";
    pAcceptor->close();
  }
  // close TCP connections
  for (auto pTcpConnection : m_vTcpConnections)
  {
    VLOG(5) << "Closing incoming TCP connections";
    pTcpConnection->close();
  }
  for (auto pTcpConnection : m_vTcpClientConnections)
  {
    VLOG(5) << "Closing outgoing TCP connections";
    pTcpConnection->close();
  }
  // close UDP sockets
  for (auto pUdpSocket : m_vUdpSockets)
  {
    VLOG(5) << "Closing UDP sockets";
    pUdpSocket->close();
  }
  VLOG(6) << "TransportLayer::stop() done";
  return boost::system::error_code();
}

void TransportLayer::startAccept(const Transport_t& transport, uint32_t index)
{
  VLOG(5) << "Transport layer listening for connections on port " << std::get<1>(transport);

  SipServerTcpConnection::ptr pNewConnection = SipServerTcpConnection::create(m_ioService);
  pNewConnection->setReadCompletionHandler(boost::bind(&TransportLayer::tcpReadCompletionHandler, this, boost::asio::placeholders::error, _2, _3));
  pNewConnection->setWriteCompletionHandler(boost::bind(&TransportLayer::tcpWriteCompletionHandler, this, boost::asio::placeholders::error, _2, _3));
  pNewConnection->setCloseHandler(boost::bind(&TransportLayer::tcpCloseCompletionHandler, this, _1));

  m_vAcceptors[index]->async_accept(pNewConnection->socket(),
    boost::bind(&TransportLayer::acceptHandler, this,
    boost::asio::placeholders::error, pNewConnection, transport, index));
}

void TransportLayer::acceptHandler(const boost::system::error_code& error, SipServerTcpConnection::ptr pNewConnection, Transport_t& transport, uint32_t index)
{
  if (!error)
  {
    VLOG(2) << "Starting new connection: " << pNewConnection->getId();
    // handle new connection
    pNewConnection->start();
    // store pointer to connection so that we can close them all on shutdown
    m_vTcpConnections.push_back(pNewConnection);

    Transport_t transport = make_tuple(pNewConnection->getClientIp(), pNewConnection->getClientPort(), TP_TCP);
    m_mConnectionMap[transport] = std::make_pair(TCP_SERVER_SOCKET, m_vTcpConnections.size() - 1);
    // listen for new connections
    startAccept(transport, index);
  }
  else
  {
    // The operation will be aborted on close.
    if (error != boost::asio::error::operation_aborted)
      LOG(WARNING) << "Error in accept: " << error.message();
  }
}

void TransportLayer::tcpReadCompletionHandler(const boost::system::error_code& ec, const std::string& sSipMessage, SipServerTcpConnection::ptr pTcpConnection)
{
  VLOG(10) << "TransportLayer::tcpReadCompletionHandler";
  if (!ec)
  {
    // parse SIP message and pass on using message handler
    boost::optional<SipMessage> pSipMessage = SipMessage::create(sSipMessage);
    if (pSipMessage)
    {
      assert(m_messageHandler);
      m_messageHandler(*pSipMessage, pTcpConnection->getClientIp(), pTcpConnection->getClientPort(), TP_TCP);
    }
    else
    {
      VLOG(5) << "Failed to parse TCP SIP message: " << sSipMessage;
    }
  }
  else
  {
    LOG(WARNING) << "TODO: Error in read completion handler: " << ec.message();
  }
}

void TransportLayer::tcpWriteCompletionHandler(const boost::system::error_code& ec, const std::string& sSipMessage, SipServerTcpConnection::ptr pTcpConnection)
{
  VLOG(10) << "TransportLayer::tcpWriteCompletionHandler";
  if (!ec)
  {

  }
  else
  {
    LOG(WARNING) << "TODO: Error in write completion handler: " << ec.message();
  }
}

void TransportLayer::tcpCloseCompletionHandler(SipServerTcpConnection::ptr pTcpConnection)
{
  VLOG(2) << "TCP connection closed: " << pTcpConnection->getClientIp() << ":" << pTcpConnection->getClientPort();
}

void TransportLayer::tcpClientReadCompletionHandler(const boost::system::error_code& ec, const std::string& sSipMessage, SipClientTcpConnection::ptr pTcpConnection)
{
  VLOG(10) << "TransportLayer::tcpClientReadCompletionHandler";
  if (!ec)
  {
    // parse SIP message and pass on using message handler
    boost::optional<SipMessage> pSipMessage = SipMessage::create(sSipMessage);
    if (pSipMessage)
    {
//      VLOG(10) << "Parsed TCP SIP message: " << sSipMessage;
      assert(m_messageHandler);
      m_messageHandler(*pSipMessage, pTcpConnection->getIpOrHost(), pTcpConnection->getPort(), TP_TCP);
    }
    else
    {
      VLOG(5) << "Failed to parse TCP SIP message: " << sSipMessage;
    }
  }
  else
  {
    LOG(WARNING) << "TODO: Error in read completion handler: " << ec.message();
  }
}

void TransportLayer::tcpClientWriteCompletionHandler(const boost::system::error_code& ec, const std::string& sSipMessage, SipClientTcpConnection::ptr pTcpConnection)
{
  VLOG(10) << "TransportLayer::tcpClientWriteCompletionHandler";
  if (!ec)
  {

  }
  else
  {
    LOG(WARNING) << "TODO: Error in write completion handler: " << ec.message();
  }
}

void TransportLayer::tcpClientCloseCompletionHandler(SipClientTcpConnection::ptr pTcpConnection)
{
  VLOG(2) << "TCP client connection closed: " << pTcpConnection->getIpOrHost() << ":" << pTcpConnection->getPort();
}

void TransportLayer::handleIncomingUdpPacket(const boost::system::error_code& ec,
  UdpSocketWrapper::ptr pSource,
  NetworkPacket networkPacket,
  const EndPoint& ep)
{
  VLOG(12) << "Incoming UDP packet.";
  if (!ec)
  {
    const std::string sSipMessage = networkPacket.toStdString();
    if (sSipMessage.length() == 4)
    {
      VLOG(6) << "TODO: Sip keep-alive?";
    }
    else
    {
      // parse SIP message and pass on using message handler
      boost::optional<SipMessage> pSipMessage = SipMessage::create(sSipMessage);
      if (pSipMessage)
      {
        VLOG(2) << "Received from [" << ep << "]: \n" << sSipMessage;
        assert(m_messageHandler);
        m_messageHandler(*pSipMessage, ep.getAddress(), ep.getPort(), TP_UDP);
      }
      else
      {
        VLOG(5) << "Failed to parse UDP SIP message: " << sSipMessage;
      }
    }
    // start next receive
    pSource->recv();
  }
  else
  {
    LOG(WARNING) << "TODO: Error in UDP read: " << ec.message();
    if (ec != boost::asio::error::operation_aborted)
    {
      // start next receive
      pSource->recv();
    }
    else
    {
      // we're probably shutting down
    }
  }
}

void TransportLayer::handleSentUdpPacket(const boost::system::error_code& ec,
  UdpSocketWrapper::ptr pSource,
  Buffer buffer,
  const EndPoint& ep)
{
  if (!ec)
  {
    VLOG(12) << "UDP packet sent.";
  }
  else
  {
    LOG(WARNING) << "TODO: Error in UDP write: " << ec.message();
  }
}

boost::system::error_code TransportLayer::getOutgoingConnection(const std::string& sInterface, uint16_t uiPort, TransactionProtocol eProtocol, ConnectionId_t& connectionId)
{
  switch (eProtocol)
  {
  case TP_TCP:
  {
    // check if we already have a connection 
    Transport_t transport = make_tuple(sInterface, uiPort, TP_TCP);
    if (m_mConnectionMap.find(transport) != m_mConnectionMap.end())
    {
      connectionId = m_mConnectionMap[transport];
      return boost::system::error_code();
    }
    // create a new connection
    SipClientTcpConnection::ptr pTcpConnection = SipClientTcpConnection::create(m_ioService, sInterface, uiPort);
    pTcpConnection->setReadCompletionHandler(boost::bind(&TransportLayer::tcpClientReadCompletionHandler, this, _1, _2, _3));
    pTcpConnection->setWriteCompletionHandler(boost::bind(&TransportLayer::tcpClientWriteCompletionHandler, this, _1, _2, _3));
    pTcpConnection->setCloseHandler(boost::bind(&TransportLayer::tcpClientCloseCompletionHandler, this, _1));

    m_vTcpClientConnections.push_back(pTcpConnection);
    connectionId = std::make_pair(TCP_CLIENT_SOCKET, m_vTcpClientConnections.size() - 1);
    m_mConnectionMap[transport] = connectionId;
    return boost::system::error_code();
    break;
  }
  case TP_UDP:
  {
    // check if we have any UDP connection as UDP is connectionless.
    if (m_vUdpSockets.empty())
    {
      boost::system::error_code ec = addLocalTransport("0.0.0.0", m_uiSipPort, TP_UDP);
      if (ec)
      {
        LOG(WARNING) << "Failed to add UDP transport: " << ec.message();
        return ec;
      }
    }
    connectionId.first = UDP_SOCKET;
    connectionId.second = 0;
    return boost::system::error_code();
    break;
  }
  default:
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }
  }
}

boost::system::error_code TransportLayer::sendSipMessage(const SipMessage& sipMessage, const std::string& sDestination,
                                                         const uint16_t uiPort, TransactionProtocol eProtocol)
{
  // lookup transport and then send SIP message
  ConnectionId_t connectionId;
  boost::system::error_code ec = getOutgoingConnection(sDestination, uiPort, eProtocol, connectionId);
  if (ec)
  {
    LOG(WARNING) << "Error adding transport: " << ec.message();
    return ec;
  }

  // TODO:
  //  The Via header maddr, ttl, and sent - by components will be set when
  //  the request is processed by the transport layer(Section 18).
  // cast away constness to update headers
  SipMessage& message = const_cast<SipMessage&>(sipMessage);

  // send SIP message
  switch (connectionId.first)
  {
  case TCP_SERVER_SOCKET:
  {
    VLOG(10) << "Using TCP server socket";
    assert(connectionId.second < m_vTcpConnections.size());
    SipServerTcpConnection::ptr pConnection = m_vTcpConnections.at(connectionId.second);

    // TOREMOVE: TODO: sent-by is implicit in via
#if 0
    if (message.isRequest() && !message.hasAttribute(VIA, SENT_BY))
    {
      std::ostringstream sent_by;
      sent_by << SENT_BY << "=" << m_sFQDN << ":" << DEFAULT_SIP_PORT;
      message.appendToHeaderField(VIA, sent_by.str());
    }
#else
    // TODO: get FQDN
    if (message.isRequest())
    {
      std::ostringstream sentBy;
      sentBy << m_sFQDN << ":" << m_uiSipPort;
      std::string sVia = message.getTopMostVia();
      boost::algorithm::replace_first(sVia, "<<PROTO>>", "TCP");
      boost::algorithm::replace_first(sVia, "<<FQDN>>", sentBy.str());
      boost::algorithm::replace_first(sVia, "<<RPORT>>", ::toString(m_uiSipPort));
      message.setTopMostVia(sVia);
    }

#endif

    pConnection->write(message.toString());
    break;
  }
  case TCP_CLIENT_SOCKET:
  {
    VLOG(10) << "Using TCP client socket";
    assert(connectionId.second < m_vTcpClientConnections.size());
    SipClientTcpConnection::ptr pConnection = m_vTcpClientConnections.at(connectionId.second);

    // TOREMOVE: TODO: sent-by is implicit in via
#if 0
    // TODO: should use TCP client port but we have not implemented accepting connections on the port
    if (message.isRequest() && !message.hasAttribute(VIA, SENT_BY))
    {
      std::ostringstream sent_by;
      sent_by << SENT_BY << "=" << m_sFQDN << ":" << DEFAULT_SIP_PORT;
      message.appendToHeaderField(VIA, sent_by.str());
    }
#else
    // TODO: get FQDN
    if (message.isRequest())
    {
      std::ostringstream sentBy;
      sentBy << m_sFQDN << ":" << m_uiSipPort;
      std::string sVia = message.getTopMostVia();
      boost::algorithm::replace_first(sVia, "<<PROTO>>", "TCP");
      boost::algorithm::replace_first(sVia, "<<FQDN>>", sentBy.str());
      boost::algorithm::replace_first(sVia, "<<RPORT>>", ::toString(m_uiSipPort));
      message.setTopMostVia(sVia);
    }

#endif

    pConnection->write(message.toString());
    break;
  }
  case UDP_SOCKET:
  {
    VLOG(10) << "Using UDP socket";
    assert(connectionId.second < m_vUdpSockets.size());
    UdpSocketWrapper::ptr pConnection = m_vUdpSockets.at(connectionId.second);
    EndPoint ep(sDestination, uiPort);
    // TODO: get FQDN
    if (message.isRequest())
    {
      std::ostringstream sentBy;
      sentBy << m_sFQDN << ":" << m_uiSipPort;
      std::string sVia = message.getTopMostVia();
      boost::algorithm::replace_first(sVia, "<<PROTO>>", "UDP");
      boost::algorithm::replace_first(sVia, "<<FQDN>>", sentBy.str());
      boost::algorithm::replace_first(sVia, "<<RPORT>>", ::toString(m_uiSipPort));
      message.setTopMostVia(sVia);
    }
    VLOG(2) << "Sent to [" << ep << "]: \n" << message.toString();

    uint32_t uiLen = message.toString().length();
    uint8_t* pCopy = new uint8_t[uiLen];
    memcpy(pCopy, message.toString().c_str(), uiLen);
    Buffer buffer(pCopy, uiLen);

    pConnection->send(buffer, ep);
    break;
  }
  }
  return boost::system::error_code();
}

} // rfc3261
} // rtp_plus_plus
