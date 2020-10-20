#include "CorePch.h"
#include <rtp++/rfc6184/Rfc6184Packetiser.h>
#include <boost/foreach.hpp>

#include <cpputil/IBitStream.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>

#define DEFAULT_MTU 1500
#define DEFAULT_BUFFER_SIZE_BYTES 10000
#define IP_UDP_RTP_HEADER_SIZE 40
// #define DEBUG_RFC6184_PACKETIZATION

namespace rtp_plus_plus
{
namespace rfc6184
{

using namespace media::h264;
using media::MediaSample;

Rfc6184Packetiser::ptr Rfc6184Packetiser::create()
{
  return std::unique_ptr<Rfc6184Packetiser>(new Rfc6184Packetiser());
}

Rfc6184Packetiser::ptr Rfc6184Packetiser::create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
{
  return std::unique_ptr<Rfc6184Packetiser>(new Rfc6184Packetiser(uiMtu, uiSessionBandwidthKbps));
}

Rfc6184Packetiser::Rfc6184Packetiser()
  :m_eMode(SINGLE_NAL_UNIT_MODE), // default mode
    m_uiMtu(DEFAULT_MTU),
    m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
    m_uiSessionBandwidthKbps(0),
    m_uiEstimatedFrameSize(DEFAULT_BUFFER_SIZE_BYTES),
    m_outputStream(m_uiEstimatedFrameSize, false),
    m_bDisableStap(false)
{

}

Rfc6184Packetiser::Rfc6184Packetiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
  :m_eMode(SINGLE_NAL_UNIT_MODE), // default mode
    m_uiMtu(uiMtu),
    m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
    m_uiSessionBandwidthKbps(uiSessionBandwidthKbps),
    m_uiEstimatedFrameSize(m_uiSessionBandwidthKbps > 0 ? m_uiSessionBandwidthKbps*10 : DEFAULT_BUFFER_SIZE_BYTES),
    m_outputStream(m_uiEstimatedFrameSize, false),
    m_bDisableStap(false)
{
  // the estimated frame size is set to m_uiSessionBandwidthKbps*10 = m_uiSessionBandwidthKbps*1000/(8*12.5) assuming
  // that the framerate is 12.5 frames per second. Note: this will be resized automatically by the reading code
  // but if it proves to be inefficient we can increase the frame size
  assert(uiMtu != 0);

}


std::vector<RtpPacket> Rfc6184Packetiser::packetise(const MediaSample& mediaSample)
{
#ifdef DEBUG_NAL_UNIT_TYPES
  const uint8_t* pData = mediaSample.getDataBuffer().data();
  uint8_t uiNut = pData[0] & 0x1F;
  rfc6184::NalUnitType eNextType = static_cast<rfc6184::NalUnitType>(uiNut);
  DLOG(INFO) << "Next NAL type: " << rfc6184::toString(eNextType);
#endif

  switch(m_eMode)
  {
    case NON_INTERLEAVED_MODE:
    {
      return packetiseNonInterleaved(mediaSample);
      break;
    }
    case INTERLEAVED_MODE:
    {
      return packetiseInterleaved(mediaSample);
      break;
    }
    default:
    case SINGLE_NAL_UNIT_MODE:
    {
      std::vector<RtpPacket> rtpPackets;
      rtpPackets.push_back(packetiseSingleNalUnit(mediaSample));
      return rtpPackets;
      break;
    }
  }
}

std::vector<RtpPacket> Rfc6184Packetiser::packetise(const std::vector<MediaSample>& mediaSamples)
{
// #define DEBUG_NAL_UNIT_TYPES
#ifdef DEBUG_NAL_UNIT_TYPES
  for (const media::MediaSample& mediaSample : mediaSamples)
  {
    media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
    VLOG(2) << "Rfc6184Packetiser::packetise NALU Size: " << mediaSample.getPayloadSize() << " Type info: " << media::h264::toString(eType);
    for (int i = 0; i < 5; ++i)
    {
      VLOG(2) << "Rfc6184Packetiser::packetise NALU[" << i << "]: " << (int)mediaSample.getDataBuffer().data()[i];
    }
  }
#endif

  m_vLastPacketisationInfo.clear();
  for (size_t i = 0; i < mediaSamples.size(); ++i )
  {
    m_vLastPacketisationInfo.push_back(std::vector<uint32_t>());
  }

  switch(m_eMode)
  {
    case NON_INTERLEAVED_MODE:
    {
      return packetiseNonInterleaved(mediaSamples);
      break;
    }
    case INTERLEAVED_MODE:
    {
      return packetiseInterleaved(mediaSamples);
      break;
    }
    default:
    case SINGLE_NAL_UNIT_MODE:
    {
      std::vector<RtpPacket> rtpPackets;
      uint32_t uiIndex = 0;
      for (auto& mediaSample : mediaSamples)
      {
        rtpPackets.push_back(packetiseSingleNalUnit(mediaSample));
        m_vLastPacketisationInfo[uiIndex].push_back(uiIndex);
        ++uiIndex;
      }
      return rtpPackets;
      break;
    }
  }
}

bool Rfc6184Packetiser::handleStapA(IBitStream& in, std::vector<MediaSample>& vSamples, const RtpPacketGroup& rtpPacketGroup)
{
  bool bError = false;
  uint32_t uiSize = 0;
  while (!bError && in.read(uiSize, 16))
  {
    if (uiSize > 0)
    {
      // allocate memory for NAL unit
      uint8_t* pBuffer = new uint8_t[uiSize];
      Buffer dataBuffer(pBuffer, uiSize);
      if (in.readBytes(pBuffer, uiSize))
      {
#ifdef DEBUG_RFC6184_PACKETIZATION
        uint32_t stap_forbidden = 0, stap_nri = 0, stap_type = 0;
        IBitStream inTemp(dataBuffer);
        if (( inTemp.read(stap_forbidden, 1)) &&
            ( inTemp.read(stap_nri, 2)) &&
            ( inTemp.read(stap_type, 5 )))
        {
          VLOG(1) << "STAP A: Parsed NAL unit type: " << stap_type << " " << toString((NalUnitType)stap_type);
        }
#endif
        MediaSample mediaSample;
        mediaSample.setData(dataBuffer);
        mediaSample.setRtpTime(rtpPacketGroup.getRtpTimestamp());
        mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
        vSamples.push_back(mediaSample);
      }
      else
      {
        bError = true;
        LOG(WARNING) << "Failed to read STAP A Nal unit data";
      }
    }
    else
    {
      DLOG(INFO) << "TODO: Zero size: padding?";
    }
  }
  return !bError;
}

bool Rfc6184Packetiser::handleStapB(IBitStream& in, std::vector<MediaSample>& vSamples, const RtpPacketGroup& rtpPacketGroup)
{
  bool bError = false;
  uint32_t uiSize = 0;
  uint16_t uiDON = 0;
  in.read(uiDON, 16);
  while (!bError && in.read(uiSize, 16))
  {
    if (uiSize > 0)
    {
      // allocate memory for NAL unit
      uint8_t* pBuffer = new uint8_t[uiSize];
      Buffer dataBuffer(pBuffer, uiSize);
      if (in.readBytes(pBuffer, uiSize))
      {
#ifdef DEBUG_RFC6184_PACKETIZATION
        uint32_t stap_forbidden = 0, stap_nri = 0, stap_type = 0;
        IBitStream inTemp(dataBuffer);
        if (( inTemp.read(stap_forbidden, 1)) &&
            ( inTemp.read(stap_nri, 2)) &&
            ( inTemp.read(stap_type, 5 )))
        {
          VLOG(1) << "STAP A: Parsed NAL unit type: " << stap_type << " " << toString((NalUnitType)stap_type);
        }
#endif
        MediaSample mediaSample;
        mediaSample.setData(dataBuffer);
        mediaSample.setDecodingOrderNumber(uiDON++);
        mediaSample.setRtpTime(rtpPacketGroup.getRtpTimestamp());
        mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
        vSamples.push_back(mediaSample);
      }
      else
      {
        bError = true;
        LOG(WARNING) << "Failed to read STAP A Nal unit data";
      }
    }
    else
    {
      DLOG(INFO) << "TODO: Zero size: padding?";
    }
  }
  return !bError;
}


std::list<RtpPacket>::const_iterator Rfc6184Packetiser::handleFuA(IBitStream& in,
                                                              const std::list<RtpPacket>& rtpPackets,
                                                              std::list<RtpPacket>::const_iterator itNextFu,
                                                              std::vector<MediaSample>& vSamples,
                                                              uint32_t uiStartSN,
                                                              uint32_t forbidden,
                                                              uint32_t nri,
                                                              const RtpPacketGroup& rtpPacketGroup)
{
  uint32_t fu_start = 0, fu_end = 0, fu_reserved = 0, fu_type = 0;
  if (( in.read(fu_start,    1) ) &&
      ( in.read(fu_end,      1) ) &&
      ( in.read(fu_reserved, 1) ) &&
      ( in.read(fu_type,     5) )
     )
  {
#ifdef DEBUG_RFC6184_PACKETIZATION
    VLOG(2) << "First FU: SN: " << uiStartSN << " start: " << fu_start << " end: " << fu_end << " res: " << fu_reserved << " Type: " << rfc6184::toString((NalUnitType)fu_type);
#endif

    // make sure start is set for first FU
    if ( fu_start != 0 )
    {
      assert( fu_end == 0);
      assert( fu_reserved == 0 );

      m_outputStream.reset();
      // write NAL unit header based on FU indicator and FU header
      m_outputStream.write(forbidden, 1);
      m_outputStream.write(nri, 2);
      m_outputStream.write(fu_type, 5);
      // write rest of FU into output NAL unit
      m_outputStream.write(in);
      // check for all subsequent packets and end bits
      uint32_t uiPreviousSN = uiStartSN;
      // search for end of FU until either end of list or a non-FU unit is encountered
      for (std::list<RtpPacket>::const_iterator itNext = itNextFu; itNext != rtpPackets.end(); ++itNext)
      {
        const RtpPacket& nextFU = *itNext;
        uint32_t uiNextSN = nextFU.getExtendedSequenceNumber();
        if (uiNextSN == uiPreviousSN + 1)
        {
          Buffer fuData = nextFU.getPayload();
          IBitStream inNextFu(fuData);

          uint32_t forbidden = 0, nri = 0, type = 0;
          uint32_t nextfu_start = 0, nextfu_end = 0, nextfu_reserved = 0, nextfu_type = 0;
          // read FU indicator
          // read FU header
          if (( inNextFu.read(forbidden, 1)) &&
              ( inNextFu.read(nri, 2)) &&
              ( inNextFu.read(type, 5 )) &&
              ( inNextFu.read(nextfu_start,    1) ) &&
              ( inNextFu.read(nextfu_end,      1) ) &&
              ( inNextFu.read(nextfu_reserved, 1) ) &&
              ( inNextFu.read(nextfu_type,     5) )
             )
          {
#ifdef DEBUG_RFC6184_PACKETIZATION
            VLOG(2) << "Next FU: SN: " << nextFU.getExtendedSequenceNumber() << " start: " << nextfu_start << " end: " << nextfu_end << " res: " << nextfu_reserved << " Type: " << rfc6184::toString((NalUnitType)nextfu_type);
#endif
            if (type != NUT_FU_A)
            {
              // something's gone wrong: this is a different NALU
#ifdef DEBUG_RFC6184_PACKETIZATION
              printPacketListInfo(rtpPackets);
#endif
              // return the current iterator so the next NALU can be extracted
              return itNext;
            }
            else
            {
              assert( nextfu_start == 0);
              assert( nextfu_reserved == 0 );
              if (nextfu_end != 0)
              {
#ifdef DEBUG_RFC6184_PACKETIZATION
                VLOG(2) << "Found end of FU ";
#endif
                m_outputStream.write(inNextFu);
                Buffer outputNalUnit = m_outputStream.data();
#ifdef DEBUG_RFC6184_PACKETIZATION
                VLOG(1) << "Successfully put FU together - Size: " << outputNalUnit.getSize();
#endif
                MediaSample mediaSample;
                mediaSample.setData(outputNalUnit);
                mediaSample.setRtpTime(rtpPacketGroup.getRtpTimestamp());
                mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
                vSamples.push_back(mediaSample);
                return ++itNext;
              }
              else
              {
                m_outputStream.write(inNextFu);
                uiPreviousSN = uiNextSN;
              }
            }
          }
          else
          {
            LOG(WARNING) << "Failed to parse next FU A header";
            return rtpPackets.end();
            // TODO: could try to continue parsing?
            //return ++itNext;
          }
        }
        else
        {
          // missing packet
          LOG(WARNING) << "Missing sequence number in FU - SN: " << uiNextSN << " Prev: " << uiPreviousSN;
          // discard previous data
          // For debugging
#ifdef DEBUG_RFC6184_PACKETIZATION
          printPacketListInfo(rtpPackets);
#endif
          return rtpPackets.end();
          // TODO: could try to continue parsing?
        }
      }
      return rtpPackets.end();
    }
    else
    {
      // start not set on first FU
      LOG(WARNING) << "TODO: start bit not set on first FU. SN: " << uiStartSN;
      return rtpPackets.end();
      // TODO: could try to continue parsing?
      //return itNextFu;
    }
  }
  else
  {
    LOG(WARNING) << "Failed to read FU A header";
    return rtpPackets.end();
    // TODO: could try to continue parsing?
    //return itNextFu;
  }
}

std::list<RtpPacket>::const_iterator Rfc6184Packetiser::handleFuB(IBitStream& in,
                                                              const std::list<RtpPacket>& rtpPackets,
                                                              std::list<RtpPacket>::const_iterator itNextFu,
                                                              std::vector<MediaSample>& vSamples,
                                                              uint32_t uiStartSN,
                                                              uint32_t forbidden,
                                                              uint32_t nri,
                                                              const RtpPacketGroup& rtpPacketGroup)
{
  uint32_t fu_start = 0, fu_end = 0, fu_reserved = 0, fu_type = 0;
  uint16_t uiDON = 0 ;
  if (( in.read(fu_start,    1) ) &&
      ( in.read(fu_end,      1) ) &&
      ( in.read(fu_reserved, 1) ) &&
      ( in.read(fu_type,     5) ) &&
      ( in.read(uiDON,     16) )
     )
  {
#ifdef DEBUG_RFC6184_PACKETIZATION
    VLOG(2) << "First FU: SN: " << uiStartSN << " start: " << fu_start << " end: " << fu_end << " res: " << fu_reserved << " Type: " << rfc6184::toString((NalUnitType)fu_type);
#endif

    // make sure start is set for first FU
    if ( fu_start != 0 )
    {
      assert( fu_end == 0);
      assert( fu_reserved == 0 );

      m_outputStream.reset();
      // write NAL unit header based on FU indicator and FU header
      m_outputStream.write(forbidden, 1);
      m_outputStream.write(nri, 2);
      m_outputStream.write(fu_type, 5);
      // write rest of FU into output NAL unit
      m_outputStream.write(in);
      // check for all subsequent packets and end bits
      uint32_t uiPreviousSN = uiStartSN;
      // search for end of FU until either end of list or a non-FU unit is encountered
      for (std::list<RtpPacket>::const_iterator itNext = itNextFu; itNext != rtpPackets.end(); )
      {
        const RtpPacket& nextFU = *itNext;
        uint32_t uiNextSN = nextFU.getExtendedSequenceNumber();
        if (uiNextSN == uiPreviousSN + 1)
        {
          Buffer fuData = nextFU.getPayload();
          IBitStream inNextFu(fuData);

          uint32_t type = 0;
          uint32_t nextfu_start = 0, nextfu_end = 0, nextfu_reserved = 0, nextfu_type = 0;
          // read FU indicator
          // read FU header
          if (( inNextFu.read(forbidden, 1)) &&
              ( inNextFu.read(nri, 2)) &&
              ( inNextFu.read(type, 5 )) &&
              ( inNextFu.read(nextfu_start,    1) ) &&
              ( inNextFu.read(nextfu_end,      1) ) &&
              ( inNextFu.read(nextfu_reserved, 1) ) &&
              ( inNextFu.read(nextfu_type,     5) )
              )
          {
#ifdef DEBUG_RFC6184_PACKETIZATION
            VLOG(2) << "Next FU: SN: " << nextFU.getExtendedSequenceNumber() << " start: " << nextfu_start << " end: " << nextfu_end << " res: " << nextfu_reserved << " Type: " << rfc6184::toString((NalUnitType)nextfu_type);
#endif
            if (type != NUT_FU_A)
            {
              // something's gone wrong: this is a different NALU
#ifdef DEBUG_RFC6184_PACKETIZATION
              printPacketListInfo(rtpPackets);
#endif
              // return the current iterator so the next NALU can be extracted
              return itNext;
            }
            else
            {
              assert( nextfu_start == 0);
              assert( nextfu_reserved == 0 );
              if (nextfu_end != 0)
              {
#ifdef DEBUG_RFC6184_PACKETIZATION
                VLOG(2) << "Found end of FU ";
#endif
                m_outputStream.write(inNextFu);
                Buffer outputNalUnit = m_outputStream.data();
#ifdef DEBUG_RFC6184_PACKETIZATION
                VLOG(1) << "Successfully put FU together - Size: " << outputNalUnit.getSize();
#endif
                MediaSample mediaSample;
                mediaSample.setData(outputNalUnit);
                mediaSample.setDecodingOrderNumber(uiDON);
                mediaSample.setRtpTime(rtpPacketGroup.getRtpTimestamp());
                mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
                vSamples.push_back(mediaSample);
                return ++itNext;
              }
              else
              {
                m_outputStream.write(inNextFu);
                uiPreviousSN = uiNextSN;
              }
            }
          }
          else
          {
            LOG(WARNING) << "Failed to parse next FU A header";
            return rtpPackets.end();
            // TODO: could try to continue parsing?
            //return ++itNext;
          }
        }
        else
        {
          // missing packet
          LOG(WARNING) << "Missing sequence number in FU - SN: " << uiNextSN << " Prev: " << uiPreviousSN;
          // discard previous data
          // For debugging
#ifdef DEBUG_RFC6184_PACKETIZATION
          printPacketListInfo(rtpPackets);
#endif
          return rtpPackets.end();
          // TODO: could try to continue parsing?
        }
        ++itNext;
      }
      return rtpPackets.end();
    }
    else
    {
      // start not set on first FU
      LOG(WARNING) << "TODO: start bit not set on first FU";
      return rtpPackets.end();
      // TODO: could try to continue parsing?
      //return itNextFu;
    }
  }
  else
  {
    LOG(WARNING) << "Failed to read FU A header";
    return rtpPackets.end();
    // TODO: could try to continue parsing?
    //return itNextFu;
  }
}


std::list<RtpPacket>::const_iterator Rfc6184Packetiser::extractNextMediaSample(const RtpPacketGroup& rtpPacketGroup, const std::list<RtpPacket>& rtpPackets, std::list<RtpPacket>::const_iterator itStart, std::vector<MediaSample>& vMediaSamples)
{
#ifdef DEBUG_RFC6184_PACKETIZATION
  VLOG(5) << "extractNextMediaSample: " << rtpPackets.size();
#endif
  std::list<RtpPacket>::const_iterator it;
  for (it = itStart; it != rtpPackets.end(); ++it)
  {
    // once a sample if found, return iterator to next element
    // check nal unit type
    const RtpPacket& rtpPacket = *it;

#ifdef DEBUG_RFC6184_PACKETIZATION
    VLOG(5) << "extractNextMediaSample: SN " << rtpPacket.getExtendedSequenceNumber();
#endif

    Buffer rawData = rtpPacket.getPayload();
    IBitStream in(rawData);
    uint32_t forbidden = 0, nri = 0, type = 0;
    if (( in.read(forbidden, 1)) &&
        ( in.read(nri, 2)) &&
        ( in.read(type, 5 )))
    {
#ifdef DEBUG_RFC6184_PACKETIZATION
      VLOG(5) << "Forbidden: " << forbidden << " NRI: " << nri << " type: " << type << "(" << rfc6184::toString(static_cast<rfc6184::NalUnitType>(type)) << ")" ;
#endif
      if (type <= NUT_RESERVED_23)
      {
        MediaSample mediaSample;
        mediaSample.setData(rawData);
        mediaSample.setRtpTime(rtpPacket.getRtpTimestamp());
        mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
        vMediaSamples.push_back(mediaSample);
        return ++it;
      }
      else
      {
        // according to type, extract media data
        switch (type)
        {
          case NUT_STAP_A:
          {
            // Need to parse single time aggregation packets and split into multiple NAL units
            handleStapA(in, vMediaSamples, rtpPacketGroup);
            return ++it;
            break;
          }
          case NUT_STAP_B:
          {
            // Need to parse single time aggregation packets and split into multiple NAL units
            handleStapB(in, vMediaSamples, rtpPacketGroup);
            return ++it;
            break;
          }
          case NUT_FU_A:
          {
            return handleFuA(in, rtpPackets, ++it, vMediaSamples, rtpPacket.getExtendedSequenceNumber(), forbidden, nri, rtpPacketGroup);
            break;
          }
          case NUT_FU_B:
          {
            return handleFuB(in, rtpPackets, ++it, vMediaSamples, rtpPacket.getExtendedSequenceNumber(), forbidden, nri, rtpPacketGroup);
             break;
          }
          default:
          {
            LOG(WARNING) << "The NAL unit type " << type << " is not allowed in non-interleaved mode";
            return rtpPackets.end();
            break;
          }
        }
      }
    }
    else
    {
      LOG(FATAL) << "Failed to read FU A header";
      return rtpPackets.end();
    }
  }

  return it;
}

std::vector<MediaSample> Rfc6184Packetiser::depacketizeNonInterleaved(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();
  std::list<RtpPacket>::const_iterator it = rtpPackets.begin();
  while (it != rtpPackets.end())
  {
    it = extractNextMediaSample(rtpPacketGroup, rtpPackets, it, vSamples);
  }
  return vSamples;
}

std::vector<MediaSample> Rfc6184Packetiser::depacketizeInterleaved(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();
  std::list<RtpPacket>::const_iterator it = rtpPackets.begin();
  while (it != rtpPackets.end())
  {
    it = extractNextMediaSample(rtpPacketGroup, rtpPackets, it, vSamples);
  }
  return vSamples;
}

std::vector<MediaSample> Rfc6184Packetiser::depacketize(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();
  switch(m_eMode)
  {
    case SINGLE_NAL_UNIT_MODE:
    {
      vSamples = depacketizeSingleNalUnit(rtpPacketGroup);
      break;
    }
    case NON_INTERLEAVED_MODE:
    {
      vSamples = depacketizeNonInterleaved(rtpPacketGroup);
      break;
    }
    case INTERLEAVED_MODE:
    {
      vSamples = depacketizeInterleaved(rtpPacketGroup);
      break;
    }
    default:
    {
      LOG(FATAL) << "The interleaved packetization mode has not been implemented.";
      return std::vector<MediaSample>();
      break;
    }
  }

  // setting the marker here adds no value
#if 0

  // set marker bit if set on last sample.
  if (!rtpPackets.empty() && !vSamples.empty())
  {
    vSamples[vSamples.size() - 1].setMarker(rtpPackets.back().getHeader().isMarkerSet());
  }
#else
  // we will rather use the marker bit to indicate that RTCP sync has occurred
  for (MediaSample& mediaSample : vSamples)
  {
    mediaSample.setMarker(rtpPacketGroup.isRtcpSynchronised());
  }
#endif
  return vSamples;
}

RtpPacket Rfc6184Packetiser::packetiseSingleNalUnit(const MediaSample& mediaSample)
{
  // there is only one NAL per RTP packet
  if (mediaSample.getDataBuffer().getSize() > 1460)
    LOG(WARNING) << "Sample size may be greater than network MTU";
  // Note: the RTP header will be populated in the RTP session
  RtpPacket packet;
  // NOTE: we have to copy the payload since it belongs to the media sample
  // and we don't have control of what will happen to the memory
  Buffer mediaData(new uint8_t[mediaSample.getPayloadSize()], mediaSample.getPayloadSize());
  memcpy((char*)mediaData.data(), (char*)mediaSample.getDataBuffer().data(), mediaSample.getPayloadSize());
  packet.setPayload(mediaData);
  packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());

