#include "CorePch.h"
#include <limits>
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/sctp/SctpShvcRtpSessionManager.h>


namespace rtp_plus_plus
{

using namespace media::h265;
using media::MediaSample;

namespace sctp
{

std::unique_ptr<SctpShvcRtpSessionManager> SctpShvcRtpSessionManager::create(boost::asio::io_service& ioService,
                                                                           const GenericParameters& applicationParameters)
{
  return std::unique_ptr<SctpShvcRtpSessionManager>( new SctpShvcRtpSessionManager(ioService, applicationParameters) );
}

SctpShvcRtpSessionManager::SctpShvcRtpSessionManager(boost::asio::io_service& ioService,
                                                   const GenericParameters& applicationParameters)
  :SctpH265RtpSessionManager(ioService, applicationParameters)
{

}

void SctpShvcRtpSessionManager::send(const MediaSample& mediaSample)
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

void SctpShvcRtpSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
#if 1
  // TODO: get channel for each item in vector
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

    VLOG(15) << "Channel: " << vChannels[i]
            << " Samples: " << mediaSampleGroup.size()
            << " RTP packets: " << rtpPackets.size();

    // need to select channel for each layer
    for (RtpPacket& rtpPacket : rtpPackets)
    {
      // for now hard-coding RTP for channel 1
      rtpPacket.setId(vChannels[i]);
    }
    m_pRtpSession->sendRtpPackets(rtpPackets);
  }

#else
  std::vector<MediaSample> vBaseLayerSamples;
  std::vector<MediaSample> vEnhancementLayerSamples;
  // split media samples into base and enhancement layer ones before calling packetise
  media::h264::svc::splitMediaSamplesIntoBaseAndEnhancementLayers(vMediaSamples, vBaseLayerSamples, vEnhancementLayerSamples);

  // send base layer samples
  if (!vBaseLayerSamples.empty())
  {
    uint32_t uiChannelId = m_sctpPolicy.getRtpChannelForSvcPolicy(sctp::LAYER_TYPE_BL);
    std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
  #if 0
    DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
               << " Size: " << mediaSample.getDataBuffer().getSize()
               << " into " << rtpPackets.size() << " RTP packets";

  #endif
    // need to select channel for each layer
    for (RtpPacket& rtpPacket : rtpPackets)
    {
      // for now hard-coding RTP for channel 1
      rtpPacket.setId(uiChannelId);
    }
    m_pRtpSession->sendRtpPackets(rtpPackets);
  }

  if (!vEnhancementLayerSamples.empty())
  {
    uint32_t uiChannelId = m_sctpPolicy.getRtpChannelForSvcPolicy(sctp::LAYER_TYPE_EL);
    std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
  #if 0
    DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
               << " Size: " << mediaSample.getDataBuffer().getSize()
               << " into " << rtpPackets.size() << " RTP packets";

  #endif
    // need to select channel for each layer
    for (RtpPacket& rtpPacket : rtpPackets)
    {
      // for now hard-coding RTP for channel 1
      rtpPacket.setId(uiChannelId);
    }
    m_pRtpSession->sendRtpPackets(rtpPackets);
  }
#endif
}

}
}

