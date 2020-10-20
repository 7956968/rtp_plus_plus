#include "CorePch.h"
#include <rtp++/rfc3550/Rtcp.h>
#include <boost/make_shared.hpp>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <cpputil/Utility.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

RtcpRrReportBlock::RtcpRrReportBlock() :m_uiReporteeSSRC(0),
  m_uiFractionLost(0),
  m_uiCumulativeNumberOfPacketsLost(0),
  m_uiExtendedHighestSNReceived(0),
  m_uiInterarrivalJitter(0),
  m_uiLastSr(0),
  m_uiDelaySinceLastSr(0)
{

}

void RtcpRrReportBlock::write( OBitStream& ob ) const
{
  ob.write(m_uiReporteeSSRC,                  32);
  ob.write(m_uiFractionLost,                   8);
  ob.write(m_uiCumulativeNumberOfPacketsLost, 24);
  ob.write(m_uiExtendedHighestSNReceived,     32);
  ob.write(m_uiInterarrivalJitter,            32);
  ob.write(m_uiLastSr,                        32);
  ob.write(m_uiDelaySinceLastSr,              32);
}

void RtcpRrReportBlock::read( IBitStream& ib )
{
  ib.read(m_uiReporteeSSRC,                  32);    
  ib.read(m_uiFractionLost,                   8);    
  ib.read(m_uiCumulativeNumberOfPacketsLost, 24);    
  ib.read(m_uiExtendedHighestSNReceived,     32);    
  ib.read(m_uiInterarrivalJitter,            32);    
  ib.read(m_uiLastSr,                        32);
  ib.read(m_uiDelaySinceLastSr,              32);
}

std::ostream& operator<< (std::ostream& ostr, const RtcpRrReportBlock& block)
{
  ostr << "RR:"
    << " SSRC :" << hex(block.m_uiReporteeSSRC)
    << " LSR: " << hex(block.m_uiLastSr)
    << " DLSR: " << hex(block.m_uiDelaySinceLastSr)
    << "(" << RtpTime::convertDlsrToSeconds(block.m_uiDelaySinceLastSr) << ") "
    << " XSN: " << hex(block.m_uiExtendedHighestSNReceived)
    << " L: " << hex(block.m_uiCumulativeNumberOfPacketsLost)
    << " LF: " << hex(block.m_uiFractionLost)
    << " J: " << hex(block.m_uiInterarrivalJitter);
  return ostr;
}

RtcpSr::ptr RtcpSr::create()
{
  return boost::make_shared<RtcpSr>();
}

RtcpSr::ptr RtcpSr::create( uint32_t uiSSRC, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw,
                            uint32_t uiRtpTimestamp, uint32_t uiSendersPacketCount, uint32_t uiSendersOctetCount )
{
  return boost::make_shared<RtcpSr>(uiSSRC, uiNtpTimestampMsw, uiNtpTimestampLsw, uiRtpTimestamp, uiSendersPacketCount, uiSendersOctetCount);
}

RtcpSr::ptr RtcpSr::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpSr>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

RtcpSr::RtcpSr()
  :RtcpPacketBase(PT_RTCP_SR),
  m_uiSSRC(0),
  m_uiNtpTimestampMsw(0),
  m_uiNtpTimestampLsw(0),
  m_uiRtpTimestamp(0),
  m_uiSendersPacketCount(0),
  m_uiSendersOctetCount(0)
{

}

RtcpSr::RtcpSr( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
  :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_SR, uiLengthInWords),
  m_uiSSRC(0),
  m_uiNtpTimestampMsw(0),
  m_uiNtpTimestampLsw(0),
  m_uiRtpTimestamp(0),
  m_uiSendersPacketCount(0),
  m_uiSendersOctetCount(0)
{

}