  return packet;
}

std::vector<RtpPacket> Rfc6184Packetiser::packetiseFuA(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  assert (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload);

  VLOG(15) << "NAL unit too big " << mediaSample.getDataBuffer().getSize() << "- fragmenting";
  bool bStart = true;

  Buffer buffer = mediaSample.getDataBuffer();
  IBitStream in(buffer);
  // forbidden
  in.skipBits(1);
  // nri
  uint32_t uiNri = 0, uiNut = 0;
  in.read(uiNri, 2);
  //  NUT
  in.read(uiNut, 5);

  // the NAL unit header forms part of the Fu indicator and FU header: subtract it from the total
  uint32_t uiBytesToPacketize = in.getBytesRemaining();
  while (uiBytesToPacketize > 0)
  {
    RtpPacket packet;
    // packetize FU

    // Fu indicator
    OBitStream out1;
    out1.write(0, 1);
    out1.write(uiNri, 2);
    out1.write(static_cast<uint32_t>(NUT_FU_A), 5);

    uint32_t uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 2 /* FU headers */;
    // FU header
    if (bStart)
    {
      /// S | E | R = 1 | 0 | 0
      out1.write(1, 1);
      out1.write(0, 1);
      out1.write(0, 1);

      bStart = false;
    }
    else
    {
      // check if this is the last packet
      if (uiBytesToPacketize <= uiMaxBytesPerPayload)
      {
        /// S | E | R = 0 | 1 | 0
        out1.write(0, 1);
        out1.write(1, 1);
        out1.write(0, 1);
        // TODO: is this correct?
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
      }
      else
      {
        /// S | E | R = 0 | 0 | 0
        // out1.write(0, 3);
        out1.write(0, 1);
        out1.write(0, 1);
        out1.write(0, 1);
      }
    }
    out1.write( uiNut, 5);

    // Payload
    uint32_t uiBytesToWrite = std::min(in.getBytesRemaining(), uiMaxBytesPerPayload);
    bool bRes = out1.write(in, uiBytesToWrite);
    assert(bRes);
    uiBytesToPacketize -= uiBytesToWrite;

    VLOG(15) << "FU size: " << uiBytesToWrite;
    // create RTP packet
    packet.setPayload(out1.data());
    vRtpPackets.push_back(packet);
  }

  return vRtpPackets;
}

std::vector<RtpPacket> Rfc6184Packetiser::packetiseFuB(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  assert (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload);

  VLOG(15) << "NAL unit too big " << mediaSample.getDataBuffer().getSize() << "- fragmenting";
  bool bStart = true;

  Buffer buffer = mediaSample.getDataBuffer();
  IBitStream in(buffer);
  // forbidden
  in.skipBits(1);
  // nri
  uint32_t uiNri = 0, uiNut = 0;
  in.read(uiNri, 2);
  //  NUT
  in.read(uiNut, 5);

  // the NAL unit header forms part of the Fu indicator and FU header: subtract it from the total
  uint32_t uiBytesToPacketize = in.getBytesRemaining();
  while (uiBytesToPacketize > 0)
  {
    RtpPacket packet;
    // packetize FU

    // Fu indicator
    OBitStream out1;
    out1.write(0, 1);
    out1.write(uiNri, 2);


    uint32_t uiMaxBytesPerPayload;
    // FU header
    if (bStart)
    {
      out1.write(static_cast<uint32_t>(NUT_FU_B), 5);
      uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 4 /* FU headers */;
      /// S | E | R = 1 | 0 | 0
      out1.write(1, 1);
      out1.write(0, 1);
      out1.write(0, 1);
      out1.write( uiNut, 5);
      out1.write(mediaSample.getDecodingOrderNumber(),16);

      bStart = false;
    }
    else
    {
      out1.write(static_cast<uint32_t>(NUT_FU_A), 5);
      uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 2 /* FU headers */;
      // check if this is the last packet
      if (uiBytesToPacketize <= uiMaxBytesPerPayload)
      {
        /// S | E | R = 0 | 1 | 0
        out1.write(0, 1);
        out1.write(1, 1);
        out1.write(0, 1);
        out1.write( uiNut, 5);
        // TODO: is this correct?
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
      }
      else
      {
        /// S | E | R = 0 | 0 | 0
        // out1.write(0, 3);
        out1.write(0, 1);
        out1.write(0, 1);
        out1.write(0, 1);
        out1.write( uiNut, 5);
      }
    }

    // Payload
    uint32_t uiBytesToWrite = std::min(in.getBytesRemaining(), uiMaxBytesPerPayload);
    bool bRes = out1.write(in, uiBytesToWrite);
    assert(bRes);
    uiBytesToPacketize -= uiBytesToWrite;

    VLOG(15) << "FU size: " << uiBytesToWrite;
    // create RTP packet
    packet.setPayload(out1.data());
    vRtpPackets.push_back(packet);
  }

  return vRtpPackets;
}



std::vector<RtpPacket> Rfc6184Packetiser::packetiseNonInterleaved(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload)
  {
    vRtpPackets = packetiseFuA(mediaSample);
  }
  else
  {
    // Note: the RTP header will be populated in the RTP session
    vRtpPackets.push_back(packetiseSingleNalUnit(mediaSample));
  }
  return vRtpPackets;
}

