#include "CorePch.h"
#include <rtp++/rfc4867/Rfc4867Packetiser.h>
#include <boost/foreach.hpp>

#include <cpputil/IBitStream.h>

#define DEFAULT_MTU 1500
#define DEFAULT_BUFFER_SIZE_BYTES 10000
#define IP_UDP_RTP_HEADER_SIZE 40
// #define DEBUG_RFC4867_PACKETIZATION

#define DEFAULT_MAX_RTP_SIZE 1024

namespace rtp_plus_plus
{
namespace rfc4867
{

using media::MediaSample;

Rfc4867Packetiser::ptr Rfc4867Packetiser::create(const uint32_t uiFramesPerRtpPacket)
{
  return std::unique_ptr<Rfc4867Packetiser>(new Rfc4867Packetiser(uiFramesPerRtpPacket));
}

Rfc4867Packetiser::ptr Rfc4867Packetiser::create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps, const uint32_t uiFramesPerRtpPacket)
{
  return std::unique_ptr<Rfc4867Packetiser>(new Rfc4867Packetiser(uiMtu, uiSessionBandwidthKbps, uiFramesPerRtpPacket));
}

Rfc4867Packetiser::Rfc4867Packetiser(const uint32_t uiFramesPerRtpPacket)
  :m_uiMtu(DEFAULT_MTU),
    m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
    m_uiSessionBandwidthKbps(0),
    m_uiEstimatedFrameSize(DEFAULT_BUFFER_SIZE_BYTES),
    m_uiFramesPerRtpPacket(uiFramesPerRtpPacket != 0 ? uiFramesPerRtpPacket : 1),
    m_bIsWideband(false),
    m_uiLastFrameHeader(0),
    m_uiFrameSize(0),
    m_bIsFirstPacket(true),
    m_bIsFirstFrameInPacket(true),
    m_outputStream(DEFAULT_MAX_RTP_SIZE, false)
{

}

Rfc4867Packetiser::Rfc4867Packetiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps, const uint32_t uiFramesPerRtpPacket)
  :m_uiMtu(uiMtu),
    m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
    m_uiSessionBandwidthKbps(uiSessionBandwidthKbps),
    m_uiEstimatedFrameSize(m_uiSessionBandwidthKbps > 0 ? m_uiSessionBandwidthKbps*10 : DEFAULT_BUFFER_SIZE_BYTES),
    m_uiFramesPerRtpPacket(uiFramesPerRtpPacket != 0 ? uiFramesPerRtpPacket : 1),
    m_bIsWideband(false),
    m_uiLastFrameHeader(0),
    m_uiFrameSize(0),
    m_bIsFirstPacket(true),
    m_bIsFirstFrameInPacket(true),
    m_outputStream(m_uiEstimatedFrameSize, false)
{
  // the estimated frame size is set to m_uiSessionBandwidthKbps*10 = m_uiSessionBandwidthKbps*1000/(8*12.5) assuming
  // that the framerate is 12.5 frames per second. Note: this will be resized automatically by the reading code
  // but if it proves to be inefficient we can increase the frame size
  assert(uiMtu != 0);

}

// From liveMedia
#define FT_INVALID 65535
static unsigned short frameSize[16] = {
  12, 13, 15, 17,
  19, 20, 26, 31,
  5, FT_INVALID, FT_INVALID, FT_INVALID,
  FT_INVALID, FT_INVALID, FT_INVALID, 0
};
static unsigned short frameSizeWideband[16] = {
  17, 23, 32, 36,
  40, 46, 50, 58,
  60, 5, FT_INVALID, FT_INVALID,
  FT_INVALID, FT_INVALID, 0, 0
};

