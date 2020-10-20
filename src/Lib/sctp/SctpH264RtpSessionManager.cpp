#include "CorePch.h"
#include <rtp++/network/SctpRtpNetworkInterface.h>
#include <rtp++/sctp/SctpH264RtpSessionManager.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{

using media::MediaSample;

namespace sctp
{

std::unique_ptr<SctpH264RtpSessionManager> SctpH264RtpSessionManager::create(boost::asio::io_service& ioService,
                                                                           const GenericParameters& applicationParameters)
{
  return std::unique_ptr<SctpH264RtpSessionManager>( new SctpH264RtpSessionManager(ioService, applicationParameters) );
}

SctpH264RtpSessionManager::SctpH264RtpSessionManager(boost::asio::io_service& ioService,
                                                   const GenericParameters& applicationParameters)
  :RtpSessionManager(ioService, applicationParameters)
{
  // get SCTP AVC application parameters
  boost::optional<std::vector<std::string> > vPolicies = applicationParameters.getStringsParameter(app::ApplicationParameters::sctp_policies);
  if (vPolicies)
  {
    setSctpPolicies(*vPolicies);
  }
  else
  {
    VLOG(2) << "No SCTP policies configured, using default policies";
  }
}

void SctpH264RtpSessionManager::setSctpPolicies(const std::vector<std::string>& vPolicies )
{
  boost::optional<SctpRtpPolicy> policies = SctpRtpPolicy::create(vPolicies);
  if (policies)
  {
    m_sctpPolicy = *policies;
    // TODO: need some way to update SCTP network interface
  }
  else
  {
    m_sctpPolicy = SctpRtpPolicy::createDefaultPolicy();
  }
  m_sctpPolicyManager.setPolicy(m_sctpPolicy);
}

void SctpH264RtpSessionManager::send(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
  uint32_t uiChannelId = m_sctpPolicyManager.getRtpChannelForAvcMedia(mediaSample);

  // SCTP: need to select channel for each packet
  for (RtpPacket& rtpPacket : rtpPackets)
  {
    rtpPacket.setId(uiChannelId);
  }
  m_pRtpSession->sendRtpPackets(rtpPackets);
}

void SctpH264RtpSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
  uint32_t uiChannelId = m_sctpPolicyManager.getRtpChannelForAvcMedia(vMediaSamples);

  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif

  // Update channel id for each packet
  // for now all packets in the same access unit
  // are assigned to the same channel
  for (RtpPacket& rtpPacket : rtpPackets)
  {
    rtpPacket.setId(uiChannelId);
  }
  m_pRtpSession->sendRtpPackets(rtpPackets);
}

void SctpH264RtpSessionManager::handleMemberUpdate(const MemberUpdate& memberUpdate)
{
  LOG(WARNING) << "Member update: " << memberUpdate;
  VLOG(15) << "Member update: " << memberUpdate;
  // At this point we know the RTT so we can setup the required channels
  // configure SCTP policies on network interface
  RtpNetworkInterface::ptr& pNetworkInterface = m_pRtpSession->getRtpNetworkInterface(0);
  // we need access to the raw interface so that we can set the SCTP properties
  SctpRtpNetworkInterface* pSctpRtpNetworkInterface = dynamic_cast<SctpRtpNetworkInterface*>(pNetworkInterface.get());
  // if this is an SCTP RTP session manager it MUST be an SCTP RTP network interface 
  assert(pSctpRtpNetworkInterface);
  // HACK: wouldn't work for SCTP multihoming...
  pSctpRtpNetworkInterface->updateRoundTripTime(memberUpdate.getRoundTripTime());

  // call base class method to shutdown on BYE
  RtpSessionManager::handleMemberUpdate(memberUpdate);
}

void SctpH264RtpSessionManager::onSessionStarted()
{
  VLOG(15) << "RTP session started";
}

void SctpH264RtpSessionManager::onSessionStopped()
{
  VLOG(15) << "RTP session started";
}

}
}

