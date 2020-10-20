#include "CorePch.h"
#include <rtp++/network/MuxedUdpRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <rtp++/mprtp/MpRtp.h>

namespace rtp_plus_plus
{

MuxedUdpRtpNetworkInterface::MuxedUdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpRtcpEp, boost::system::error_code& ec)
  :RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
  m_rIoService(rIoService),
  m_bInitialised(false),
  m_bShuttingDown(false),
  m_rtpRtcpEp(rtpRtcpEp)
{
  initialise(ec);
}

MuxedUdpRtpNetworkInterface::MuxedUdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpRtcpEp, 
  std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket)
  :RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
  m_rIoService(rIoService),
  m_bInitialised(false),
  m_bShuttingDown(false),
  m_rtpRtcpEp(rtpRtcpEp)
{
  initialiseExistingSocket(std::move(pRtpRtcpSocket));
}

MuxedUdpRtpNetworkInterface::~MuxedUdpRtpNetworkInterface()
{
}

void MuxedUdpRtpNetworkInterface::reset()
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

void MuxedUdpRtpNetworkInterface::shutdown()
{
  VLOG(1) << "[" << this << "] Shutting down";
  if (m_bInitialised)
  {
    m_pMuxedRtpSocket->close();
    m_bInitialised = false;
  }
}

bool MuxedUdpRtpNetworkInterface::recv()
{
  assert(m_pMuxedRtpSocket);
  m_pMuxedRtpSocket->recv();
  return true;
}

void MuxedUdpRtpNetworkInterface::handleMuxedRtpRtcpPacket(const boost::system::error_code& ec,
                                                           UdpSocketWrapper::ptr pSource,
                                                           NetworkPacket networkPacket,
                                                           const EndPoint& ep)
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
    // TODO: FIXME together with TCP version!
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
    if (!m_bShuttingDown) pSource->recv();
  }
  else
  {
#ifdef _WIN32
    // http://stackoverflow.com/questions/13995830/windows-boost-asio-10061-in-async-receive-from-on-on-async-send-to
    if (ec == boost::asio::error::connection_refused || ec == boost::asio::error::connection_reset)
    {
      VLOG(10) << "WIN32 Error in receive: " << ec.message();
      // just start next read
      m_pMuxedRtpSocket->recv();
      return;
    }
#endif
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

void MuxedUdpRtpNetworkInterface::handleSentRtpRtcpPacket(const boost::system::error_code& ec,
                                                          UdpSocketWrapper::ptr pSource,
                                                          Buffer buffer,
                                                          const EndPoint& ep)
{
  // Note: this mutex must be locked carefully to not interfere
  // with the mutexin the UdpSocketWrapper
  PacketType eType;
  {
    boost::mutex::scoped_lock l(m_lock);
    eType = m_qTypes.front();
    m_qTypes.pop_front();
  }
  switch (eType)
  {
    case RTP_PACKET:
    {
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

bool MuxedUdpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
{
  if (!m_bInitialised)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTP packets";
    return false;
  }
  // we need to keep track of the packet oder in which RTP/RTCP is sent
  // RTP and RTCP may be sent concurrently: need to lock type queue
  // Note: this mutex must be locked carefully to not interfere
  // with the mutexin the UdpSocketWrapper
  {
    boost::mutex::scoped_lock l(m_lock);
    m_qTypes.push_back(RTP_PACKET);
  }
  m_pMuxedRtpSocket->send(rtpBuffer, rtpEp);
  return true;
}

bool MuxedUdpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
{
  if (!m_bInitialised)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTP packets";
    return false;
  }
  // RTP and RTCP may be sent concurrently if we are using multiple threads
  // running the asio service: need to lock type queue
  // Note: this mutex must be locked carefully to not interfere
  // with the mutexin the UdpSocketWrapper
  {
    boost::mutex::scoped_lock l(m_lock);
    m_qTypes.push_back(COMPOUND_RTCP_PACKET);
  }
  m_pMuxedRtpSocket->send(rtcpBuffer, rtcpEp );
  return true;
}

void MuxedUdpRtpNetworkInterface::initialise(boost::system::error_code& ec)
{
  // Note: unicast only case: RTP+RTCP mux is not defined for ASM!!!
  // this basic receiver can only handle one endpoint
  VLOG(1) << "ADDR/RTP/RTCP " << m_rtpRtcpEp.getAddress()
          << ":" << m_rtpRtcpEp.getPort() << ":" << m_rtpRtcpEp.getPort();

  // check if we are dealing with RTCP MUX
  m_pMuxedRtpSocket = UdpSocketWrapper::create(m_rIoService,
                                               m_rtpRtcpEp.getAddress(),
                                               m_rtpRtcpEp.getPort(),
                                               ec);
  if (ec) return;

  m_pMuxedRtpSocket->onRecv(boost::bind(&MuxedUdpRtpNetworkInterface::handleMuxedRtpRtcpPacket,
                                        this, _1, _2, _3, _4));
  m_pMuxedRtpSocket->onSendComplete(boost::bind(&MuxedUdpRtpNetworkInterface::handleSentRtpRtcpPacket,
                                                this, _1, _2, _3, _4));
  LOG(INFO) << "Muxing RTP and RTCP on port " << m_rtpRtcpEp.getPort();

  m_bInitialised = true;
  m_bShuttingDown = false;
}

void MuxedUdpRtpNetworkInterface::initialiseExistingSocket(std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket)
{
  // Note: unicast only case: RTP+RTCP mux is not defined for ASM!!!
  // this basic receiver can only handle one endpoint
  VLOG(1) << "ADDR/RTP/RTCP " << m_rtpRtcpEp.getAddress()
    << ":" << m_rtpRtcpEp.getPort() << ":" << m_rtpRtcpEp.getPort();

  // check if we are dealing with RTCP MUX
  m_pMuxedRtpSocket = UdpSocketWrapper::create(m_rIoService,
    m_rtpRtcpEp.getAddress(),
    m_rtpRtcpEp.getPort(),
    std::move(pRtpRtcpSocket));

  m_pMuxedRtpSocket->onRecv(boost::bind(&MuxedUdpRtpNetworkInterface::handleMuxedRtpRtcpPacket,
    this, _1, _2, _3, _4));
  m_pMuxedRtpSocket->onSendComplete(boost::bind(&MuxedUdpRtpNetworkInterface::handleSentRtpRtcpPacket,
    this, _1, _2, _3, _4));
  LOG(INFO) << "Muxing RTP and RTCP on port " << m_rtpRtcpEp.getPort();

  m_bInitialised = true;
  m_bShuttingDown = false;
}

} // rtp_plus_plus