std::vector<RtpPacket> Rfc6184Packetiser::packetiseInterleaved(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload-4)//In STAP-B we need additionally 2 bytes for DON and 2 for NALU length
  {
    vRtpPackets = packetiseFuB(mediaSample);
  }
  else
  {
    // Note: the RTP header will be populated in the RTP session
    RtpPacket packet;
    // STAP-A indicator
    OBitStream out1(mediaSample.getPayloadSize() + 4);
    IBitStream in(mediaSample.getDataBuffer());
    // forbidden
    in.skipBits(1);
    // nri
    uint32_t uiNri = 0, uiNut = 0;
    in.read(uiNri, 2);
    //  NUT
    in.read(uiNut, 5);

    out1.write(0, 1);
    out1.write(uiNri, 2);
    out1.write(static_cast<uint32_t>(NUT_STAP_B), 5);
    out1.write(mediaSample.getDecodingOrderNumber(), 16);
    out1.write(mediaSample.getPayloadSize(), 16);
    out1.write(0, 1);
    out1.write(uiNri, 2);
    out1.write(uiNut, 5);
    out1.write(in,mediaSample.getPayloadSize()-1);
    packet.setPayload(out1.data());
    packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
    vRtpPackets.push_back(packet);
  }
  return vRtpPackets;
}


