#pragma once
#include <vector>
#include <cpputil/OBitStream.h>
#include <rtp++/PayloadPacketiserBase.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>

#define DEFAULT_MTU 1500

namespace rtp_plus_plus
{
namespace rfchevc
{

/// @def DEBUG_NAL_UNIT_TYPES Debug information about the next Nal Unit Type
// #define DEBUG_NAL_UNIT_TYPES

/// @def DEBUG_RFC6184_PACKETIZATION Verbose logging during parsing
// #define DEBUG_RFC6184_PACKETIZATION

/**
  * RFC6184 RTP packetiser implementation
  * This implementation currently implements SINGLE_NAL_UNIT_MODE and NON_INTERLEAVED_MODE.
  * NOTE: The packetiser has to buffer packets until it can determine that an
  * access unit is complete in order to be able to set the sample time as well as the marker bit.
  * The rules for determining the end of an access unit are in the standard
  * But for simplicity sake, we have configured ffmpeg to output Access Unit Delimiters
  * in the stream:
  * THIS CODE WILL NOT FUNCTION CORRECTLY IF AUDs ARE NOT PRESENT.
  */
class RfchevcPacketiser : public PayloadPacketiserBase
{
public:
  typedef std::unique_ptr<RfchevcPacketiser> ptr;

  enum PacketizationMode
  {
    NON_INTERLEAVED_MODE = 0,
    INTERLEAVED_MODE     = 1
  };

  static ptr create();
  static ptr create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps);

  RfchevcPacketiser();
  RfchevcPacketiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps);

  PacketizationMode getPacketizationMode() const { return m_eMode; }
  void setPacketizationMode(PacketizationMode eMode) { m_eMode = eMode; }

  uint32_t getMtu() const { return m_uiMtu; }
  void setMtu(const uint32_t uiMtu) { m_uiMtu = uiMtu; }

  virtual std::vector<RtpPacket> packetise(const media::MediaSample& mediaSample);
  virtual std::vector<RtpPacket> packetise(const std::vector<media::MediaSample>& mediaSample);

  // Not every call to depacketize results in a media sample
  // some calls might results in multiple samples
  virtual std::vector<media::MediaSample> depacketize(const RtpPacketGroup& rtpPacketGroup);

private:
  bool handleAPA(IBitStream& in, std::vector<media::MediaSample>& vSamples, uint32_t RtpTime);
  bool handleAPB(IBitStream& in, std::vector<media::MediaSample>& vSamples, uint32_t RtpTime);

  std::list<RtpPacket>::const_iterator handleFuA(IBitStream& in, const std::list<RtpPacket>& rtpPackets, std::list<RtpPacket>::const_iterator itNextFu, std::vector<media::MediaSample>& vSamples, uint32_t uiStartSN, uint32_t forbidden, uint32_t layerId, uint32_t tempId, uint32_t RtpTime );
  std::list<RtpPacket>::const_iterator handleFuB(IBitStream& in, const std::list<RtpPacket>& rtpPackets, std::list<RtpPacket>::const_iterator itNextFu, std::vector<media::MediaSample>& vSamples, uint32_t uiStartSN, uint32_t forbidden, uint32_t layerId, uint32_t tempId, uint32_t RtpTime );

  std::list<RtpPacket>::const_iterator extractNextMediaSample(const std::list<RtpPacket>& rtpPackets, std::list<RtpPacket>::const_iterator itStart, std::vector<media::MediaSample>& vMediaSamples);

  RtpPacket packetiseSingleNalUnit(const media::MediaSample& mediaSample);
  std::vector<RtpPacket> packetiseNonInterleaved(const media::MediaSample& mediaSample);
  std::vector<RtpPacket> packetiseNonInterleaved(const std::vector<media::MediaSample>& mediaSamples);

  std::vector<RtpPacket> packetiseInterleaved(const media::MediaSample& mediaSample);
  std::vector<RtpPacket> packetiseInterleaved(const std::vector<media::MediaSample>& mediaSamples);

  std::vector<media::MediaSample> depacketizeSingleNalUnit(const std::list<RtpPacket>& rtpPackets);
  std::vector<media::MediaSample> depacketizeNonInterleaved(const std::list<RtpPacket>& rtpPackets);
  std::vector<media::MediaSample> depacketizeInterleaved(const std::list<RtpPacket>& rtpPackets);

  std::vector<RtpPacket> packetiseFuA(const media::MediaSample& mediaSample);
  std::vector<RtpPacket> packetiseFuB(const media::MediaSample& mediaSample);

  /**
   * @brief getNumberOfSamplesToAggregate This method returns
   * @param uiBytesAvailableForPayload The bytes available for aggregation
   * @param mediaSamples The vector containing the media samples
   * @param uiStartIndex The start index into mediaSamples
   * @param uiStapSize The size of the resulting STAP-A
   * @param bFBit The F-bit according to RFC6184.
   * @param uiNri The NRI-bits according to RFC6184
   * @return the number of samples in the vector that can be aggregated into an STAP-A
   */
  int32_t getNumberOfSamplesToAggregate(uint32_t uiBytesAvailableForPayload, const std::vector<media::MediaSample>& mediaSamples,
                                        uint32_t uiStartIndex, uint32_t& uiStapSize,
                                        bool& bFBit,  uint32_t& uiLayerId, uint32_t& uiTempId, bool Interleaved=false);


  /// for debugging
  void printPacketListInfo(const std::list<RtpPacket>& dataList);

  PacketizationMode m_eMode;
  uint32_t m_uiMtu;
  uint32_t m_uiBytesAvailableForPayload;
  uint32_t m_uiSessionBandwidthKbps;
  uint32_t m_uiEstimatedFrameSize;

  OBitStream m_outputStream;
};

static std::unique_ptr<rfchevc::RfchevcPacketiser> create()
{
  return std::unique_ptr<rfchevc::RfchevcPacketiser>(new RfchevcPacketiser());
}

}
}

