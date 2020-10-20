#include "CorePch.h"
#include <rtp++/mprtp/MpRtcp.h>
#include <cassert>
#include <boost/make_shared.hpp>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/rfc4585/Rfc4585.h>

// #define DEBUG_MPRTP

namespace rtp_plus_plus
{
namespace mprtp
{

MpRtcpReport::MpRtcpReport()
  :m_uiMpRtcpReportType(0),
    m_uiBlockLengthInWords(0),
    m_uiFlowId(0)
{
}

MpRtcpReport::MpRtcpReport( uint8_t uiMpRtcpReportType, uint16_t uiFlowId )
  :m_uiMpRtcpReportType( uiMpRtcpReportType ),
    m_uiBlockLengthInWords( 1 ),
    m_uiFlowId( uiFlowId )
{
}

uint32_t MpRtcpReport::calculateLength() const
{
  // calculate length of parsed packets
  uint32_t uiLen = 0;
  for (size_t i = 0; i < m_vRtcpReports.size(); ++i)
  {
    uiLen += (1 + m_vRtcpReports[i]->getLength());
  }

#ifdef DEBUG_MPRTP
  VLOG(2) << "Recalculated MPRTCP length. Length: " << uiLen
             << " Reports: " << m_vRtcpReports.size();
#endif
  return uiLen;
}

void MpRtcpReport::addRtcpReport( RtcpPacketBase::ptr pRtcpPacket )
{
  m_vRtcpReports.push_back(pRtcpPacket);
  m_uiBlockLengthInWords = calculateLength();
}

void MpRtcpReport::write( OBitStream& ob ) const
{
  ob.write(m_uiMpRtcpReportType,   8);
  ob.write(m_uiBlockLengthInWords, 8);
  ob.write(m_uiFlowId,             16);

#ifdef DEBUG_MPRTP
  VLOG(2) << "Writing MPRTCP "
          << (int)m_uiMpRtcpReportType << " "
          << (int)m_uiBlockLengthInWords << " "
          << (int)m_uiFlowId << " "
          << m_vRtcpReports.size();
#endif

  std::for_each(m_vRtcpReports.begin(), m_vRtcpReports.end(), [&ob](const RtcpPacketBase::ptr& pReport)
  {
    ob << pReport.get();
    // pReport->write(ob);
  });
}

void MpRtcpReport::read( IBitStream& ib )
{
  ib.read(m_uiMpRtcpReportType,    8);
  ib.read(m_uiBlockLengthInWords,  8);
  ib.read(m_uiFlowId,              16);

#ifdef DEBUG_MPRTP
  VLOG(2) << "Read MPRTCP "
          << (int)m_uiMpRtcpReportType << " "
          << (int)m_uiBlockLengthInWords << " "
          << (int)m_uiFlowId << " ";
#endif

  RtpPacketiser packetiser;
  int32_t uiBlocksToBeRead = m_uiBlockLengthInWords;
  while (uiBlocksToBeRead > 0)
  {
    RtcpPacketBase::ptr pRtcpReport = packetiser.parseRtcpPacket(ib);
    m_vRtcpReports.push_back(pRtcpReport);
    uiBlocksToBeRead -= (pRtcpReport->getLength() + 1);
  }

#ifdef DEBUG_MPRTP
  VLOG(2) << "Block length in words: " << (int32_t) m_uiBlockLengthInWords
          << " calculated: " << calculateLength();
#endif
  assert(m_uiBlockLengthInWords == calculateLength());
}


MpRtcp::ptr MpRtcp::create(uint32_t uiSSRC)
{
  return boost::make_shared<MpRtcp>(uiSSRC);
}

// raw data constructor: this can be used when the report will parse the body itself
MpRtcp::ptr MpRtcp::create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
{
  return boost::make_shared<MpRtcp>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
}

MpRtcp::MpRtcp(uint32_t uiSSRC)
  :RtcpPacketBase( PT_MPRTCP ),
    m_uiSSRC( uiSSRC )
{
  setLength(1); // length of SSRC in words
}

// NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
// be checked after the read method
MpRtcp::MpRtcp(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords)
  :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_MPRTCP, uiLengthInWords)
{
  // NB: do not set length here!!!
}

void MpRtcp::addMpRtcpReport(uint16_t uiFlowId, uint8_t uiType, RtcpPacketBase::ptr pReport)
{
#ifdef DEBUG_MPRTP
  VLOG(2) << "Adding MPRTCP report Flow: " << uiFlowId
          << " Type: " << (int)uiType
          << " Report Type: " << (int)pReport->getPacketType()
          << " Length: " << pReport->getLength();
#endif

  auto it = m_mMpRtcpReports.find(uiFlowId);
  if (it == m_mMpRtcpReports.end())
  {
    m_mMpRtcpReports[uiFlowId] = MpRtcpReport(uiType, uiFlowId);
  }
  m_mMpRtcpReports[uiFlowId].addRtcpReport(pReport);

  setLength(calculateLength());

#ifdef DEBUG_MPRTP
  VLOG(2) << "New Length: " << getLength();
#endif
}

std::vector<RtcpPacketBase::ptr> MpRtcp::getRtcpReports(uint16_t uiFlowId)
{
  auto it = m_mMpRtcpReports.find(uiFlowId);
  if (it != m_mMpRtcpReports.end())
  {
    return it->second.getReports();
  }
  else
  {
    return std::vector<RtcpPacketBase::ptr>();
  }
}

uint32_t MpRtcp::calculateLength() const
{
  // calculate length
  uint32_t uiLen = 1; // SSRC
  for (auto it = m_mMpRtcpReports.begin(); it != m_mMpRtcpReports.end(); ++it)
  {
    uiLen += (1 + it->second.getBlockLengthinWords()); // Add the word for the MPRTCP sub-header
  }
  return uiLen;
}

void MpRtcp::writeTypeSpecific(OBitStream& ob) const
{
  ob.write(m_uiSSRC,        32);
  std::for_each(m_mMpRtcpReports.begin(), m_mMpRtcpReports.end(), [&ob](const std::pair<uint16_t, MpRtcpReport>& reportPair)
  {
    reportPair.second.write(ob);
  });
}

void MpRtcp::readTypeSpecific(IBitStream& ib)
{
  ib.read(m_uiSSRC,                 32);
  int32_t iToBeRead = getLength() - 1; // subtract SSRC
  while (iToBeRead > 0)
  {
    MpRtcpReport report;
    report.read(ib);
    m_mMpRtcpReports[report.getFlowId()] = report;
    /*      auto it = m_mMpRtcpReports.find(report.getFlowId());
      if (it == m_mMpRtcpReports.end())
      {
          m_mMpRtcpReports[report.getFlowId()] = MpRtcpReport(report.
      }
*/
    iToBeRead -= ( report.getBlockLengthinWords() + 1 );
  }
}

}
}