int32_t Rfc6184Packetiser::getNumberOfSamplesToAggregate(uint32_t uiBytesAvailableForPayload,
                                                         const std::vector<MediaSample>& mediaSamples,
                                                         uint32_t uiStartIndex, uint32_t& uiStapSize,
                                                         bool& bFBit, uint32_t& uiNri)
{
  // optional STAP disabling
  if (m_bDisableStap)
  {
    return 1;
  }

  int iNalUnitsToAggregate = 0;
  uiStapSize = 0;
  bFBit = false;
  uiNri = 0;

  // subtract first byte of STAP-A
  ++uiStapSize;
  --uiBytesAvailableForPayload;

  for (size_t i = uiStartIndex; uiBytesAvailableForPayload > 0 && i < mediaSamples.size(); ++i)
  {
    // add 2 bytes for the length of the NAL unit
    uint32_t uiSizeRequiredForThisNal = mediaSamples[i].getPayloadSize() + 2;
    if (uiSizeRequiredForThisNal > uiBytesAvailableForPayload)
      break;

    uint8_t uiNalHeader = mediaSamples[i].getDataBuffer().data()[0];
    // F-Bit
    if ((uiNalHeader & 0x80) != 0) bFBit = true;
    // NRI
    uint32_t uiCurrentNri = (uiNalHeader & 0x60) >> 5;
    if (uiCurrentNri > uiNri) uiNri = uiCurrentNri;

    ++iNalUnitsToAggregate;
    uiStapSize += uiSizeRequiredForThisNal;
    uiBytesAvailableForPayload -= uiSizeRequiredForThisNal;
  }

  return iNalUnitsToAggregate;
}

