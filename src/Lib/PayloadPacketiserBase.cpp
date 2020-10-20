#include "CorePch.h"
#include <rtp++/PayloadPacketiserBase.h>

namespace rtp_plus_plus
{

using media::MediaSample;

std::unique_ptr<PayloadPacketiserBase> PayloadPacketiserBase::create()
{
  return std::unique_ptr<PayloadPacketiserBase>(new PayloadPacketiserBase());
}

PayloadPacketiserBase::~PayloadPacketiserBase()
{

}

std::vector<RtpPacket> PayloadPacketiserBase::packetise(const MediaSample& mediaSample)
{
  // simple one to one packetization
  std::vector<RtpPacket> vRtpPackets;
  RtpPacket packet;
  packet.setPayload(mediaSample.getDataBuffer());
  vRtpPackets.push_back(packet);
  return vRtpPackets;
}

std::vector<RtpPacket> PayloadPacketiserBase::packetise(const std::vector<MediaSample>& vMediaSamples)
{
  // simple one to one packetization
  std::vector<RtpPacket> vRtpPackets;
  for (auto& mediaSample : vMediaSamples)
  {
    RtpPacket packet;
    packet.setPayload(mediaSample.getDataBuffer());
    vRtpPackets.push_back(packet);
  }
  return vRtpPackets;
}

std::vector<MediaSample> PayloadPacketiserBase::depacketize(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  for (auto& rtpPacket : rtpPacketGroup.getRtpPackets())
  {
    MediaSample mediaSample;
    mediaSample.setData(rtpPacket.getPayload());
    mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
    mediaSample.setMarker(rtpPacketGroup.isRtcpSynchronised());
    vSamples.push_back(mediaSample);
  }
  return vSamples;
}

} // rtp_plus_plus
