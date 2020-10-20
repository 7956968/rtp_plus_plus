#include "CorePch.h"
#include <rtp++/rfc5762/DccpRtpNetworkInterface.h>
#ifdef ENABLE_ASIO_DCCP
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc5762
{

DccpRtpNetworkInterface::DccpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp)
    : RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
    m_rIoService(rIoService),
    m_rtpEp(rtpEp),
    m_rtcpEp(rtcpEp),
    m_bShuttingDown(false)
#ifdef RTP_DEBUG
    ,m_rtpDump("dump.rtp")
#endif
{
}

DccpRtpNetworkInterface::~DccpRtpNetworkInterface()
{
}

void DccpRtpNetworkInterface::reset()
{
}

void DccpRtpNetworkInterface::shutdown()
{
  VLOG(1) << "[" << this << "] Shutting down RTP network interface";
  m_bShuttingDown = true;
  // shutdown all connections
  for (auto& pair: m_connections)
  {
    pair.second->close();
  }
  m_connections.clear();
}

void DccpRtpNetworkInterface::stopConnection(const std::string& sEndPoint, DccpRtpConnection::ptr pConnection)
{
  auto it = m_connections.find(sEndPoint);
  assert (it != m_connections.end());
  m_connections.erase(it);
  pConnection->close();
}

bool DccpRtpNetworkInterface::recv()
{
  // active connections are initiated when there is something to be sent
  return true;
}

DccpRtpConnection::ptr DccpRtpNetworkInterface::getConnection(const EndPoint& ep, bool bRtp)
{
  boost::mutex::scoped_lock l(m_connectionLock);

  std::string sHost;
  uint16_t uiPort;
  if (bRtp)
  {
    sHost = m_rtpEp.getAddress();
    uiPort = m_rtpEp.getPort();
  }
  else
  {
    sHost = m_rtcpEp.getAddress();
    uiPort = m_rtcpEp.getPort();
  }
  auto it = m_connections.find(ep.toString());
  if (it == m_connections.end())
  {
    VLOG(2) << "Creating DCCP connection to " << ep;
    DccpRtpConnection::ptr pConnection = DccpRtpConnection::create(m_rIoService, sHost, uiPort);
    if (bRtp)
    {
      pConnection->onRecv(boost::bind(&DccpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
      pConnection->onSendComplete(boost::bind(&DccpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
    }
    else
    {
      pConnection->onRecv(boost::bind(&DccpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
      pConnection->onSendComplete(boost::bind(&DccpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
    }
    boost::asio::ip::dccp::resolver r(m_rIoService);
    VLOG(2) << "Starting connection to " << ep;
    pConnection->start(r.resolve(boost::asio::ip::dccp::resolver::query(ep.getAddress(), ::toString(ep.getPort()))));
    m_connections[ep.toString()] = pConnection;
    return pConnection;
  }
  else
  {
    VLOG(10) << "Retrieving connection to " << ep;
    return it->second;
  }
}

bool DccpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
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

  DccpRtpConnection::ptr pConnection = getConnection(rtpEp, true);

  // going to handle framing at the socket wrapper level
  // TODO: rewrite buffer class to optionally leave spacec at beginning of buffer so that we
  // can prepend the size header for TCP
  //Buffer newBuffer(new uint8_t[rtpBuffer.getSize() + 2], rtpBuffer.getSize() + 2);
  //OBitStream out(newBuffer);
  //uint16_t uiSize = rtpBuffer.getSize();
  //out.write(uiSize, 16);
  //const uint8_t* pDest = rtpBuffer.data();
  //out.writeBytes(pDest, uiSize);
  //pConnection->send(newBuffer, rtpEp);

  if (pConnection)
  {
    pConnection->send(rtpBuffer, rtpEp);
    return true;
  }
  else
  {
    VLOG(2)  << "Couldn't get connection for RTP: connection in progress?";
    return false;
  }
}

bool DccpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
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

  // The active participant should create a connection
  // The passive participant should check if a connection has
  // been made and use that. Otherwise it should not send any RTCP
  DccpRtpConnection::ptr pConnection = getConnection(rtcpEp, false);

  // going to handle framing at the socket wrapper level
  // TODO: rewrite buffer class to optionally leave spacec at beginning of buffer so that we
  // can prepend the size header for TCP
  //Buffer newBuffer(new uint8_t[rtcpBuffer.getSize() + 2], rtcpBuffer.getSize() + 2);
  //OBitStream out(newBuffer);
  //uint16_t uiSize = rtcpBuffer.getSize();
  //out.write(uiSize, 16);
  //const uint8_t* pDest = rtcpBuffer.data();
  //out.writeBytes(pDest, uiSize);
  //pConnection->send(newBuffer, rtcpEp);
  if (pConnection)
  {
    pConnection->send(rtcpBuffer, rtcpEp);
    return true;
  }
  else
  {
    VLOG(2)  << "Couldn't get connection for RTCP: connection in progress?";
    return false;
  }
}

void DccpRtpNetworkInterface::handleRtpPacket(const boost::system::error_code& ec, DccpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep)
{
#ifdef DEBUG_INCOMING_RTP
  LOG_EVERY_N(INFO, 100) << "[" << this << "] Read  " << google::COUNTER << "RTP packets";
#endif
  if (!ec)
  {
    processIncomingRtpPacket(networkPacket, ep);
    //pSource->recv();
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


void DccpRtpNetworkInterface::handleRtcpPacket(const boost::system::error_code& ec, DccpRtpConnection::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep)
{
#if 0
  DLOG(INFO) << "[" << this << "]";
#endif
  if (!ec)
  {
#ifdef DEBUG_RTCP
    uint32_t uiNtpTimestampMsw = 0;
    uint32_t uiNtpTimestampLsw = 0;
    getNTPTimeStamp(uiNtpTimestampMsw, uiNtpTimestampLsw);
    DLOG(INFO) << "###### RTCP received: MSW " << hex(uiNtpTimestampMsw) << " LSW:" << hex(uiNtpTimestampLsw);
#endif
    // call base class method
    processIncomingRtcpPacket(networkPacket, ep);
    //pSource->recv();
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

void DccpRtpNetworkInterface::handleSentRtpPacket(const boost::system::error_code& ec, DccpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep)
{
  onRtpSent(buffer, ep);
}

void DccpRtpNetworkInterface::handleSentRtcpPacket(const boost::system::error_code& ec, DccpRtpConnection::ptr pSource, Buffer buffer, const EndPoint& ep)
{
  onRtcpSent(buffer, ep);
}


}
}
#endif