std::vector<RtpPacket> Rfc6184Packetiser::packetiseNonInterleaved(const std::vector<MediaSample>& mediaSamples)
{
  std::vector<RtpPacket> vRtpPackets;
  uint32_t iCurrentPacket = 0;
  uint32_t iRtpPacketIndex = 0;
  while (iCurrentPacket < mediaSamples.size())
  {
    const MediaSample& mediaSample = mediaSamples[iCurrentPacket];
    if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload)
    {
      // FU-A
      std::vector<RtpPacket> vFuRtpPackets = packetiseFuA(mediaSample);
      vRtpPackets.insert(vRtpPackets.end(), vFuRtpPackets.begin(), vFuRtpPackets.end());
      // update packetisation info
      for (size_t i = 0; i < vFuRtpPackets.size(); ++i)
      {
        m_vLastPacketisationInfo[iCurrentPacket].push_back(iRtpPacketIndex);
        ++iRtpPacketIndex;
      }

      ++iCurrentPacket;
    }
    else
    {
      // see if we can do STAPs
      bool bFBit = false;
      uint32_t uiNri = 0;
      uint32_t uiStapSize = 0;
      int32_t iCount = getNumberOfSamplesToAggregate(m_uiBytesAvailableForPayload, mediaSamples, iCurrentPacket, uiStapSize, bFBit, uiNri);
      // getNumberOfSamplesToAggregate can return 0 or one depending on the payload and header size
      if (iCount < 2)
      {
        // Single Nal Unit
        vRtpPackets.push_back(packetiseSingleNalUnit(mediaSample));
        // update packetisation info
        m_vLastPacketisationInfo[iCurrentPacket].push_back(iRtpPacketIndex);
        ++iRtpPacketIndex;

        ++iCurrentPacket;
      }
      else
      {
        RtpPacket packet;
        // STAP-A indicator
        OBitStream out1(uiStapSize);
        out1.write(bFBit, 1);
        out1.write(uiNri, 2);
        out1.write(static_cast<uint32_t>(NUT_STAP_A), 5);
        // we can aggregate some packets
        for (size_t i = iCurrentPacket; i < iCurrentPacket + iCount; ++i)
        {
          const MediaSample& mediaSample = mediaSamples[i];
          out1.write(mediaSample.getPayloadSize(), 16);
          VLOG(15) << "Copying media sample data into STAP-A: " << mediaSample.getPayloadSize() << " bytes";
          // copy rest of stream
          IBitStream in(mediaSample.getDataBuffer());
          out1.write(in);

          // update packetisation info
          m_vLastPacketisationInfo[i].push_back(iRtpPacketIndex);
        }
        packet.setPayload(out1.data());
        // the marker bit gets set according to the last sample in the STAP
        packet.getHeader().setMarkerBit(mediaSamples[iCurrentPacket + iCount - 1].isMarkerSet());
        vRtpPackets.push_back(packet);

        // update packetisation info
        ++iRtpPacketIndex;

        iCurrentPacket += iCount;
      }
    }
  }
  return vRtpPackets;
}