RtcpSr::RtcpSr( uint32_t uiSSRC, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw,
                uint32_t uiRtpTimestamp, uint32_t uiSendersPacketCount, uint32_t uiSendersOctetCount )
  :RtcpPacketBase(PT_RTCP_SR),
  m_uiSSRC(uiSSRC),
  m_uiNtpTimestampMsw(uiNtpTimestampMsw),
  m_uiNtpTimestampLsw(uiNtpTimestampLsw),
  m_uiRtpTimestamp(uiRtpTimestamp),
  m_uiSendersPacketCount(uiSendersPacketCount),
  m_uiSendersOctetCount(uiSendersOctetCount)
{
  setLength(BASIC_SR_LENGTH); // length of sender report specific data
}

bool RtcpSr::addReportBlock( RtcpRrReportBlock reportBlock )
{
  if (m_vReports.size() < 31)
  {
    m_vReports.push_back(reportBlock);
    // update RC field in base RTCP packet
    setTypeSpecific(m_vReports.size());
    // update length field
    setLength(BASIC_SR_LENGTH + m_vReports.size() * BASIC_RR_BLOCK_LENGTH);
    return true;
  }
  return false;
}

void RtcpSr::writeTypeSpecific( OBitStream& ob ) const
{
  ob.write(m_uiSSRC,                32);
  ob.write(m_uiNtpTimestampMsw,     32);    
  ob.write(m_uiNtpTimestampLsw,     32);    
  ob.write(m_uiRtpTimestamp,        32);    
  ob.write(m_uiSendersPacketCount,  32);    
  ob.write(m_uiSendersOctetCount,   32);
  std::for_each(m_vReports.begin(), m_vReports.end(), [&ob](RtcpRrReportBlock report)
  {
    report.write(ob);
  }
  );
}

void RtcpSr::readTypeSpecific( IBitStream& ib )
{
  ib.read(m_uiSSRC,                 32);    
  ib.read(m_uiNtpTimestampMsw,      32);    
  ib.read(m_uiNtpTimestampLsw,      32);    
  ib.read(m_uiRtpTimestamp,         32);    
  ib.read(m_uiSendersPacketCount,   32);    
  ib.read(m_uiSendersOctetCount,    32);
  for (size_t i = 0; i < getTypeSpecific(); ++i)
  {
    RtcpRrReportBlock reportBlock;
    reportBlock.read(ib); // need to add validation to ib!!!!
    m_vReports.push_back(reportBlock);
  }
  // TODO: perform some validation that the raw constructor values add up to the new length
}

RtcpRr::ptr RtcpRr::create()
{
  return boost::make_shared<RtcpRr>();
}

RtcpRr::ptr RtcpRr::create( uint32_t uiSSRC )
{
  return boost::make_shared<RtcpRr>(uiSSRC);
}

RtcpRr::ptr RtcpRr::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpRr>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

RtcpRr::RtcpRr() :RtcpPacketBase(PT_RTCP_RR),
  m_uiReporterSSRC(0)
{
  // update length field
  setLength(BASIC_RR_LENGTH);
}

RtcpRr::RtcpRr( uint32_t uiSSRC ) :RtcpPacketBase(PT_RTCP_RR),
  m_uiReporterSSRC(uiSSRC)
{
  // update length field
  setLength(BASIC_RR_LENGTH);
}

RtcpRr::RtcpRr( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords ) :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_RR, uiLengthInWords),
  m_uiReporterSSRC(0)
{

}

bool RtcpRr::addReportBlock( RtcpRrReportBlock reportBlock )
{
  if (m_vReports.size() < 31)
  {
    m_vReports.push_back(reportBlock);
    // update RC field in base RTCP packet
    setTypeSpecific(m_vReports.size());
    // update length field
    setLength(BASIC_RR_LENGTH + m_vReports.size() * BASIC_RR_BLOCK_LENGTH);
    return true;
  }
  return false;
}

void RtcpRr::writeTypeSpecific( OBitStream& ob ) const
{
  ob.write(m_uiReporterSSRC,                  32);
  std::for_each(m_vReports.begin(), m_vReports.end(), [&ob](RtcpRrReportBlock report)
  {
    report.write(ob);
  }
  );
}

