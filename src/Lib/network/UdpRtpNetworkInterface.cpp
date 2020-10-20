#include "CorePch.h"
#include <rtp++/network/UdpRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <rtp++/RtpTime.h>

//#define DEBUG_RTP
//#define DEBUG_RTP_TS
//#define RTP_DEBUG

namespace rtp_plus_plus
{

UdpRtpNetworkInterface::UdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp, boost::system::error_code& ec)
  :RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
  m_rIoService(rIoService),
  m_bInitialised(false),
  m_bShuttingDown(false),
  m_rtpEp(rtpEp),
  m_rtcpEp(rtcpEp)
#ifdef RTP_DEBUG
  ,m_rtpDump("dump.rtp")
#endif
{
  initialise(ec);
}

UdpRtpNetworkInterface::UdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp,
  std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket, std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket)
  :RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
  m_rIoService(rIoService),
  m_bInitialised(false),
  m_bShuttingDown(false),
  m_rtpEp(rtpEp),
  m_rtcpEp(rtcpEp)
#ifdef RTP_DEBUG
  , m_rtpDump("dump.rtp")
#endif
{
  initialiseExistingSockets(std::move(pRtpSocket), std::move(pRtcpSocket));
}

UdpRtpNetworkInterface::~UdpRtpNetworkInterface()
{
}

void UdpRtpNetworkInterface::initialise(boost::system::error_code& ec)
{  
  // unicast case
  VLOG(5) << "Creating RTP socket " << m_rtpEp;
  m_pRtpSocket = UdpSocketWrapper::create(m_rIoService, m_rtpEp.getAddress(), m_rtpEp.getPort(), ec);
  if (ec) return;
  m_pRtpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
  m_pRtpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
  // This check should be sufficient
  assert (m_rtpEp.getPort() != m_rtcpEp.getPort());
  // add rtcp socket
  VLOG(5) << "Creating RTCP socket " << m_rtcpEp;
  m_pRtcpSocket = UdpSocketWrapper::create(m_rIoService, m_rtcpEp.getAddress(), m_rtcpEp.getPort(), ec);
  if (ec) return;
  m_pRtcpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
  m_pRtcpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));

  m_bInitialised = true;
  m_bShuttingDown = false;

  // Multicast HACK: commenting out for now: refactoring code to pass in local endpoint so
  // that this class does not need to know about the RtpSessionParameter
#if 0
  // we need to create the appropriate UDP sockets according to the RTP session description
  // and bind the handlers to the methods that can handle incoming packets
  const EndPoint remoteRtpEp = m_sessionParameters.getRemoteEndPoint(0).first;
  const EndPoint remoteRtcpEp = m_sessionParameters.getRemoteEndPoint(0).second;
  VLOG(1) << "ADDR/RTP/RTCP " << remoteRtpEp.getAddress() << ":" << remoteRtpEp.getPort() << ":" << remoteRtcpEp.getPort();

  // HACK: use the remote endpoint to determine if this is multicast!
  boost::asio::ip::address addr(boost::asio::ip::address::from_string(remoteRtpEp.getAddress()));
  if (addr.is_multicast())
  {
    m_pRtpSocket = UdpSocketWrapper::create(m_rIoService, remoteRtpEp.getAddress(), remoteRtpEp.getPort());
    m_pRtcpSocket = UdpSocketWrapper::create(m_rIoService, remoteRtcpEp.getAddress(), remoteRtcpEp.getPort());
    m_pRtpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
    m_pRtpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
    m_pRtcpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
    m_pRtcpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
  }
  else
  {
    // unicast case
    // this basic receiver can only handle one endpoint
    const EndPoint rtpEp = m_sessionParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = m_sessionParameters.getLocalEndPoint(0).second;

    m_pRtpSocket = UdpSocketWrapper::create(m_rIoService, rtpEp.getAddress(), rtpEp.getPort());
    m_pRtpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
    m_pRtpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
    // This check should be sufficient
    assert (rtpEp.getPort() != rtcpEp.getPort());
    // add rtcp socket
    m_pRtcpSocket = UdpSocketWrapper::create(m_rIoService, rtcpEp.getAddress(), rtcpEp.getPort());
    m_pRtcpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
    m_pRtcpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));
  }

  m_bInitialised = true;
  m_bShuttingDown = false;
#endif
}