std::vector<RtpPacket> Rfc6184Packetiser::packetiseInterleaved(const std::vector<MediaSample>& mediaSamples)
{
  std::vector<RtpPacket> vRtpPackets;
  uint32_t iCurrentPacket = 0;
  uint32_t iRtpPacketIndex = 0;
  while (iCurrentPacket < mediaSamples.size())
  {
    const MediaSample& mediaSample = mediaSamples[iCurrentPacket];
    if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload-4)
    {
      // FU-A
      std::vector<RtpPacket> vFuRtpPackets = packetiseFuB(mediaSample);
      vRtpPackets.insert(vRtpPackets.end(), vFuRtpPackets.begin(), vFuRtpPackets.end());
      // update packetisation info
      for (size_t i = 0; i < vFuRtpPackets.size(); ++i)
      {
        m_vLastPacketisationInfo[iCurrentPacket].push_back(iRtpPacketIndex);
        ++iRtpPacketIndex;
      }
      ++iCurrentPacket;
    }
    else
    {
      // see if we can do STAPs
      bool bFBit = false;
      uint32_t uiNri = 0;
      uint32_t uiStapSize = 0;
      int32_t iCount = getNumberOfSamplesToAggregate(m_uiBytesAvailableForPayload-2, mediaSamples, iCurrentPacket, uiStapSize, bFBit, uiNri);
      if (iCount < 2)
      {
        // Single Nal Unit
        RtpPacket packet;
        // STAP-A indicator
        OBitStream out1(uiStapSize + 2);

        IBitStream in(mediaSample.getDataBuffer());
        // forbidden
        in.skipBits(1);

        out1.write(0, 1);
        out1.write(uiNri, 2);
        out1.write(static_cast<uint32_t>(NUT_STAP_B), 5);
        out1.write(mediaSample.getDecodingOrderNumber(), 16);
        out1.write(mediaSample.getPayloadSize(), 16);
        // nri
        uint32_t uiNut = 0;
        in.read(uiNri, 2);
        //  NUT
        in.read(uiNut, 5);

        out1.write(0, 1);
        out1.write(uiNri, 2);
        out1.write(uiNut, 5);
        out1.write(in,mediaSample.getPayloadSize()-1);
        packet.setPayload(out1.data());
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
        vRtpPackets.push_back(packet);
        // update packetisation info
        m_vLastPacketisationInfo[iCurrentPacket].push_back(iRtpPacketIndex);
        ++iRtpPacketIndex;
        ++iCurrentPacket;
      }
      else
      {
        RtpPacket packet;
        // STAP-A indicator
        OBitStream out1(uiStapSize);
        out1.write(bFBit, 1);
        out1.write(uiNri, 2);
        out1.write(static_cast<uint32_t>(NUT_STAP_B), 5);
        const MediaSample& mediaSample1=mediaSamples[iCurrentPacket];
        out1.write(mediaSample1.getDecodingOrderNumber(),16);
        // we can aggregate some packets
        for (size_t i = iCurrentPacket; i < (uint32_t)iCount; ++i)
        {
          const MediaSample& mediaSample = mediaSamples[i];
          out1.write(mediaSample.getPayloadSize(), 16);
          VLOG(15) << "Copying media sample data into STAP-A: " << mediaSample.getPayloadSize() << " bytes";
          // copy rest of stream
          IBitStream in(mediaSample.getDataBuffer());
          out1.write(in);
          
          // update packetisation info
          m_vLastPacketisationInfo[i].push_back(iRtpPacketIndex);
        }
        packet.setPayload(out1.data());
        // the marker bit gets set according to the last sample in the STAP
        packet.getHeader().setMarkerBit(mediaSamples[iCurrentPacket + iCount - 1].isMarkerSet());
        vRtpPackets.push_back(packet);

        // update packetisation info
        ++iRtpPacketIndex;

        iCurrentPacket += iCount;
      }
    }
  }
  return vRtpPackets;
}