void RtcpRr::readTypeSpecific( IBitStream& ib )
{
  ib.read(m_uiReporterSSRC,                  32);
  for (size_t i = 0; i < getTypeSpecific(); ++i)
  {
    VLOG(10) << "Parsing Rtcp Report Block";
    RtcpRrReportBlock reportBlock;
    reportBlock.read(ib); // need to add validation to ib!!!!
    m_vReports.push_back(reportBlock);
  }
}

RtcpSdesReportBlock::RtcpSdesReportBlock() :m_uiSSRC_CSRC1(0)
{

}

RtcpSdesReportBlock::RtcpSdesReportBlock( uint32_t uiSSRC_CSRC ) :m_uiSSRC_CSRC1(uiSSRC_CSRC)
{

}

std::uint32_t RtcpSdesReportBlock::getOctetCount() const
{
  return 4  + // SSRC CSRC
    addLength(getCName())  +
    addLength(getName())   +
    addLength(getEmail())  +
    addLength(getPhone())  +
    addLength(getLoc())    +
    addLength(getTool())   +
    addLength(getNote())   +
    addLength(getPriv());
}

std::uint32_t RtcpSdesReportBlock::getWordCount() const
{
  uint32_t uiWords = getOctetCount() >> 2;
  if (getPadding()) ++uiWords;
  return uiWords;
}

std::uint32_t RtcpSdesReportBlock::getPadding() const
{
  return (4 - (getOctetCount() % 4));
}

void RtcpSdesReportBlock::write( OBitStream& ob ) const
{
  ob.write(m_uiSSRC_CSRC1, 32);
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_CNAME, getCName());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_NAME,  getName());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_EMAIL, getEmail());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_PHONE, getPhone());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_LOC,   getLoc());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_TOOL,  getTool());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_NOTE,  getNote());
  writeListItemIfNotEmpty(ob, RTCP_SDES_TYPE_PRIV,  getPriv());
}

void RtcpSdesReportBlock::read( IBitStream& ib )
{
  ib.read(m_uiSSRC_CSRC1, 32);
  // read until we see a NULL octet
  while (moreSdesItemsToBeRead(ib))
  {
    // parse SDES list item
    readListItem(ib);
  }
  // now we need to consume the padding until a 32-bit boundary is reached

  VLOG(10) << "Skipping padding: " << getPadding();
  ib.skipBytes(getPadding());
}

bool RtcpSdesReportBlock::moreSdesItemsToBeRead( IBitStream& ib )
{
  // peek ahead and check for NULL octet
  return (ib.getBitsRemaining() > 0 && ib.peekAtCurrentByte() != 0);
}

void RtcpSdesReportBlock::writeListItemIfNotEmpty( OBitStream& ob, uint8_t uiType, const std::string& sValue ) const
{
  if (!sValue.empty())
  {
    std::string sTempVal;
    uint32_t uiLen = sValue.length();
    if (uiLen > RTP_MAX_SDES)
      sTempVal = sValue.substr(0, RTP_MAX_SDES);
    else
      sTempVal = sValue;

    ob.write( uiType,           8);
    ob.write( sTempVal.length(),  8);
    std::for_each(sTempVal.begin(), sTempVal.end(), [&ob](char c)
    {
      ob.write(c, 8);
    }
    );
  }
}