void UdpRtpNetworkInterface::initialiseExistingSockets(std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket, std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket)
{
  // unicast case
  VLOG(5) << "Using existing RTP socket " << m_rtpEp;
  m_pRtpSocket = UdpSocketWrapper::create(m_rIoService, m_rtpEp.getAddress(), m_rtpEp.getPort(), std::move(pRtpSocket));
  m_pRtpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtpPacket, this, _1, _2, _3, _4));
  m_pRtpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtpPacket, this, _1, _2, _3, _4));
  // This check should be sufficient
  assert(m_rtpEp.getPort() != m_rtcpEp.getPort());
  // add rtcp socket
  VLOG(5) << "Using existing RTCP socket " << m_rtcpEp;
  m_pRtcpSocket = UdpSocketWrapper::create(m_rIoService, m_rtcpEp.getAddress(), m_rtcpEp.getPort(), std::move(pRtcpSocket));
  m_pRtcpSocket->onRecv(boost::bind(&UdpRtpNetworkInterface::handleRtcpPacket, this, _1, _2, _3, _4));
  m_pRtcpSocket->onSendComplete(boost::bind(&UdpRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3, _4));

  m_bInitialised = true;
  m_bShuttingDown = false;
}

void UdpRtpNetworkInterface::reset()
{
  boost::system::error_code ec;
  // TODO: how to handle this case after using error code in init? 
  // Also, there are two init methods now...
  // Throw exception? Get rid of reset method?
  initialise(ec);
  if (ec)
  {
    assert(false);
  }
}

void UdpRtpNetworkInterface::shutdown()
{
  VLOG(5) << "[" << this << "] Shutting down RTP network interface";
  if (m_bInitialised)
  {
    m_pRtpSocket->close();
    m_pRtcpSocket->close();

    m_bInitialised = false;
  }
}

bool UdpRtpNetworkInterface::recv()
{
  assert(m_pRtpSocket && m_pRtcpSocket);
  m_pRtpSocket->recv();
  m_pRtcpSocket->recv();
  return true;
}

bool UdpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
{
  if (!m_bInitialised)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTP packets";
    return false;
  }

#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTP packet";
#endif

  m_pRtpSocket->send(rtpBuffer, rtpEp);
  return true;
}

bool UdpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
{
  if (!m_bInitialised)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTCP packets";
    return false;
  }

#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTCP packet";
#endif

  m_pRtcpSocket->send(rtcpBuffer, rtcpEp );
  return true;
}

void UdpRtpNetworkInterface::handleRtpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep)
{
#ifdef DEBUG_INCOMING_RTP
  LOG_EVERY_N(INFO, 100) << "[" << this << "] Read  " << google::COUNTER << "RTP packets";
#endif
  if (!ec)
  {
    processIncomingRtpPacket(networkPacket, ep);
    pSource->recv();
  }
  else
  {
#ifdef _WIN32
    // http://stackoverflow.com/questions/13995830/windows-boost-asio-10061-in-async-receive-from-on-on-async-send-to
    if (ec == boost::asio::error::connection_refused || ec == boost::asio::error::connection_reset)
    {
      VLOG(10) << "WIN32 Error in receive: " << ec.message();
      // just start next read
      pSource->recv();
      return;
    }
#endif
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Error in receive: " << ec.message();
    }
    else
    {
      VLOG(10) << "Shutting down: " << ec.message();
    }
  }
}


void UdpRtpNetworkInterface::handleRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep)
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
    pSource->recv();
  }
  else
  {
#ifdef _WIN32
    // http://stackoverflow.com/questions/13995830/windows-boost-asio-10061-in-async-receive-from-on-on-async-send-to
    if (ec == boost::asio::error::connection_refused || ec == boost::asio::error::connection_reset)
    {
      VLOG(10) << "WIN32 Error in receive: " << ec.message();
      // just start next read
      pSource->recv();
      return;
    }
#endif

    if ((ec != boost::asio::error::operation_aborted) &&
        (ec != boost::asio::error::connection_refused)) // seems to get triggered on Windows?
    {
      LOG(WARNING) << "Error in receive: " << ec.message();
    }
    else
    {
      VLOG(10) << "Error in receive. Shutting down? " << ec.message();
    }
  }
}

void UdpRtpNetworkInterface::handleSentRtpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, Buffer buffer, const EndPoint& ep)
{
  onRtpSent(buffer, ep);
}

void UdpRtpNetworkInterface::handleSentRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, Buffer buffer, const EndPoint& ep)
{
  onRtcpSent(buffer, ep);
}

}