void Rfc4867Packetiser::processAmrFrame(const media::MediaSample& mediaSample)
{
  // unsigned uiSize = 0;
  // double dStartTime = 0.0;
  const uint8_t* pBuffer = mediaSample.getDataBuffer().data();
#if 1
  m_uiLastFrameHeader = pBuffer[0]; // Should be pos 0 or 1???
  if ((m_uiLastFrameHeader & 0x83) != 0) {
#ifdef DEBUG
    VLOG(WARNING) << "Invalid frame header " << m_uiLastFrameHeader << " (padding bits (0x83) are not zero)";
#endif
  }
  else {
    unsigned char ft = (m_uiLastFrameHeader & 0x78) >> 3;
    m_uiFrameSize = m_bIsWideband ? frameSizeWideband[ft] : frameSize[ft];
    if (m_uiFrameSize == FT_INVALID) {
#ifdef DEBUG
      VLOG(WARNING) << "Invalid FT field " << ft << " (from frame header " << m_uiLastFrameHeader << ")";
#endif
    }
    else {
      // The frame header is OK
#ifdef DEBUG
      VLOG(2) << "Valid frame header " << m_uiLastFrameHeader << " -> ft " << ft << " -> frame size " << m_uiFrameSize;
#endif
    }
  }
#endif
}

std::vector<RtpPacket> Rfc4867Packetiser::processStoredMediaSamples()
{
  std::vector<RtpPacket> rtpPackets;
  RtpPacket rtpPacket;

  m_outputStream.reset();
  // first write payload header
  uint8_t payloadHeader = 0xF0;
  m_outputStream.write(payloadHeader, 8);
  // first pass: write tocs
  for (size_t i = 0; i < m_uiFramesPerRtpPacket; ++i)
  {
    processAmrFrame(m_vMediaSamples[i]);
    uint8_t toc = m_uiLastFrameHeader;
    if (i < m_uiFramesPerRtpPacket - 1)
      toc |= 0x80;
    else
      toc &=~ 0x80;

    VLOG(12) << "Sample size: " << m_vMediaSamples[i] .getPayloadSize() << " Frame header: " << m_uiLastFrameHeader << " value: " << (int)m_uiLastFrameHeader << " TOC: " << toc << " value: " << (int)toc;

    uint8_t uiD1 = *(m_vMediaSamples[i].getDataBuffer().data() + 1);
    uint8_t uiD2 = *(m_vMediaSamples[i].getDataBuffer().data() + 2);
    uint8_t uiD3 = *(m_vMediaSamples[i].getDataBuffer().data() + 3);
    uint8_t uiD4 = *(m_vMediaSamples[i].getDataBuffer().data() + 4);
    VLOG(12) << "Frame data: " << (int)uiD1 << " " << (int)uiD2 << " " << (int)uiD3 << " " << (int)uiD4;

    m_outputStream.write(toc, 8);
  }

  // second pass: write remainder of frames
  for (size_t i = 0; i < m_uiFramesPerRtpPacket; ++i)
  {
    const uint8_t* pData = m_vMediaSamples[i].getDataBuffer().data() + 1;
    bool bRes = m_outputStream.writeBytes( pData, m_vMediaSamples[i].getPayloadSize() - 1);
    if (!bRes)
    {
      LOG_FIRST_N(WARNING, 1) << "Failed to write AMR data to output stream";
    }
  }

  Buffer frames = m_outputStream.data();
  if (frames.getSize() > m_uiMtu)
  {
    LOG_FIRST_N(WARNING, 1) << "Framed payload is bigger than MTU!";
  }
  else
  {
    VLOG(15) << "Packetised AMR into RTP payload of size " << frames.getSize();
  }
  rtpPacket.setPayload(frames);
  
  // TODO: marker bit?
  rtpPacket.getHeader().setMarkerBit(true);
  rtpPackets.push_back(rtpPacket);

  m_vMediaSamples.clear();
  return rtpPackets;
}

std::vector<RtpPacket> Rfc4867Packetiser::packetise(const MediaSample& mediaSample)
{
  m_vMediaSamples.push_back(mediaSample);
  if (m_vMediaSamples.size() == m_uiFramesPerRtpPacket)
  {
    // packetise sample
    return processStoredMediaSamples();
  }
  return std::vector<RtpPacket>();
}

std::vector<RtpPacket> Rfc4867Packetiser::packetise(const std::vector<MediaSample>& mediaSamples)
{
  std::vector<RtpPacket> rtpPackets;
  for (size_t i = 0; i < mediaSamples.size(); ++i)
  {
    m_vMediaSamples.push_back(mediaSamples[i]);
    if (m_vMediaSamples.size() == m_uiFramesPerRtpPacket)
    {
      // packetise sample
      std::vector<RtpPacket> rtpPacketsInCurrentFrame = processStoredMediaSamples();
      rtpPackets.insert(rtpPackets.end(), rtpPacketsInCurrentFrame.begin(), rtpPacketsInCurrentFrame.end());
    }
  }
  return rtpPackets;
}

