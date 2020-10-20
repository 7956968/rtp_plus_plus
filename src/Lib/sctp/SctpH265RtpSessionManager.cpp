#include "CorePch.h"
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/network/SctpRtpNetworkInterface.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/sctp/SctpH265RtpSessionManager.h>

namespace rtp_plus_plus
{

using media::MediaSample;

namespace sctp
{

std::unique_ptr<SctpH265RtpSessionManager> SctpH265RtpSessionManager::create(boost::asio::io_service& ioService,
                                                                           const GenericParameters& applicationParameters)
{
  return std::unique_ptr<SctpH265RtpSessionManager>( new SctpH265RtpSessionManager(ioService, applicationParameters) );
}

SctpH265RtpSessionManager::SctpH265RtpSessionManager(boost::asio::io_service& ioService,
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

void SctpH265RtpSessionManager::setSctpPolicies(const std::vector<std::string>& vPolicies )
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

void SctpH265RtpSessionManager::send(const MediaSample& mediaSample)
{
  uint32_t uiChannelId = m_sctpPolicyManager.getRtpChannelForShvcMedia(mediaSample);

  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
  // SCTP: need to select channel for each packet
  for (RtpPacket& rtpPacket : rtpPackets)
  {
    // for now hard-coding RTP for channel 1
    rtpPacket.setId(uiChannelId);
  }
  m_pRtpSession->sendRtpPackets(rtpPackets);
}

void SctpH265RtpSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
  std::vector<std::vector<MediaSample> > vGroups;
  std::vector<uint32_t> vChannels;

  // create initial group
  assert(!vMediaSamples.empty());

  uint32_t uiLastChannel = UINT_MAX;

  // group into vectors of consecutive similar channels
  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  {
    uint32_t uiChannel = m_sctpPolicyManager.getRtpChannelForShvcMedia(vMediaSamples[i]);
    if (uiChannel != uiLastChannel)
    {
      // create new group
      vGroups.push_back(std::vector<MediaSample>());
      vChannels.push_back(uiChannel);
      uiLastChannel = uiChannel;
    }

    vGroups[vGroups.size()-1].push_back(vMediaSamples[i]);

    /*NalUnitType eType = getNalUnitType(vMediaSamples[i]);
    switch (eType)
    {
      case NUT_PREFIX_NAL_UNIT:
      {
        // skip the following NAL unit
        ++i;
        // applies to following NAL units as well
        vGroups[vGroups.size()-1].push_back(vMediaSamples[i]);
        break;
      }
    }*/
  }

  uint32_t uiNtpMws = 0;
  uint32_t uiNtpLws = 0;
  // packetise and send each group with the same RTP timestamp
  uint32_t uiRtpTimestamp = m_referenceClock.getRtpTimestamp(m_pRtpSession->getRtpSessionParameters().getRtpTimestampFrequency(),
                                                             m_pRtpSession->getRtpSessionState().getRtpTimestampBase(),
                                                             vMediaSamples[0].getStartTime(), uiNtpMws, uiNtpLws);

  // NOTE: prefix NAL units must be assigned the same channel as the following NAL unit
  for (size_t i = 0; i < vGroups.size(); ++i)
  {
    auto& mediaSampleGroup = vGroups[i];
    std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSampleGroup, uiRtpTimestamp);
    std::vector<std::vector<uint16_t> > vRtpInfo = m_pRtpSession->getRtpPacketisationInfo();
    assert(vRtpInfo.size() == mediaSampleGroup.size());

    using boost::optional;
    using namespace media::h265;
    for (size_t j = 0; j < mediaSampleGroup.size(); ++j)
    {
      const MediaSample& mediaSample = mediaSampleGroup[j];
      // OK since we only using 2 layers for now
      NalUnitType eType = getNalUnitType(mediaSample);
      std::ostringstream layer;

      // get sequence numbers
      std::ostringstream ostr;
      for (uint16_t uiSN : vRtpInfo[j])
      {
        ostr << " <<" << uiSN << ">>";
      }


      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " SCTP channel: " << vChannels[i]
              << " NUT: " << eType << " " << layer.str()
              << " (" << mediaSampleGroup[j].getPayloadSize() << ") RTP SN: [ "
              << ostr.str() << "] ";
                 ;
    }

    // Update channel id for each packet
    // for now all packets in the same access unit
    // are assigned to the same channel
    for (RtpPacket& rtpPacket : rtpPackets)
    {
      rtpPacket.setId(vChannels[i]);
    }
    // send the packets
    m_pRtpSession->sendRtpPackets(rtpPackets);
  }
}

void SctpH265RtpSessionManager::handleMemberUpdate(const MemberUpdate& memberUpdate)
{
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

void SctpH265RtpSessionManager::onSessionStarted()
{
  VLOG(15) << "RTP session started";
}

void SctpH265RtpSessionManager::onSessionStopped()
{
  VLOG(15) << "RTP session started";
}

}
}

