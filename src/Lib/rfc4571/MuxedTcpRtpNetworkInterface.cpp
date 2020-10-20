#include "CorePch.h"
#include <rtp++/rfc4571/MuxedTcpRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/mprtp/MpRtp.h>

namespace rtp_plus_plus
{
namespace rfc4571
{

MuxedTcpRtpNetworkInterface::MuxedTcpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& localRtpEp, const EndPoint& remoteRtpEp, bool bIsActiveConnection )
    : RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
    m_rIoService(rIoService),
    m_localRtpEp(localRtpEp),
    m_remoteRtpEp(remoteRtpEp),
    m_bIsActiveConnection(bIsActiveConnection),
    m_bConnected(false),
    m_bShuttingDown(false),
    m_acceptor(rIoService)
#ifdef RTP_DEBUG
    ,m_rtpDump("dump.rtp")
#endif
{
  if (!m_bIsActiveConnection)
  {
    initAccept();
  }
  else
  {
    m_pConnection = TcpRtpConnection::create(m_rIoService, m_localRtpEp.getAddress(), m_localRtpEp.getPort());
    m_pConnection->onRecv(boost::bind(&MuxedTcpRtpNetworkInterface::handleMuxedRtpRtcpPacket, this, _1, _2, _3, _4));
    m_pConnection->onSendComplete(boost::bind(&MuxedTcpRtpNetworkInterface::handleSentRtpRtcpPacket, this, _1, _2, _3, _4));
    boost::asio::ip::tcp::resolver r(m_rIoService);
    VLOG(2) << "Starting connection to " << m_remoteRtpEp;
    m_pConnection->start(r.resolve(boost::asio::ip::tcp::resolver::query(m_remoteRtpEp.getAddress(), ::toString(m_remoteRtpEp.getPort()))));
  }
}

MuxedTcpRtpNetworkInterface::~MuxedTcpRtpNetworkInterface()
{
}

void MuxedTcpRtpNetworkInterface::reset()
{
  m_bConnected = false;
}

void MuxedTcpRtpNetworkInterface::shutdown()
{
  VLOG(1) << "[" << this << "] Shutting down RTP network interface";
  m_bShuttingDown = true;
  m_acceptor.close();
  if (m_pConnection)
    m_pConnection->close();
  m_bConnected = false;
}

bool MuxedTcpRtpNetworkInterface::recv()
{
  // active connections are initiated when there is something to be sent
  return true;
}

bool MuxedTcpRtpNetworkInterface::sendDataOverTcp(Buffer buffer, const EndPoint& ep, bool bIsRtp)
{
  if (bIsRtp)
  {
    // we need to keep track of the packet oder in which RTP/RTCP is sent
    // RTP and RTCP may be sent concurrently: need to lock type queue
    boost::mutex::scoped_lock l(m_lock);
    m_qTypes.push_back(RTP_PACKET);
  }
  else
  {
    boost::mutex::scoped_lock l(m_lock);
    m_qTypes.push_back(COMPOUND_RTCP_PACKET);
  }

  if (m_bIsActiveConnection)
  {
    // BF: MOVED to constructor: race condition being called by RTP and RTCP
//    // try connect if no connection
//    if (!m_pConnection)
//    {
//      boost::mutex::scoped_lock l(m_connectionLock);
//      // create connection
//      if (!m_pConnection)
//      {
//        boost::asio::ip::tcp::resolver r(m_rIoService);
//        VLOG(2) << "Starting connection to " << m_remoteRtpEp;
//        m_pConnection->start(r.resolve(boost::asio::ip::tcp::resolver::query(m_remoteRtpEp.getAddress(), ::toString(m_remoteRtpEp.getPort()))));
//      }
//      else
//      {
//        LOG(INFO) << "Connection previously created!!!!!";
//      }
//    }
    m_pConnection->send(buffer, m_localRtpEp);
    return true;
  }
  else
  {
    if (m_bConnected && m_pConnection)
    {
      m_pConnection->send(buffer, m_localRtpEp);
      return true;
    }
    else
    {
      VLOG(2)  << "Couldn't get connection for RTP: connection in progress?";
      return false;
    }
  }
}

bool MuxedTcpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
{
  if (m_bShuttingDown)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTP packets";
    return false;
  }

#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTP packet";
#endif