void RtcpSdesReportBlock::readListItem( IBitStream& ib )
{
  uint32_t uiType = 0;
  uint32_t uiLength = 0;
  ib.read( uiType,      8);
  ib.read( uiLength,    8);
  uint8_t buffer[256];
  buffer[uiLength] = '\0';
  for (size_t i = 0; i < uiLength; ++i)
  {
    ib.read(buffer[i], 8);
  }
  std::string sValue(reinterpret_cast<const char*>(buffer), uiLength);
  switch (uiType)
  {
  case RTCP_SDES_TYPE_CNAME:
    {
      VLOG(10) << "Read SDES CName: " << sValue;
      setCName(sValue);
      break;
    }
  case RTCP_SDES_TYPE_NAME:
    {
      VLOG(10) << "Read SDES Name: " << sValue;
      setName(sValue);
      break;
    }
  case RTCP_SDES_TYPE_EMAIL:
    {
      VLOG(10) << "Read SDES Email: " << sValue;
      setEmail(sValue);
      break;
    }
  case RTCP_SDES_TYPE_PHONE:
    {
      VLOG(10) << "Read SDES Phone: " << sValue;
      setPhone(sValue);
      break;
    }
  case RTCP_SDES_TYPE_LOC:  
    {
      VLOG(10) << "Read SDES Loc: " << sValue;
      setLoc(sValue);
      break;
    }
  case RTCP_SDES_TYPE_TOOL: 
    {
      VLOG(10) << "Read SDES Tool: " << sValue;
      setTool(sValue);
      break;
    }
  case RTCP_SDES_TYPE_NOTE: 
    {
      VLOG(10) << "Read SDES Note: " << sValue;
      setNote(sValue);
      break;
    }
  case RTCP_SDES_TYPE_PRIV: 
    {
      VLOG(10) << "Read SDES Priv: " << sValue;
      setPriv(sValue);
      break;
    }
  }
}

std::uint32_t RtcpSdesReportBlock::addLength( const std::string& sValue ) const
{
  return (sValue.empty()) ? 0 : (sValue.length() + SDES_ITEM_HEADER_LENGTH);
}

RtcpSdes::ptr RtcpSdes::create()
{
  return boost::make_shared<RtcpSdes>();
}

RtcpSdes::ptr RtcpSdes::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpSdes>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

RtcpSdes::RtcpSdes() :RtcpPacketBase(PT_RTCP_SDES)
{

}

RtcpSdes::RtcpSdes( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords ) :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_SDES, uiLengthInWords)
{

}

void RtcpSdes::addSdesReport( RtcpSdesReportBlock report )
{
  m_vReports.push_back(report);
  // update base field
  setTypeSpecific(m_vReports.size());
  // update length field
  setLength(getLength() + report.getWordCount());
}

void RtcpSdes::writeTypeSpecific( OBitStream& ob ) const
{
  std::for_each(m_vReports.begin(), m_vReports.end(), [&ob](RtcpSdesReportBlock report)
  {
    report.write(ob);
    // padding
    for (size_t i = 0; i < report.getPadding(); ++i)
    {
      ob.write(0, 8);
    }
  }
  );
}

void RtcpSdes::readTypeSpecific( IBitStream& ib )
{
  for (size_t i = 0; i < getTypeSpecific(); ++i)
  {
    RtcpSdesReportBlock reportBlock;
    reportBlock.read(ib);
    m_vReports.push_back(reportBlock);
  }
}

RtcpApp::ptr RtcpApp::create()
{
  return boost::make_shared<RtcpApp>();
}

RtcpApp::ptr RtcpApp::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpApp>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

RtcpApp::RtcpApp() :RtcpPacketBase(PT_RTCP_APP)
{
  memset(m_PacketName, 0, 4);
}

RtcpApp::RtcpApp( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords ) :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_APP, uiLengthInWords)
{
  memset(m_PacketName, 0, 4);
}

void RtcpApp::setAppData( uint8_t uiSubtype, char packetName[4], uint8_t* uiData, uint32_t uiLenBytes )
{
  // app specific
  setTypeSpecific(uiSubtype);
  memcpy(m_PacketName, packetName, 4);
  uint8_t* pData = new uint8_t[uiLenBytes];
  m_appData.setData(pData, uiLenBytes);
  memcpy(pData, uiData, uiLenBytes);
  // base header length
  recalculateLength();
}

std::uint32_t RtcpApp::getPadding() const
{
  return (4 - (m_appData.getSize() % 4));
}

void RtcpApp::recalculateLength()
{
  uint32_t uiSizeWords = m_appData.getSize() >> 2;
  if (getPadding() > 0) ++uiSizeWords;
  setLength( 2 + uiSizeWords);
}

