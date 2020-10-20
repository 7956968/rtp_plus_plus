#include "CorePch.h"
#include <rtp++/network/VirtualUdpRtpNetworkInterface.h>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

VirtualUdpRtpNetworkInterface::VirtualUdpRtpNetworkInterface( const EndPoint& rtpEp, const EndPoint& rtcpEp )
    : RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
    m_rtpEp(rtpEp),
    m_rtcpEp(rtcpEp)
{
}

VirtualUdpRtpNetworkInterface::~VirtualUdpRtpNetworkInterface()
{
}


void VirtualUdpRtpNetworkInterface::reset()
{
}

void VirtualUdpRtpNetworkInterface::shutdown()
{
  VLOG(1) << "[" << this << "] Shutting down";
}

bool VirtualUdpRtpNetworkInterface::recv()
{
  return true;
}

bool VirtualUdpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp)
{
#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTP packet";
#endif

  if (m_rtpCallback) m_rtpCallback(rtpBuffer, m_rtpEp, rtpEp);
  // manually call the completion handler since virtual sockets are being used
  handleSentRtpPacket(rtpBuffer, rtpEp);

  return true;
}

bool VirtualUdpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp)
{
#ifdef DEBUG_RTP
  LOG_FIRST_N(INFO, 1) << "Sending 1st RTCP packet";
#endif

  if (m_rtcpCallback) m_rtcpCallback(rtcpBuffer, m_rtcpEp, rtcpEp);
  // manually call the completion handler since virtual sockets are being used
  handleSentRtcpPacket(rtcpBuffer, rtcpEp);
  return true;
}

void VirtualUdpRtpNetworkInterface::receiveRtpPacket(NetworkPacket networkPacket, const EndPoint& from)
{
  networkPacket.setNtpArrivalTime(RtpTime::getNTPTimeStamp());
  processIncomingRtpPacket(networkPacket, from);
}

void VirtualUdpRtpNetworkInterface::receiveRtcpPacket(NetworkPacket networkPacket, const EndPoint& from)
{
  networkPacket.setNtpArrivalTime(RtpTime::getNTPTimeStamp());
  processIncomingRtcpPacket(networkPacket, from);
}

void VirtualUdpRtpNetworkInterface::handleSentRtpPacket(Buffer buffer, const EndPoint& ep)
{
  onRtpSent(buffer, ep);
}

void VirtualUdpRtpNetworkInterface::handleSentRtcpPacket(Buffer buffer, const EndPoint& ep)
{
  onRtcpSent(buffer, ep);
}

}
