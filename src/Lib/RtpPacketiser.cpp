#include "CorePch.h"
#include <rtp++/RtpPacketiser.h>
#include <algorithm>
#include <numeric>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3550/RtcpParser.h>
#include <rtp++/rfc5285/RtpHeaderExtension.h>

#define COMPONENT_LOG_LEVEL 10

// #define DEBUG_RTCP
// #define DEBUG_MPRTP
#ifdef DEBUG_MPRTP
#include <rtp++/mprtp/MpRtpHeader.h>
#endif

namespace rtp_plus_plus
{

RtpPacketiser::RtpPacketiser()
  :m_pValidator(std::unique_ptr<rfc3550::RtcpValidator>(new rfc3550::RtcpValidator()))
{
  registerDefaultRtcpParser();
}

RtpPacketiser::RtpPacketiser(std::unique_ptr<rfc3550::RtcpValidator> pValidator)
  :m_pValidator(std::move(pValidator))
{
  registerDefaultRtcpParser();
}

RtpPacketiser::~RtpPacketiser()
{

}

bool RtpPacketiser::validate(const CompoundRtcpPacket& rtcpPackets)
{
  return m_pValidator->validateCompoundRtcpPacket(rtcpPackets, 0);
}

Buffer RtpPacketiser::packetise( const RtpPacket& rtpPacket )
{
  Buffer buffer(new uint8_t[rtpPacket.getSize()], rtpPacket.getSize());

  OBitStream ob(buffer);
  ob.write( rtpPacket.getHeader().getVersion(),         2);
  ob.write( rtpPacket.getHeader().hasPadding(),         1);
  ob.write( rtpPacket.getHeader().hasExtension(),       1);
  ob.write( rtpPacket.getHeader().getCC(),              4);
  ob.write( rtpPacket.getHeader().isMarkerSet(),        1);
  ob.write( (uint32_t) rtpPacket.getHeader().getPayloadType(),    7);
  ob.write( rtpPacket.getHeader().getSequenceNumber(), 16);
  ob.write( rtpPacket.getHeader().getRtpTimestamp(),   32);
  ob.write( rtpPacket.getHeader().getSSRC(),           32);
#if 0
  Logger& rLogger = Logger::getInstance();
  LOG_INFO(rLogger, LOG_FUNCTION, "RTP Packet size: %1%", ob.bytesUsed());
#endif
  assert(ob.bytesUsed() == 12);

  std::vector<uint32_t> vCSRCs = rtpPacket.getHeader().getCSRCs();
  std::for_each(vCSRCs.begin(), vCSRCs.end(), [&ob](uint32_t uiCSRC)
  {
    ob.write(uiCSRC,     32);
  } 
  );

  // write extension header
  const rfc5285::RtpHeaderExtension& headerExtension = rtpPacket.getHeader().getHeaderExtension();
  if (headerExtension.containsExtensions())
  {
#ifdef DEBUG_MPRTP
      // Potentially expensive???: create a work around if this proves to be a problem
      mprtp::MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pExtension.get());
      if (pSubflowHeader)
      {
        uint16_t uiFlowId = pSubflowHeader->getFlowId();
        uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();

        VLOG(2) << "Sending packet with SN " << rtpPacket.getSequenceNumber()
                << " FlowId: " << uiFlowId
                << " FSSN: " << uiFlowSpecificSequenceNumber;
      }
#endif
    headerExtension.writeExtensionData(ob);
  }

  // write packet data
  memcpy(&buffer[ob.bytesUsed()], rtpPacket.getPayload().data(), rtpPacket.getPayload().getSize());

  return buffer;
}