std::vector<MediaSample> Rfc6184Packetiser::depacketizeSingleNalUnit(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();
  // there is only one NAL per RTP packet
  // SPS and PPS could have the same timestamp and thus be grouped together
  for(const RtpPacket& rtpPacket: rtpPackets)
  {
    // check nal unit type
    const Buffer rawData = rtpPacket.getPayload();
    IBitStream in(rawData);
    uint32_t forbidden = 0, nri = 0, type = 0;
    if (( in.read(forbidden, 1)) &&
        ( in.read(nri, 2)) &&
        ( in.read(type, 5 )))
    {
      VLOG(10) << "Forbidden: " << forbidden << " NRI: " << nri << " type: " << type;
      if (type > NUT_RESERVED_23)
      {
        LOG(WARNING) << "The NAL unit type " << type << " is not allowed in single nal unit mode";
      }
      else
      {
        MediaSample mediaSample;
        mediaSample.setData(rawData);
        mediaSample.setPresentationTime(rtpPacketGroup.getPresentationTime());
        vSamples.push_back(mediaSample);
      }
    }
    else
    {
      LOG(FATAL) << "Failed to read NAL unit data";
    }
  }

  return vSamples;
}

void Rfc6184Packetiser::printPacketListInfo(const std::list<RtpPacket>& rtpPackets)
{
  std::ostringstream ostr;
  std::list<RtpPacket>::const_iterator it = rtpPackets.begin();
  while (it != rtpPackets.end())
  {
    ostr << it->getExtendedSequenceNumber() << " ";
    ++it;
  }
  VLOG(2) << "Packet list info: " << ostr.str();
}

} // rfc6184
} // rtp_plus_plus
