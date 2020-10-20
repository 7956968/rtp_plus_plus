#include "CorePch.h"
#include <rtp++/rto/VirtualRtpSession.h>
#include <boost/make_shared.hpp>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/VirtualUdpRtpNetworkInterface.h>
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>

namespace rtp_plus_plus
{
namespace rto
{

VirtualRtpSession::ptr VirtualRtpSession::create(boost::asio::io_service &rIoService, const RtpSessionParameters &rtpParameters, 
                                                 RtpReferenceClock& rtpReferenceClock, const GenericParameters& parameters, bool bIsSender, bool bDisableRtcp)
{
  return boost::make_shared<VirtualRtpSession>(rIoService, rtpParameters, rtpReferenceClock, parameters, bIsSender, bDisableRtcp);
}

VirtualRtpSession::VirtualRtpSession(boost::asio::io_service &rIoService, const RtpSessionParameters &rtpParameters, 
                                     RtpReferenceClock& rtpReferenceClock, const GenericParameters& parameters, bool bIsSender, bool bDisableRtcp)
  : RtpSession(rIoService, rtpParameters, rtpReferenceClock, parameters, bIsSender),
  m_bDisableRtcp(bDisableRtcp)
{

}

void VirtualRtpSession::receiveRtpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to)
{
  VLOG(20) << "Received RTP from " << from.getAddress() << ":" << from.getPort()
          << " at " << to.getAddress() << ":" << to.getPort();

  // locate virtual interface and forward packet
  VirtualUdpRtpNetworkInterface* pInterface = static_cast<VirtualUdpRtpNetworkInterface*>(m_vRtpInterfaces[m_mEndPointInterface[to.toString()]].get());
  pInterface->receiveRtpPacket(networkPacket, from);
}

void VirtualRtpSession::receiveRtcpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to)
{
  VLOG(20) << "Received RTCP from " << from.getAddress() << ":" << from.getPort()
          << " at " << to.getAddress() << ":" << to.getPort();

  // locate virtual interface and forward packet
  VirtualUdpRtpNetworkInterface* pInterface = static_cast<VirtualUdpRtpNetworkInterface*>(m_vRtpInterfaces[m_mEndPointInterface[to.toString()]].get());
  pInterface->receiveRtcpPacket(networkPacket, from);
}

std::vector<RtpNetworkInterface::ptr> VirtualRtpSession::createRtpNetworkInterfaces(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters)
{
  std::vector<RtpNetworkInterface::ptr> vRtpInterfaces;
  if (!mprtp::isMpRtpSession(m_parameters))
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    VLOG(1) << "ADDR/RTP/RTCP " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

    std::unique_ptr<VirtualUdpRtpNetworkInterface> pRtpInterface;
    pRtpInterface = std::unique_ptr<VirtualUdpRtpNetworkInterface>( new VirtualUdpRtpNetworkInterface(rtpEp, rtcpEp));
    pRtpInterface->setRtpSendCallback(boost::bind(&VirtualRtpSession::onSendRtp, this, _1, _2, _3));
    pRtpInterface->setRtcpSendCallback(boost::bind(&VirtualRtpSession::onSendRtcp, this, _1, _2, _3));

    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
  else
  {
    for (std::size_t i = 0; i < rtpParameters.getLocalEndPointCount(); ++i)
    {
      // check if we are dealing with RTP/RTCP mux
      const EndPoint rtpEp = rtpParameters.getLocalEndPoint(i).first;
      const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(i).second;
      LOG(INFO) << "LOCAL ADDR/RTP/RTCP " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

      std::unique_ptr<VirtualUdpRtpNetworkInterface> pRtpInterface;
      pRtpInterface = std::unique_ptr<VirtualUdpRtpNetworkInterface>(new VirtualUdpRtpNetworkInterface( rtpEp, rtcpEp ));
      pRtpInterface->setRtpSendCallback(boost::bind(&VirtualRtpSession::onSendRtp, this, _1, _2, _3));
      pRtpInterface->setRtcpSendCallback(boost::bind(&VirtualRtpSession::onSendRtcp, this, _1, _2, _3));

      vRtpInterfaces.push_back(std::move(pRtpInterface));
      // store index in map so that we can look up interface using end point address
      m_mEndPointInterface[rtpEp.toString()] = vRtpInterfaces.size() - 1;
      m_mEndPointInterface[rtcpEp.toString()] = vRtpInterfaces.size() - 1;

      // Now that we have created a local interface: create flows
      // initialise flows: each flow consists of a 5-tuple of IP-port pairs and protocol
      for (std::size_t j = 0; j < rtpParameters.getRemoteEndPointCount(); ++j)
      {
        // map flow id to interface index
        m_MpRtp.m_mFlowInterfaceMap[m_MpRtp.m_uiLastFlowId] = vRtpInterfaces.size() - 1;
        m_MpRtp.m_mFlows[m_MpRtp.m_uiLastFlowId] = mprtp::MpRtpFlow(m_MpRtp.m_uiLastFlowId, rtpParameters.getLocalEndPoint(i), rtpParameters.getRemoteEndPoint(j));
        m_MpRtp.m_uiLastFlowId++;
      }
    }
  }

  return vRtpInterfaces;
}

std::vector<rfc3550::RtcpReportManager::ptr> VirtualRtpSession::createRtcpReportManagers(const RtpSessionParameters &rtpParameters, const GenericParameters& parameters)
{
  // don't create any RTCP report managers to disable RTCP
  // TODO: this will cause the calling code to fail
  if (m_bDisableRtcp) return std::vector<rfc3550::RtcpReportManager::ptr>();
  else
  {
    return RtpSession::createRtcpReportManagers(rtpParameters, parameters);
  }
}

} // rto
} // rtp_plus_plus