Buffer RtpPacketiser::packetise( const RtpPacket& rtpPacket, uint32_t uiPreferredBufferSize )
{
  // we know how much we need for the packet, so use the rest as post buffer
  assert(uiPreferredBufferSize >= rtpPacket.getSize());

  uint32_t uiPostBuffer = uiPreferredBufferSize - rtpPacket.getSize();
  Buffer buffer(new uint8_t[uiPreferredBufferSize], uiPreferredBufferSize, 0, uiPostBuffer);

  OBitStream ob(buffer);
  ob.write( rtpPacket.getHeader().getVersion(),         2);
  ob.write( rtpPacket.getHeader().hasPadding(),         1);
  ob.write( rtpPacket.getHeader().hasExtension(),       1);
  ob.write( rtpPacket.getHeader().getCC(),              4);
  ob.write( rtpPacket.getHeader().isMarkerSet(),        1);
  ob.write( (uint32_t) rtpPacket.getHeader().getPayloadType(),    7);
  ob.write( rtpPacket.getHeader().getSequenceNumber(), 16);
  ob.write( rtpPacket.getHeader().getRtpTimestamp(),   32);
  ob.write( rtpPacket.getHeader().getSSRC(),           32);
#if 0
  Logger& rLogger = Logger::getInstance();
  LOG_INFO(rLogger, LOG_FUNCTION, "RTP Packet size: %1%", ob.bytesUsed());
#endif
  assert(ob.bytesUsed() == 12);

  std::vector<uint32_t> vCSRCs = rtpPacket.getHeader().getCSRCs();
  std::for_each(vCSRCs.begin(), vCSRCs.end(), [&ob](uint32_t uiCSRC)
  {
    ob.write(uiCSRC,     32);
  }
  );

  // write extension header
  rfc5285::RtpHeaderExtension extension = rtpPacket.getHeader().getHeaderExtension();
  if (extension.containsExtensions())
  {
#ifdef DEBUG_MPRTP
      // Potentially expensive???: create a work around if this proves to be a problem
      mprtp::MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pExtension.get());
      if (pSubflowHeader)
      {
        uint16_t uiFlowId = pSubflowHeader->getFlowId();
        uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();

        VLOG(2) << "Sending packet with SN " << rtpPacket.getSequenceNumber()
                << " FlowId: " << uiFlowId
                << " FSSN: " << uiFlowSpecificSequenceNumber;
      }
#endif
    extension.writeExtensionData(ob);
  }

  // write packet data
  memcpy(&buffer[ob.bytesUsed()], rtpPacket.getPayload().data(), rtpPacket.getPayload().getSize());

  return buffer;
}

Buffer RtpPacketiser::packetise( CompoundRtcpPacket rtcpPackets )
{
  OBitStream ob;
  std::for_each(rtcpPackets.begin(), rtcpPackets.end(), [&ob](RtcpPacketBase::ptr pRtcpPacket)
  {
    ob << pRtcpPacket.get();
  });
  return ob.str();
}

Buffer RtpPacketiser::packetise( CompoundRtcpPacket rtcpPackets, uint32_t uiPreferredBufferSize )
{
  // allocate a few (4) bytes of prebuffer space
  const uint32_t uiPreBufferBytes = 4;
  Buffer buffer(new uint8_t[uiPreferredBufferSize], uiPreferredBufferSize, uiPreBufferBytes, 0);
  OBitStream ob(buffer);

  std::for_each(rtcpPackets.begin(), rtcpPackets.end(), [&ob](RtcpPacketBase::ptr pRtcpPacket)
  {
    ob << pRtcpPacket.get();
  });
  return ob.str();
}