  if (m_remoteRtpEp.getAddress() != rtpEp.getAddress() || m_remoteRtpEp.getPort() != rtpEp.getPort())
  {
    LOG(WARNING) << "RTP end points don't match!! Local: " << m_localRtpEp << " Send: " << rtpEp << ". Using local endpoint";
  }

  return sendDataOverTcp(rtpBuffer, m_localRtpEp, true);
}

bool MuxedTcpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
{
  if (m_bShuttingDown)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTCP packets";
    return false;
  }

#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTCP packet";
#endif

  if (m_remoteRtpEp.getAddress() != rtcpEp.getAddress() || m_remoteRtpEp.getPort() != rtcpEp.getPort())
  {
    LOG(WARNING) << "RTP/RTCP end points don't match!! Local: " << m_localRtpEp << " Send: " << rtcpEp << ". Using local endpoint";
  }

  return sendDataOverTcp(rtcpBuffer, m_localRtpEp, false);
}

void MuxedTcpRtpNetworkInterface::handleMuxedRtpRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep)
{
#if 0
  DLOG(INFO) << "[" << this << "]";
#endif
  if (!ec)
  {
    // Need to determine whether we are dealing with an RTP or an RTCP packet
    // extract payload type byte
    const uint8_t* pData = networkPacket.data();
    uint8_t uiPt = pData[1];
    // An issue arises here if pt + 128 (marker bit) falls in the RTCP range
#if 0
    VLOG(2) << "Muxed RTP RTCP Packet Size: " << networkPacket.getSize() << " PT: " << (int)uiPt;
#endif
    // TODO: FIXME: fix together with UDP version!
    // HACK to get around marker bit issue for now
    // Should check payload types in SDP for classification
    if (rfc3550::isRtcpPacketType(uiPt) && uiPt <= mprtp::PT_MPRTCP )
    {
      processIncomingRtcpPacket(networkPacket, ep);
    }
    else
    {
      processIncomingRtpPacket(networkPacket, ep);
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Error in receive: " << ec.message();
    }
    else
    {
      VLOG(1) << "Shutting down: " << ec.message();
    }
  }
}

void MuxedTcpRtpNetworkInterface::handleSentRtpRtcpPacket(const boost::system::error_code& ec, TcpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep)
{
  boost::mutex::scoped_lock l(m_lock);
  PacketType eType = m_qTypes.front();
  m_qTypes.pop_front();
  switch (eType)
  {
  case RTP_PACKET:
  {
    // pop RTP packet
    onRtpSent(buffer, ep);
    break;
  }
  case COMPOUND_RTCP_PACKET:
  {
    onRtcpSent(buffer, ep);
    break;
  }
  }
}

void MuxedTcpRtpNetworkInterface::initAccept()
{
  boost::asio::ip::tcp::resolver resolver(m_rIoService);
  boost::asio::ip::tcp::resolver::query query(m_localRtpEp.getAddress(), ::toString(m_localRtpEp.getPort()));
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  m_acceptor.open(endpoint.protocol());
  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  VLOG(2) << "Binding acceptor to endpoint " << m_localRtpEp;
  m_acceptor.bind(endpoint);
  m_acceptor.listen();
  startAccept();
}

void MuxedTcpRtpNetworkInterface::startAccept()
{
  m_pConnection.reset(new TcpRtpConnection(m_rIoService));
  m_pConnection->onRecv(boost::bind(&MuxedTcpRtpNetworkInterface::handleMuxedRtpRtcpPacket, this, _1, _2, _3, _4));
  m_pConnection->onSendComplete(boost::bind(&MuxedTcpRtpNetworkInterface::handleSentRtpRtcpPacket, this, _1, _2, _3, _4));

  m_acceptor.async_accept(m_pConnection->socket(),
      boost::bind(&MuxedTcpRtpNetworkInterface::handleAccept, this,
      boost::asio::placeholders::error));
}

void MuxedTcpRtpNetworkInterface::handleAccept(const boost::system::error_code& e)
{
  // Check whether the server was stopped by a signal before this completion
  // handler had a chance to run.
  if (!m_acceptor.is_open())
  {
    return;
  }

  if (!e)
  {
    m_bConnected = true;
    LOG(INFO) << "Accept succeeded";
    m_pConnection->start();
  }
  else
  {
    LOG(WARNING) << "Failed to accept connection";
    initAccept();
  }
}

}
}
