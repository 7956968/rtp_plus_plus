#include "CorePch.h"
#include <rtp++/RtcpPacketBase.h>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>

namespace rtp_plus_plus
{

std::ostream& operator<<( std::ostream& ostr, const RtcpPacketBase& rtcpPacket )
{
  ostr << "V: " << (int) rtcpPacket.getVersion();
  ostr << " P: " << rtcpPacket.getPadding();
  ostr << " Type specific: " << (int)rtcpPacket.getTypeSpecific();
  ostr << " Type: " << (int)rtcpPacket.getPacketType();
  ostr << " Length: " << rtcpPacket.getLength();
  return ostr;
}

OBitStream& operator<<( OBitStream& stream, const RtcpPacketBase& rtcpHeader )
{
  rtcpHeader.write(stream);
  return stream;
}

OBitStream& operator<<( OBitStream& ob, const RtcpPacketBase* pRtcpPacket )
{
#define OPTION1
#ifdef OPTION1
  // option 1
  pRtcpPacket->write(ob);
#else
  // option 2
  ob.write( pRtcpPacket->m_uiVersion,         2);
  ob.write( pRtcpPacket->m_bPadding,          1);
  ob.write( pRtcpPacket->m_uiTypeSpecific,    5);
  ob.write( pRtcpPacket->m_uiPacketType,     8);
  ob.write( pRtcpPacket->m_uiLength,         16);
  pRtcpPacket->writeTypeSpecific(ob);
#endif
  return ob;
}

RtcpPacketBase::RtcpPacketBase( uint8_t uiPacketType ) 
  :m_uiVersion(rfc3550::RTP_VERSION_NUMBER ),
  m_bPadding(false),
  m_uiTypeSpecific(0),
  m_uiPacketType(uiPacketType),
  m_uiLength(0),
  m_uiArrivalTimeNtp(0)
{

}

RtcpPacketBase::RtcpPacketBase( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint8_t uiPayload, uint16_t uiLengthInWords ) 
  :m_uiVersion(uiVersion),
  m_bPadding(uiPadding),
  m_uiTypeSpecific(uiTypeSpecific),
  m_uiPacketType(uiPayload),
  m_uiLength(uiLengthInWords),
  m_uiArrivalTimeNtp(0)
{

}

void RtcpPacketBase::read( IBitStream& ib )
{
  // The read method only calls the read type specific function
  // since by the time we know what type of report we are dealing
  // with, we have already parsed the first word.
  readTypeSpecific(ib);
  // However: we still need to 'read' the padding
  if (m_bPadding)
  {
    // TODO: calculate amount of padding and call skip
  }
}

void RtcpPacketBase::write( OBitStream& ob ) const
{
  ob.write( m_uiVersion,         2);
  ob.write( m_bPadding,          1);
  ob.write( m_uiTypeSpecific,    5);
  ob.write( m_uiPacketType,     8);
  ob.write( m_uiLength,         16);

#ifdef DEBUG_RTCP
  VLOG(2) << "RTCP: "
          << (int)m_uiVersion << " "
          << (int)m_bPadding << " "
          << (int)m_uiTypeSpecific << " "
          << (int)m_uiPacketType << " "
          << (int)m_uiLength;
#endif

  writeTypeSpecific(ob);
}

} // rtp_plus_plus