boost::optional<RtpPacket> RtpPacketiser::doDepacketise( Buffer buffer )
{
  if (buffer.getSize() < 12)
  {
    LOG(WARNING) << "Invalid RTP packet. Too small";
    return boost::optional<RtpPacket>();
  }

  IBitStream ib(buffer);

  uint32_t uiVersion   = UINT_MAX;
  uint32_t uiPadding   = UINT_MAX;
  uint32_t uiExtension = UINT_MAX;
  uint32_t uiCC        = UINT_MAX;
  uint32_t uiMarker    = UINT_MAX;
  uint32_t uiPayload   = UINT_MAX;
  uint32_t uiSN        = UINT_MAX;
  uint32_t uiTS        = UINT_MAX;
  uint32_t uiSSRC      = UINT_MAX;

  // RTP version: 2 bits
  ib.read(uiVersion, 2);
  // we don't support RTP version other than version 2
  if (uiVersion != rfc3550::RTP_VERSION_NUMBER)
  {
    LOG(WARNING) << "Invalid RTP Version number";
    return boost::optional<RtpPacket>();
  }
  // Padding: 1 bit
  ib.read(uiPadding, 1);
  // Extension: 1 bit
  ib.read(uiExtension, 1);
  // CC: 4 bits
  ib.read(uiCC, 4);
  // Marker: 1 bit
  ib.read(uiMarker, 1);
  // Payload type: 7 bits
  ib.read(uiPayload, 7);
  // Sequence Number: 16 bits
  ib.read(uiSN, 16);
  // Timestamp: 32 bits
  ib.read(uiTS, 32);
  // SSRC: 32 bits
  ib.read(uiSSRC, 32);

  rfc3550::RtpHeader header( i2b(uiPadding), i2b(uiMarker), uiPayload, uiSN, uiTS, uiSSRC) ;
  for (size_t i = 0; i < uiCC; ++i)
  {
    uint32_t uiCSRC = UINT_MAX;
    // SSRC: 32 bits
    ib.read(uiCSRC, 32);
    header.addContributingSource(uiCSRC);
  }

  if (uiExtension)
  {
    rfc5285::RtpHeaderExtension& extension = header.getHeaderExtension();
    extension.readExtensionData(ib);
  }

  uint32_t uiPayloadSize = ib.getBytesRemaining();
  uint8_t* pPayload = new uint8_t[uiPayloadSize];
  Buffer payload(pPayload, uiPayloadSize);
  ib.readBytes(pPayload, uiPayloadSize);

  RtpPacket packet;
  packet.setRtpHeader(header);
  packet.setPayload(payload);
  // set payload
  return boost::optional<RtpPacket>(packet);
}

boost::optional<RtpPacket> RtpPacketiser::depacketise( Buffer buffer )
{
  return doDepacketise(buffer);
}

boost::optional<RtpPacket> RtpPacketiser::depacketise( NetworkPacket networkPacket )
{
  boost::optional<RtpPacket> packet = doDepacketise(networkPacket);
  if (packet) packet->setNtpArrivalTime(networkPacket.getNtpArrivalTime());
  return packet;
}

RtcpPacketBase::ptr RtpPacketiser::parseRtcpPacket(IBitStream& ib)
{
  uint32_t uiVersion        = UINT_MAX;
  uint32_t uiPadding        = UINT_MAX;
  uint32_t uiTypeSpecific   = UINT_MAX;
  uint32_t uiPayload        = UINT_MAX;
  uint32_t uiLengthInWords  = UINT_MAX;

  // RTP version: 2 bits
  ib.read(uiVersion, 2);
  // Padding: 1 bit
  ib.read(uiPadding, 1);
  // Type specific: 5 bits
  ib.read(uiTypeSpecific, 5);
  // Payload type: 8 bits
  ib.read(uiPayload, 8);
  // Length in 32bit words: 16 bits
  ib.read(uiLengthInWords, 16);

#ifdef DEBUG_RTCP
  VLOG(2) << "RTCP: "
          << (int)uiVersion << " "
          << (int)uiPadding << " "
          << (int)uiTypeSpecific << " "
          << (int)uiPayload << " "
          << (int)uiLengthInWords;
#endif

  // we don't support RTP version other than version 2
  if (uiVersion != rfc3550::RTP_VERSION_NUMBER)
  {
    LOG_FIRST_N(WARNING, 1) << "Unsupported RTP version number: " << uiVersion;
    return RtcpPacketBase::ptr();
  }
  std::vector<int> indices = findRtcpParsers(uiPayload);
  if (indices.empty())
  {
    LOG(WARNING) << "No parser for RTCP packet type: " << uiPayload;
    // skip to next packet
    if (!ib.skipBytes(uiLengthInWords << 2))
    {
      LOG(WARNING) << "Failed to skip bytes when parsing RTCP header";
      // something's gone wrong: exit
      return RtcpPacketBase::ptr();
    }
    else
    {
      LOG(WARNING) << "Skipping unknown RTCP packet type: " << uiPayload;
      // try and parse the next packet in the stream
      return parseRtcpPacket(ib);
    }
  }
  else
  {
    // making this code more robust by allowing multiple parsers to be registered.
    // This method returns the first packet successfully parsed
    for (int index : indices)
    {
      // parse found parser to handle RTCP report
      RtcpPacketBase::ptr pRtcpPacket = m_vRtcpParsers[index]->parseRtcpPacket(ib, uiVersion, uiPadding, uiTypeSpecific, uiPayload, uiLengthInWords);
      if (pRtcpPacket)
      {
        return pRtcpPacket;
      }
      else
      {
        continue;
      }
    }
    return RtcpPacketBase::ptr();
  }
}

