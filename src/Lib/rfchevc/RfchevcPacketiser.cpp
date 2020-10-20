#include "CorePch.h"
#include <rtp++/rfchevc/RfchevcPacketiser.h>
#include <boost/foreach.hpp>

#include <cpputil/IBitStream.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>

#define DEFAULT_MTU 1500
#define DEFAULT_BUFFER_SIZE_BYTES 10000
#define IP_UDP_RTP_HEADER_SIZE 40
// #define DEBUG_RFC6184_PACKETIZATION

namespace rtp_plus_plus
{
namespace rfchevc
{

using namespace media::h265;
using media::MediaSample;

RfchevcPacketiser::ptr RfchevcPacketiser::create()
{
  return std::unique_ptr<RfchevcPacketiser>(new RfchevcPacketiser());
}

RfchevcPacketiser::ptr RfchevcPacketiser::create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
{
  return std::unique_ptr<RfchevcPacketiser>(new RfchevcPacketiser(uiMtu, uiSessionBandwidthKbps));
}

RfchevcPacketiser::RfchevcPacketiser()
  :m_eMode(NON_INTERLEAVED_MODE), // default mode
  m_uiMtu(DEFAULT_MTU),
  m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
  m_uiSessionBandwidthKbps(0),
  m_uiEstimatedFrameSize(DEFAULT_BUFFER_SIZE_BYTES),
  m_outputStream(m_uiEstimatedFrameSize, false)
{

}

RfchevcPacketiser::RfchevcPacketiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
  :m_eMode(NON_INTERLEAVED_MODE), // default mode
  m_uiMtu(uiMtu),
  m_uiBytesAvailableForPayload(m_uiMtu - IP_UDP_RTP_HEADER_SIZE), // NB: this header does not account for the RTP extension header
  m_uiSessionBandwidthKbps(uiSessionBandwidthKbps),
  m_uiEstimatedFrameSize(m_uiSessionBandwidthKbps > 0 ? m_uiSessionBandwidthKbps*10 : DEFAULT_BUFFER_SIZE_BYTES),
  m_outputStream(m_uiEstimatedFrameSize, false)
{
  // the estimated frame size is set to m_uiSessionBandwidthKbps*10 = m_uiSessionBandwidthKbps*1000/(8*12.5) assuming
  // that the framerate is 12.5 frames per second. Note: this will be resized automatically by the reading code
  // but if it proves to be inefficient we can increase the frame size
  assert(uiMtu != 0);

}


std::vector<RtpPacket> RfchevcPacketiser::packetise(const MediaSample& mediaSample)
{
#ifdef DEBUG_NAL_UNIT_TYPES
  const uint8_t* pData = mediaSample.getDataBuffer().data();
  uint8_t uiNut = ( pData[0] & 0x7E ) >> 1;
  rfchevc::NalUnitType eNextType = static_cast<rfchevc::NalUnitType>(uiNut);
  DLOG(INFO) << "Next NAL type: " << rfchevc::toString(eNextType);
#endif

  switch(m_eMode)
  {
    case INTERLEAVED_MODE:
    {
      return packetiseInterleaved(mediaSample);
      break;
    }
    default:
    case NON_INTERLEAVED_MODE:
    {
      return packetiseNonInterleaved(mediaSample);
      break;
    }
  }
}

std::vector<RtpPacket> RfchevcPacketiser::packetise(const std::vector<MediaSample>& mediaSamples)
{
  m_vLastPacketisationInfo.clear();
  for (size_t i = 0; i < mediaSamples.size(); ++i )
  {
    m_vLastPacketisationInfo.push_back(std::vector<uint32_t>());
  }

  switch(m_eMode)
  {
    case INTERLEAVED_MODE:
    {
      return packetiseInterleaved(mediaSamples);
      break;
    }
    default:
    case NON_INTERLEAVED_MODE:
    {
      return packetiseNonInterleaved(mediaSamples);
      break;
    }
  }
}

bool RfchevcPacketiser::handleAPA(IBitStream& in, std::vector<MediaSample>& vSamples, uint32_t RtpTime)
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
        uint32_t stap_forbidden = 0, stap_layerid = 0, stap_tempid = 0, stap_type = 0;
        IBitStream inTemp(dataBuffer);
        if (( inTemp.read(stap_forbidden, 1)) &&
            ( inTemp.read(stap_type, 6 )) &&
            ( inTemp.read(stap_layerid, 6)) &&
            ( inTemp.read(stap_tempid, 6)))
        {
          VLOG(1) << "STAP A: Parsed NAL unit type: " << stap_type << " " << toString((NalUnitType)stap_type);
        }
#endif
        MediaSample mediaSample;
        mediaSample.setData(dataBuffer);
        mediaSample.setRtpTime(RtpTime);
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

bool RfchevcPacketiser::handleAPB(IBitStream& in, std::vector<MediaSample>& vSamples, uint32_t RtpTime)
{
  bool bError = false;
  uint32_t uiSize = 0;
  uint16_t uiDON = 0;
  uint8_t uiDONd = 0;
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
        mediaSample.setDecodingOrderNumber(uiDON);
        mediaSample.setRtpTime(RtpTime);
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
  in.read(uiDONd, 8);
  uiDON+=uiDONd+1;
  }
  return !bError;
}


std::list<RtpPacket>::const_iterator RfchevcPacketiser::handleFuA(IBitStream& in,
                                                              const std::list<RtpPacket>& rtpPackets,
                                                              std::list<RtpPacket>::const_iterator itNextFu,
                                                              std::vector<MediaSample>& vSamples,
                                                              uint32_t uiStartSN,
                                                              uint32_t forbidden,
                                                              uint32_t layerId,
                                                              uint32_t tempId,
                                                              uint32_t RtpTime)
{
  uint32_t fu_start = 0, fu_end = 0, fu_type = 0;
  if (( in.read(fu_start,    1) ) &&
      ( in.read(fu_end,      1) ) &&
      ( in.read(fu_type,     6) )
     )
  {
#ifdef DEBUG_RFC6184_PACKETIZATION
    VLOG(2) << "First FU: SN: " << uiStartSN << " start: " << fu_start << " end: " << fu_end << " res: " << fu_reserved << " Type: " << rfc6184::toString((NalUnitType)fu_type);
#endif

    // make sure start is set for first FU
    if ( fu_start != 0 )
    {
      assert( fu_end == 0);

      m_outputStream.reset();
      // write NAL unit header based on FU indicator and FU header
      m_outputStream.write(forbidden, 1);
      m_outputStream.write(fu_type, 6);
      m_outputStream.write(layerId, 6);
      m_outputStream.write(tempId, 3);
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

          uint32_t type = 0;
          uint32_t nextfu_start = 0, nextfu_end = 0, nextfu_type = 0;
          // read FU indicator
          // read FU header
          if (( inNextFu.read(forbidden, 1)) &&
              ( inNextFu.read(type, 6 )) &&
              ( inNextFu.read(layerId, 6)) &&
              ( inNextFu.read(tempId, 3)) &&
              ( inNextFu.read(nextfu_start,    1) ) &&
              ( inNextFu.read(nextfu_end,      1) ) &&
              ( inNextFu.read(nextfu_type,     6) )
             )
          {
#ifdef DEBUG_RFC6184_PACKETIZATION
            VLOG(2) << "Next FU: SN: " << nextFU.getExtendedSequenceNumber() << " start: " << nextfu_start << " end: " << nextfu_end << " res: " << nextfu_reserved << " Type: " << rfc6184::toString((NalUnitType)nextfu_type);
#endif
            if (type != NUT_FU)
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
                mediaSample.setRtpTime(RtpTime);
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

std::list<RtpPacket>::const_iterator RfchevcPacketiser::handleFuB(IBitStream& in,
                                                              const std::list<RtpPacket>& rtpPackets,
                                                              std::list<RtpPacket>::const_iterator itNextFu,
                                                              std::vector<MediaSample>& vSamples,
                                                              uint32_t uiStartSN,
                                                              uint32_t forbidden,
                                                              uint32_t layerId,
                                                              uint32_t tempId,
                                                              uint32_t RtpTime)
{
  uint32_t fu_start = 0, fu_end = 0, fu_type = 0;
  uint16_t uiDON = 0 ;
  if (( in.read(fu_start,    1) ) &&
      ( in.read(fu_end,      1) ) &&
      ( in.read(fu_type,     6) ) &&
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

      m_outputStream.reset();
      // write NAL unit header based on FU indicator and FU header
      m_outputStream.write(forbidden, 1);
      m_outputStream.write(fu_type, 6);
      m_outputStream.write(layerId, 6);
      m_outputStream.write(tempId, 3);

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

          uint32_t  type = 0;
          uint32_t nextfu_start = 0, nextfu_end = 0, nextfu_type = 0;
          // read FU indicator
          // read FU header
          if (( inNextFu.read(forbidden, 1)) &&
              ( inNextFu.read(type, 6 )) &&
              ( inNextFu.read(layerId, 6)) &&
              ( inNextFu.read(tempId, 3)) &&
              ( inNextFu.read(nextfu_start,    1) ) &&
              ( inNextFu.read(nextfu_end,      1) ) &&
              ( inNextFu.read(nextfu_type,     6) )
             )
          {
#ifdef DEBUG_RFC6184_PACKETIZATION
            VLOG(2) << "Next FU: SN: " << nextFU.getExtendedSequenceNumber() << " start: " << nextfu_start << " end: " << nextfu_end << " res: " << nextfu_reserved << " Type: " << rfc6184::toString((NalUnitType)nextfu_type);
#endif
            if (type != NUT_FU)
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
                mediaSample.setRtpTime(RtpTime);
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


std::list<RtpPacket>::const_iterator RfchevcPacketiser::extractNextMediaSample(const std::list<RtpPacket>& rtpPackets, std::list<RtpPacket>::const_iterator itStart, std::vector<MediaSample>& vMediaSamples)
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
    uint32_t forbidden = 0, layerId = 0, tempId = 0, type = 0;
    if (( in.read(forbidden, 1)) &&
        ( in.read(type, 6 )) &&
        ( in.read(layerId, 6)) &&
        ( in.read(tempId, 3)))
    {
//#ifdef DEBUG_RFC6184_PACKETIZATION
      VLOG(5) << "Forbidden: " << forbidden << " type: " << type << "(" << rfchevc::toString(static_cast<rfchevc::NalUnitType>(type)) << ") "<< " layer ID " << layerId << " tempId "<< tempId   ;
//#endif
      if (type <= NUT_RSV_47 )
      {
        MediaSample mediaSample;
        mediaSample.setData(rawData);
        mediaSample.setRtpTime(rtpPacket.getRtpTimestamp());
        vMediaSamples.push_back(mediaSample);
        return ++it;
      }
      else
      {
        // according to type, extract media data
        switch (type)
        {
          case NUT_AP:
          {
            // Need to parse single time aggregation packets and split into multiple NAL units
            if(true)
                handleAPA(in, vMediaSamples, rtpPacket.getRtpTimestamp());
            else
                handleAPB(in, vMediaSamples, rtpPacket.getRtpTimestamp());
            return ++it;
            break;
          }
          case NUT_FU:
          {
            if(true)
                return handleFuA(in, rtpPackets, ++it, vMediaSamples, rtpPacket.getExtendedSequenceNumber(), forbidden, layerId, tempId, rtpPacket.getRtpTimestamp());
            else
                return handleFuB(in, rtpPackets, ++it, vMediaSamples, rtpPacket.getExtendedSequenceNumber(), forbidden, layerId, tempId, rtpPacket.getRtpTimestamp());
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

std::vector<MediaSample> RfchevcPacketiser::depacketizeNonInterleaved(const std::list<RtpPacket>& rtpPackets)
{
  std::vector<MediaSample> vSamples;
  std::list<RtpPacket>::const_iterator it = rtpPackets.begin();
  while (it != rtpPackets.end())
  {
    it = extractNextMediaSample(rtpPackets, it, vSamples);
  }
  return vSamples;
}

std::vector<MediaSample> RfchevcPacketiser::depacketizeInterleaved(const std::list<RtpPacket>& rtpPackets)
{
  std::vector<MediaSample> vSamples;
  std::list<RtpPacket>::const_iterator it = rtpPackets.begin();
  while (it != rtpPackets.end())
  {
    it = extractNextMediaSample(rtpPackets, it, vSamples);
  }
  return vSamples;
}


std::vector<MediaSample> RfchevcPacketiser::depacketize(const RtpPacketGroup& rtpPacketGroup)
{
  std::vector<MediaSample> vSamples;
  const std::list<RtpPacket>& rtpPackets = rtpPacketGroup.getRtpPackets();
  switch (m_eMode)
  {
    case NON_INTERLEAVED_MODE:
    {
      vSamples = depacketizeNonInterleaved(rtpPackets);
      break;
    }
    case INTERLEAVED_MODE:
    {
      vSamples = depacketizeInterleaved(rtpPackets);
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

RtpPacket RfchevcPacketiser::packetiseSingleNalUnit(const MediaSample& mediaSample)
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

std::vector<RtpPacket> RfchevcPacketiser::packetiseFuA(const MediaSample& mediaSample)
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
  // NUT
  uint32_t uiLayerId = 0, uiTempId = 0, uiNut = 0;
  in.read(uiNut, 6);
  //  LayerId
  in.read(uiLayerId, 6);
  //  TempId
  in.read(uiTempId, 3);

  // the NAL unit header forms part of the Fu indicator and FU header: subtract it from the total
  uint32_t uiBytesToPacketize = in.getBytesRemaining();
  while (uiBytesToPacketize > 0)
  {
    RtpPacket packet;
    // packetize FU

    // Fu indicator
    OBitStream out1;
    out1.write(0, 1);
    out1.write(static_cast<uint32_t>(NUT_FU), 6);
    out1.write(uiLayerId, 6);
    out1.write(uiTempId, 3);

    uint32_t uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 3 /* FU headers */;
    // FU header
    if (bStart)
    {
      /// S | E = 1 | 0
      out1.write(1, 1);
      out1.write(0, 1);

      bStart = false;
    }
    else
    {
      // check if this is the last packet
      if (uiBytesToPacketize <= uiMaxBytesPerPayload)
      {
        /// S | E = 0 | 1
        out1.write(0, 1);
        out1.write(1, 1);
        // TODO: is this correct?
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
      }
      else
      {
        /// S | E = 0 | 0
        // out1.write(0, 3);
        out1.write(0, 1);
        out1.write(0, 1);
      }
    }
    out1.write( uiNut, 6);

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

std::vector<RtpPacket> RfchevcPacketiser::packetiseFuB(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  assert (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload - 5);

  VLOG(15) << "NAL unit too big " << mediaSample.getDataBuffer().getSize() << "- fragmenting";
  bool bStart = true;

  Buffer buffer = mediaSample.getDataBuffer();
  IBitStream in(buffer);
  // forbidden
  in.skipBits(1);
  // NUT
  uint32_t uiLayerId = 0, uiTempId = 0, uiNut = 0;
  in.read(uiNut, 6);
  //  LayerId
  in.read(uiLayerId, 6);
  //  TempId
  in.read(uiTempId, 3);

  // the NAL unit header forms part of the Fu indicator and FU header: subtract it from the total
  uint32_t uiBytesToPacketize = in.getBytesRemaining();
  while (uiBytesToPacketize > 0)
  {
    RtpPacket packet;
    // packetize FU

    // Fu indicator
    OBitStream out1;
    out1.write(0, 1);
    out1.write(static_cast<uint32_t>(NUT_FU), 6);
    out1.write(uiLayerId, 6);
    out1.write(uiTempId, 3);


    uint32_t uiMaxBytesPerPayload;
    // FU header
    if (bStart)
    {
      uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 5 /* FU headers */;
      /// S | E  = 1 | 0
      out1.write(1, 1);
      out1.write(0, 1);
      out1.write( uiNut, 6);
      out1.write(mediaSample.getDecodingOrderNumber(),16);

      bStart = false;
    }
    else
    {
      uiMaxBytesPerPayload = m_uiBytesAvailableForPayload - 3 /* FU headers */;
      // check if this is the last packet
      if (uiBytesToPacketize <= uiMaxBytesPerPayload)
      {
        /// S | E  = 0 | 1
        out1.write(0, 1);
        out1.write(1, 1);
        out1.write( uiNut, 6);
        // TODO: is this correct?
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
      }
      else
      {
        /// S | E = 0 | 0
        // out1.write(0, 3);
        out1.write(0, 1);
        out1.write(0, 1);
        out1.write( uiNut, 6);
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


std::vector<RtpPacket> RfchevcPacketiser::packetiseNonInterleaved(const MediaSample& mediaSample)
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

std::vector<RtpPacket> RfchevcPacketiser::packetiseInterleaved(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> vRtpPackets;
  // for now don't do STAP
  if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload-5)//In STAP-B we need additionally 2 bytes for DON and 2 for NALU length
  {
    vRtpPackets = packetiseFuB(mediaSample);
  }
  else
  {
    // Note: the RTP header will be populated in the RTP session
    RtpPacket packet;
    // STAP-A indicator
    OBitStream out1(mediaSample.getPayloadSize() + 5);
    IBitStream in(mediaSample.getDataBuffer());
    // forbidden
    in.skipBits(1);
    // NUT
    uint32_t uiLayerId = 0, uiTempId = 0, uiNut = 0;
    in.read(uiNut, 6);
    //  LayerId
    in.read(uiLayerId, 6);
    //  TempId
    in.read(uiTempId, 3);

    out1.write(0, 1);
    out1.write(static_cast<uint32_t>(NUT_AP), 6);
    out1.write(uiLayerId, 6);
    out1.write(uiTempId, 3);
    out1.write(mediaSample.getDecodingOrderNumber(), 16);
    out1.write(mediaSample.getPayloadSize(), 16);
    out1.write(0, 1);
    out1.write(uiNut, 6);
    out1.write(uiLayerId, 6);
    out1.write(uiTempId, 3);
    out1.write(in,mediaSample.getPayloadSize()-2);
    packet.setPayload(out1.data());
    packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
    vRtpPackets.push_back(packet);
  }
  return vRtpPackets;
}

int32_t RfchevcPacketiser::getNumberOfSamplesToAggregate(uint32_t uiBytesAvailableForPayload,
                                                         const std::vector<MediaSample>& mediaSamples,
                                                         uint32_t uiStartIndex, uint32_t& uiStapSize,
                                                         bool& bFBit, uint32_t& uiLayerId, uint32_t& uiTempId, bool Interleaved )
{
  int iNalUnitsToAggregate = 0;
  uiStapSize = 0;
  bFBit = false;
  uiLayerId = -1;
  uiTempId = -1;

  // subtract first byte of STAP-A
  ++uiStapSize;
  --uiBytesAvailableForPayload;

  for (size_t i = uiStartIndex; uiBytesAvailableForPayload > 0 && i < mediaSamples.size(); ++i)
  {
    // add 2 bytes for the length of the NAL unit
    uint32_t uiSizeRequiredForThisNal = mediaSamples[i].getPayloadSize() + 2 + 1*(Interleaved==true&&i==uiStartIndex) + 1*(Interleaved==true) ;
    if (uiSizeRequiredForThisNal > uiBytesAvailableForPayload)
      break;

    uint16_t uiNalHeader = (mediaSamples[i].getDataBuffer().data()[0]<<8)+mediaSamples[i].getDataBuffer().data()[1];
    // F-Bit
    if ((uiNalHeader & 0x8000) != 0) bFBit = true;
    // LayerId
    uint32_t uiCurrentLayerId = (uiNalHeader & 0x01F8) >> 3;
    if (uiCurrentLayerId < uiLayerId) uiLayerId = uiCurrentLayerId;
    uint32_t uiCurrentTempId = (uiNalHeader & 0x0007);
    if (uiCurrentTempId < uiTempId) uiTempId = uiCurrentTempId;

    ++iNalUnitsToAggregate;
    uiStapSize += uiSizeRequiredForThisNal;
    uiBytesAvailableForPayload -= uiSizeRequiredForThisNal;
  }

  return iNalUnitsToAggregate;
}

std::vector<RtpPacket> RfchevcPacketiser::packetiseNonInterleaved(const std::vector<MediaSample>& mediaSamples)
{
  std::vector<RtpPacket> vRtpPackets;
  uint32_t iCurrentPacket = 0;
  int iRtpPacketIndex = 0;
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
      uint32_t uiLayerId = 0;
      uint32_t uiTempId = 0;
      uint32_t uiStapSize = 0;
      int32_t iCount = getNumberOfSamplesToAggregate(m_uiBytesAvailableForPayload, mediaSamples, iCurrentPacket, uiStapSize, bFBit, uiLayerId, uiTempId);
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
        out1.write(static_cast<uint32_t>(NUT_AP), 6);
        out1.write(uiLayerId, 6);
        out1.write(uiTempId, 3);
        // we can aggregate some packets
        for (size_t i = iCurrentPacket; i < (uint32_t)iCurrentPacket + iCount; ++i)
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

std::vector<RtpPacket> RfchevcPacketiser::packetiseInterleaved(const std::vector<MediaSample>& mediaSamples)
{
  std::vector<RtpPacket> vRtpPackets;
  uint32_t iCurrentPacket = 0;
  uint16_t uiBaseDON = 0;
  uint8_t uiDiffDON = 0;
  while (iCurrentPacket < mediaSamples.size())
  {
    const MediaSample& mediaSample = mediaSamples[iCurrentPacket];
    if (mediaSample.getDataBuffer().getSize() > m_uiBytesAvailableForPayload-4)
    {
      // FU-A
      std::vector<RtpPacket> vFuRtpPackets = packetiseFuB(mediaSample);
      vRtpPackets.insert(vRtpPackets.end(), vFuRtpPackets.begin(), vFuRtpPackets.end());
      ++iCurrentPacket;
    }
    else
    {
      // see if we can do STAPs
      bool bFBit = false;
      uint32_t uiLayerId = 0;
      uint32_t uiTempId = 0;
      uint32_t uiStapSize = 0;
      uiBaseDON = mediaSample.getDecodingOrderNumber();
      int32_t iCount = getNumberOfSamplesToAggregate(m_uiBytesAvailableForPayload, mediaSamples, iCurrentPacket, uiStapSize, bFBit, uiLayerId, uiTempId, true);
      if (iCount == 1)
      {
        // Single Nal Unit
        RtpPacket packet;
        // STAP-A indicator
        OBitStream out1(uiStapSize);

        IBitStream in(mediaSample.getDataBuffer());
        // forbidden
        in.skipBits(1);
        //NUT
        uint32_t uiNut = 0;
        in.read(uiNut, 6);
        // LayerId
        in.read(uiLayerId, 6);
        // TempId
        in.read(uiLayerId, 3);

        out1.write(bFBit, 1);
        out1.write(static_cast<uint32_t>(NUT_AP), 6);
        out1.write(uiLayerId, 6);
        out1.write(uiTempId, 3);
        out1.write(mediaSample.getDecodingOrderNumber(), 16);
        out1.write(mediaSample.getPayloadSize(), 16);
        out1.write(bFBit, 1);
        out1.write(uiNut, 6);
        out1.write(uiLayerId, 6);
        out1.write(uiTempId, 3);
        out1.write(in,mediaSample.getPayloadSize()-2);
        packet.setPayload(out1.data());
        packet.getHeader().setMarkerBit(mediaSample.isMarkerSet());
        vRtpPackets.push_back(packet);
        ++iCurrentPacket;
      }
      else
      {
        RtpPacket packet;
        // STAP-A indicator
        OBitStream out1(uiStapSize);
        out1.write(bFBit, 1);
        out1.write(static_cast<uint32_t>(NUT_AP), 6);
        out1.write(uiLayerId, 6);
        out1.write(uiTempId, 3);
        // we can aggregate some packets
        for (size_t i = iCurrentPacket; i < (uint32_t)iCount; ++i)
        {
          const MediaSample& mediaSample = mediaSamples[i];
          if(uiBaseDON == mediaSample.getDecodingOrderNumber()){
              out1.write(mediaSample.getDecodingOrderNumber(), 16);
              uiBaseDON = mediaSample.getDecodingOrderNumber();
          }
          else{
              uiBaseDON += 1;
              uiDiffDON = mediaSample.getDecodingOrderNumber() - uiBaseDON;
              out1.write(uiDiffDON , 8);
              uiBaseDON = mediaSample.getDecodingOrderNumber();
          }
          out1.write(mediaSample.getPayloadSize(), 16);
          VLOG(15) << "Copying media sample data into STAP-A: " << mediaSample.getPayloadSize() << " bytes";
          // copy rest of stream
          IBitStream in(mediaSample.getDataBuffer());
          out1.write(in);
        }
        packet.setPayload(out1.data());
        vRtpPackets.push_back(packet);

        iCurrentPacket += iCount;
      }
    }
  }
  return vRtpPackets;
}


std::vector<MediaSample> RfchevcPacketiser::depacketizeSingleNalUnit(const std::list<RtpPacket>& rtpPackets)
{
  std::vector<MediaSample> vSamples;
  // there is only one NAL per RTP packet
  // SPS and PPS could have the same timestamp and thus be grouped together
  for(const RtpPacket& rtpPacket: rtpPackets)
  {
    // check nal unit type
    const Buffer rawData = rtpPacket.getPayload();
    IBitStream in(rawData);
    uint32_t forbidden = 0, layerId = 0, tempId, type = 0;
    if (( in.read(forbidden, 1)) &&
        ( in.read(type, 6)) &&
        ( in.read(layerId, 6 )) &&
        ( in.read(tempId, 3 )) )
    {
        DLOG(INFO) << "Forbidden: " << forbidden << " LayerId: " << layerId << "TempId" << tempId << " type: " << type;
      if (type > NUT_RSV_47)
      {
        LOG(WARNING) << "The NAL unit type " << type << " is not allowed in single nal unit mode";
      }
      else
      {
        MediaSample mediaSample;
        mediaSample.setData(rawData);
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

void RfchevcPacketiser::printPacketListInfo(const std::list<RtpPacket>& rtpPackets)
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

}
}
