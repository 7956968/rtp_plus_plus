#pragma once
#include <vector>
#include <cpputil/OBitStream.h>
#include <rtp++/PayloadPacketiserBase.h>

#define DEFAULT_MTU 1500
#define DEFAULT_FRAMES_PER_RTP_PACKET 1

namespace rtp_plus_plus
{
namespace rfc4867
{

/// @def DEBUG_RFC4867_PACKETIZATION Verbose logging during parsing
// #define DEBUG_RFC4867_PACKETIZATION

/**
  * Basic RFC4867 RTP packetiser implementation based on live555 implementation
  */
class Rfc4867Packetiser : public PayloadPacketiserBase
{
public:
  typedef std::unique_ptr<Rfc4867Packetiser> ptr;

  static ptr create(const uint32_t uiFramesPerRtpPacket = DEFAULT_FRAMES_PER_RTP_PACKET);
  static ptr create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps, const uint32_t uiFramesPerRtpPacket = DEFAULT_FRAMES_PER_RTP_PACKET);

  Rfc4867Packetiser(const uint32_t uiFramesPerRtpPacket = DEFAULT_FRAMES_PER_RTP_PACKET);
  Rfc4867Packetiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps, const uint32_t uiFramesPerRtpPacket = DEFAULT_FRAMES_PER_RTP_PACKET);

  uint32_t getMtu() const { return m_uiMtu; }
  void setMtu(const uint32_t uiMtu) { m_uiMtu = uiMtu; }

  virtual std::vector<RtpPacket> packetise(const media::MediaSample& mediaSample);
  virtual std::vector<RtpPacket> packetise(const std::vector<media::MediaSample>& mediaSample);

  // Not every call to depacketize results in a media sample
  // some calls might results in multiple samples
  virtual std::vector<media::MediaSample> depacketize(const RtpPacketGroup& rtpPacketGroup);

private:

  void processAmrFrame(const media::MediaSample& mediaSample);
  std::vector<RtpPacket> processStoredMediaSamples();

  uint32_t m_uiMtu;
  uint32_t m_uiBytesAvailableForPayload;
  uint32_t m_uiSessionBandwidthKbps;
  uint32_t m_uiEstimatedFrameSize;
  uint32_t m_uiFramesPerRtpPacket;
  bool m_bIsWideband;

  uint8_t m_uiLastFrameHeader;
  uint32_t m_uiFrameSize;
  bool m_bIsFirstPacket;
  bool m_bIsFirstFrameInPacket;

  /**
   * vector to store media samples until packetisation
   */
  std::vector<media::MediaSample> m_vMediaSamples;

  OBitStream m_outputStream;
};

static std::unique_ptr<rfc4867::Rfc4867Packetiser> create(const uint32_t uiFramesPerRtpPacket = DEFAULT_FRAMES_PER_RTP_PACKET)
{
  return std::unique_ptr<rfc4867::Rfc4867Packetiser>(new Rfc4867Packetiser(uiFramesPerRtpPacket));
}

} // rfc 4867
} // rtp_plus_plus