CompoundRtcpPacket RtpPacketiser::doDepacketiseRtcp( Buffer buffer )
{
  VLOG(COMPONENT_LOG_LEVEL) << "Depacketising RTCP: parsing buffer: " << buffer.getSize();
  CompoundRtcpPacket compoundPacket;

  IBitStream ib(buffer);

  while (ib.getBitsRemaining())
  {
    RtcpPacketBase::ptr pRtcp = parseRtcpPacket(ib);
    // if something went wrong, exit!
    if (pRtcp) compoundPacket.push_back(pRtcp);
    else break;
  }

  VLOG(COMPONENT_LOG_LEVEL) << "Depacketising RTCP: packets parsed: " << compoundPacket.size();

  if ( m_bValidate )
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Validating RTCP compound packet: " << compoundPacket.size();
    if ( m_pValidator->validateCompoundRtcpPacket(compoundPacket, buffer.getSize()))
      return compoundPacket;
    return CompoundRtcpPacket();
  }
  else
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Not validating RTCP compound packet: " << compoundPacket.size();
    return compoundPacket;
  }
}

CompoundRtcpPacket RtpPacketiser::depacketiseRtcp( Buffer buffer )
{
  return doDepacketiseRtcp(buffer);
}

CompoundRtcpPacket RtpPacketiser::depacketiseRtcp( NetworkPacket packet )
{
  CompoundRtcpPacket packets = doDepacketiseRtcp(packet);
// #define DEBUG_NTP
#ifdef DEBUG_NTP
  DLOG(INFO) << "NTP arrival time: " << convertNtpTimestampToPosixTime(packet.getNtpArrivalTime());
#endif

  std::for_each(packets.begin(), packets.end(), [packet](RtcpPacketBase::ptr pRtcpPacket)
  {
    pRtcpPacket->setArrivalTimeNtp(packet.getNtpArrivalTime());
  });

  return packets;
}

std::vector<int> RtpPacketiser::findRtcpParsers( uint32_t uiType ) const
{
  std::vector<int> indices;
  // iterate over RTCP parser vector and find index of capable parser
  for (size_t i = 0; i < m_vRtcpParsers.size(); ++i)
  {
    if (m_vRtcpParsers[i]->canHandleRtcpType(uiType))
      indices.push_back(i);
  }
  return indices;
}

void RtpPacketiser::registerDefaultRtcpParser()
{
  std::unique_ptr<RtcpParserInterface> pParser = std::unique_ptr<rfc3550::RtcpParser>( new rfc3550::RtcpParser() );
  registerRtcpParser(std::move(pParser));
}

} // rtp_plus_plus
