#include "CorePch.h"
#include <rtp++/network/RtspAdapterRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <rtp++/RtpTime.h>


namespace rtp_plus_plus
{

RtspAdapterRtpNetworkInterface::RtspAdapterRtpNetworkInterface( rfc2326::RtspClientConnection::ptr pConnection, uint32_t uiRtpChannel, uint32_t uiRtcpChannel )
    : RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
      m_pConnection(pConnection),
      m_uiRtpChannel(uiRtpChannel),
      m_uiRtcpChannel(uiRtcpChannel)
{
  VLOG(5) << "Registering RTP channel " << m_uiRtpChannel << " RTCP channel: " << m_uiRtcpChannel;
  pConnection->registerRtpRtcpReadCompletionHandler(uiRtpChannel, boost::bind(&RtspAdapterRtpNetworkInterface::handleIncomingRtp, this, _1, _2, _3, _4));
  pConnection->registerRtpRtcpReadCompletionHandler(uiRtcpChannel, boost::bind(&RtspAdapterRtpNetworkInterface::handleIncomingRtcp, this, _1, _2, _3, _4));
  pConnection->registerRtpRtcpWriteCompletionHandler(uiRtcpChannel, boost::bind(&RtspAdapterRtpNetworkInterface::handleSentRtcpPacket, this, _1, _2, _3));
}

void RtspAdapterRtpNetworkInterface::handleIncomingRtp(const boost::system::error_code &ec, NetworkPacket networkPacket, const EndPoint& ep, rfc2326::RtspClientConnection::ptr)
{
  if (!ec)
  {
    VLOG(10) << "Handling incoming RTP size: " << networkPacket.getSize();
    processIncomingRtpPacket(networkPacket, ep);
  }
  else
  {
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

void RtspAdapterRtpNetworkInterface::handleIncomingRtcp(const boost::system::error_code &ec, NetworkPacket networkPacket, const EndPoint& ep, rfc2326::RtspClientConnection::ptr)
{
  if (!ec)
  {
    VLOG(2) << "Handling incoming RTCP size: " << networkPacket.getSize();
    processIncomingRtcpPacket(networkPacket, ep);
  }
  else
  {
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

RtspAdapterRtpNetworkInterface::~RtspAdapterRtpNetworkInterface()
{
}

void RtspAdapterRtpNetworkInterface::reset()
{
  VLOG(5) << "[" << this << "] Initialising RTP network interface";
  // NOOP: no initialisation necessary since everything is handled by the RtspClientConnection
}

void RtspAdapterRtpNetworkInterface::shutdown()
{
  VLOG(5) << "[" << this << "] Shutting down RTP network interface";
  // NOOP: no initialisation necessary since everything is handled by the RtspClientConnection
}

bool RtspAdapterRtpNetworkInterface::recv()
{
  // NOOP: no initialisation necessary since everything is handled by the RtspClientConnection
  return true;
}

bool RtspAdapterRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
{
  // NOOP: we don't send RTP back to the server
  return false;
}

bool RtspAdapterRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
{
  // TODO: check if TCP connection is ok
#if 1
  m_pConnection->writeInterleavedData(m_uiRtcpChannel, rtcpBuffer);
#endif
  return true;
}


void RtspAdapterRtpNetworkInterface::handleSentRtcpPacket(const boost::system::error_code& ec, const std::string& sMessage, rfc2326::RtspClientConnection::ptr pConnection)
{
  //TODO: need to call of framework code: update arguments
  // onRtcpSent(buffer, ep);
  // dummy for now....
  onRtcpSent(Buffer(), EndPoint());

}

}
