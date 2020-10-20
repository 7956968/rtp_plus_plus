#pragma once
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus
{
namespace mprtp
{

class MpRtcpReport
{
public:

  MpRtcpReport();
  MpRtcpReport( uint8_t uiMpRtcpReportType, uint16_t uiFlowId );

  uint8_t getMpRtcpType() const { return m_uiMpRtcpReportType; }
  uint8_t getBlockLengthinWords() const { return m_uiBlockLengthInWords; }
  uint16_t getFlowId() const { return m_uiFlowId; }
  CompoundRtcpPacket getReports() const { return m_vRtcpReports; }

  uint32_t calculateLength() const;
  void addRtcpReport( RtcpPacketBase::ptr pRtcpPacket );
  void write( OBitStream& ob ) const;
  void read( IBitStream& ib );

private:

  uint8_t m_uiMpRtcpReportType;
  // length of block in words - 1 (excluding MPRTP wrapper line)
  uint8_t m_uiBlockLengthInWords;
  uint16_t m_uiFlowId;
  CompoundRtcpPacket m_vRtcpReports;
};

class MpRtcp : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<MpRtcp> ptr;
  typedef std::map<uint16_t, MpRtcpReport> ReportMap_t;
  typedef std::pair<const uint16_t, MpRtcpReport> ReportPair_t;

  static ptr create(uint32_t uiSSRC = 0);

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );
  MpRtcp(uint32_t uiSSRC = 0);

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  MpRtcp(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);

  uint32_t getSSRC() const { return m_uiSSRC; }
  void setSSRC(uint32_t uiSSRC) { m_uiSSRC = uiSSRC; }

  void addMpRtcpReport(uint16_t uiFlowId, uint8_t uiType, RtcpPacketBase::ptr pReport);
  std::vector<RtcpPacketBase::ptr> getRtcpReports(uint16_t uiFlowId);
  ReportMap_t getRtcpReportMap() const { return m_mMpRtcpReports; }

private:
  uint32_t calculateLength() const;
  void writeTypeSpecific(OBitStream& ob) const;
  void readTypeSpecific(IBitStream& ib);

  uint32_t m_uiSSRC;

  // map for RTCP reports sorted according to flow id
  std::map<uint16_t, MpRtcpReport> m_mMpRtcpReports;
};

}
}