std::vector<MediaSample> Rfc4867Packetiser::depacketize(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();

  for (const RtpPacket& rtpPacket : rtpPackets)
  {
    IBitStream in(rtpPacket.getPayload());
    uint8_t payloadHeader = 0xF0;
    in.read(payloadHeader, 8);
    assert(payloadHeader == 0xF0);
    // read tocs
    uint8_t toc = 0;
    in.read(toc, 8);
    uint32_t uiPacketsToBeRead = 0;
    while ((toc & 0x80) != 0)
    {
      ++uiPacketsToBeRead;
      in.read(toc, 8);
    }
    // last or single packet
    ++uiPacketsToBeRead;
    VLOG(10) << "RTP packet size: " << rtpPacket.getPayload().getSize() << " packets to be read: " << uiPacketsToBeRead;
    unsigned char ft = (toc & 0x78) >> 3;
    m_uiFrameSize = m_bIsWideband ? frameSizeWideband[ft] : frameSize[ft];
    if (m_uiFrameSize == FT_INVALID) 
    {
#ifdef DEBUG
      LOG(WARNING) << "Invalid FT field " << ft << " (from frame header " << m_uiLastFrameHeader << ")";
#endif
    }
    else 
    {
      // The frame header is OK
#ifdef DEBUG
      VLOG(2) << "Valid frame header " << m_uiLastFrameHeader << " -> ft " << ft << " -> frame size " << m_uiFrameSize;
#endif
    }

    uint8_t* pSource = const_cast<uint8_t*>(rtpPacket.getPayload().data()) + 1 + uiPacketsToBeRead;
    for (size_t i = 0; i < uiPacketsToBeRead; ++i)
    {
      MediaSample mediaSample;
      uint8_t* pData = new uint8_t[m_uiFrameSize + 1];
      pData[0] = (toc & ~0x80);
      memcpy(pData + 1, pSource, m_uiFrameSize);
      VLOG(12) << "Framesize: " << m_uiFrameSize << " header: " << pData[0] << " value: " << (int)pData[0] << " TOC: " << toc << " value: " << (int)toc;
      Buffer payload(pData, m_uiFrameSize + 1);
      mediaSample.setData(payload);
      mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
      mediaSample.setMarker(rtpPacketGroup.isRtcpSynchronised());
      uint8_t uiD0 = *(mediaSample.getDataBuffer().data() + 0);
      uint8_t uiD1 = *(mediaSample.getDataBuffer().data() + 1);
      uint8_t uiD2 = *(mediaSample.getDataBuffer().data() + 2);
      uint8_t uiD3 = *(mediaSample.getDataBuffer().data() + 3);
      uint8_t uiD4 = *(mediaSample.getDataBuffer().data() + 4);
      VLOG(12) << "Frame data: " << (int)uiD0 << " " << (int)uiD1 << " " << (int)uiD2 << " " << (int)uiD3 << " " << (int)uiD4;
      vSamples.push_back(mediaSample);
      pSource += (m_uiFrameSize - 1);
    }
  }
#if 0
  // first pass: write tocs
  for (size_t i = 0; i < m_uiFramesPerRtpPacket; ++i)
  {
    processAmrFrame(m_vMediaSamples[i]);
    uint8_t toc = m_uiLastFrameHeader;
    if (i < m_uiFramesPerRtpPacket - 1)
      toc |= 0x80;
    else
      toc &= ~0x80;

    m_outputStream.write(toc, 8);
  }
  // second pass: write remainder of frames
  for (size_t i = 0; i < m_uiFramesPerRtpPacket; ++i)
  {
    const uint8_t* pData = m_vMediaSamples[i].getDataBuffer().data() + 1;
    bool bRes = m_outputStream.writeBytes(pData, m_vMediaSamples[i].getPayloadSize() - 1);
    if (!bRes)
    {
      LOG_FIRST_N(WARNING, 1) << "Failed to write AMR data to output stream";
    }
  }
#endif
  return vSamples;
}

} // rfc 4867
} // rtp_plus_plus