void RtcpApp::writeTypeSpecific( OBitStream& ob ) const
{
  // SSRC
  ob.write(m_uiSSRC, 32);
  // packet name
  for (size_t i = 0; i < 4; ++i)
  {
    ob.write(m_PacketName[i], 8);
  }

  for (size_t i = 0; i < m_appData.getSize(); ++i)
  {
    ob.write(m_appData[i], 8);
  }

  for (size_t i = 0; i < getPadding(); ++i)
  {
    ob.write(0, 8);
  }
}

void RtcpApp::readTypeSpecific( IBitStream& ib )
{
  // SSRC
  ib.read(m_uiSSRC, 32);
  // packet name
  for (size_t i = 0; i < 4; ++i)
  {
    ib.read(m_PacketName[i], 8);
  }

  // TODO:
  //    for (size_t i = 0; i < m_appData.getSize(); ++i)
  //    {
  //    ob.write(m_appData[i], 8);
  //    }
  //
  //    for (size_t i = 0; i < getPadding(); ++i)
  //    {
  //    ob.write(0, 8);
  //    }
}

RtcpBye::ptr RtcpBye::create()
{
  return boost::make_shared<RtcpBye>();
}

RtcpBye::ptr RtcpBye::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpBye>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

RtcpBye::ptr RtcpBye::create( uint32_t uiSSRC )
{
  return boost::make_shared<RtcpBye>(uiSSRC);
}

RtcpBye::RtcpBye() :RtcpPacketBase(PT_RTCP_BYE)
{

}

RtcpBye::RtcpBye( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords ) :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_BYE, uiLengthInWords)
{

}

RtcpBye::RtcpBye( uint32_t uiSSRC ) :RtcpPacketBase(PT_RTCP_BYE)
{
  addSSRC(uiSSRC);
}

void RtcpBye::addSSRC( uint32_t uiSSRC )
{
  m_vSSRCs.push_back(uiSSRC);
  // update base field
  setTypeSpecific(m_vSSRCs.size());
  // update length field
  recalculateLength();
  //setLength( getLength() + 1);
}

std::uint32_t RtcpBye::getPadding() const
{
  return ( 4 - (m_sOptionalReasonForLeaving.size() % 4 ));
}

void RtcpBye::recalculateLength()
{
  uint32_t uiSizeWords = 0;
  if (!m_sOptionalReasonForLeaving.empty())
  {
    uiSizeWords = m_sOptionalReasonForLeaving.size() >> 2;
    if (getPadding() > 0) ++uiSizeWords;
  }
  setLength( m_vSSRCs.size() + uiSizeWords);
}

void RtcpBye::writeTypeSpecific( OBitStream& ob ) const
{
  std::for_each(m_vSSRCs.begin(), m_vSSRCs.end(), [&ob](uint32_t uiSSRC)
  {
    ob.write(uiSSRC, 32);
  }
  );
  if (!m_sOptionalReasonForLeaving.empty())
  {
    ob.write( m_sOptionalReasonForLeaving.length(),  8);
    std::for_each(m_sOptionalReasonForLeaving.begin(), m_sOptionalReasonForLeaving.end(), [&ob](char c)
    {
      ob.write(c, 8);
    }
    );

    // padding
    for (size_t i = 0; i < getPadding(); ++i)
    {
      ob.write(0, 8);
    }
  }
}

void RtcpBye::readTypeSpecific( IBitStream& ib )
{
  for (size_t i = 0; i < getTypeSpecific(); ++i)
  {
    uint32_t uiSSRC = 0;
    ib.read(uiSSRC, 32);
    m_vSSRCs.push_back(uiSSRC);
  }

  uint8_t uiLength = 0;
  ib.read(uiLength, 8);

  uint8_t buffer[256];
  buffer[uiLength] = '\0';
  for (size_t i = 0; i < uiLength; ++i)
  {
    ib.read(buffer[i], 8);
  }
  m_sOptionalReasonForLeaving = std::string(reinterpret_cast<const char*>(buffer), uiLength);

  // padding
  uint8_t dummy = 0;
  for (size_t i = 0; i < getPadding(); ++i)
  {
    ib.read(dummy, 8);
  }
}

}
}
