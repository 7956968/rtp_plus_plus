#pragma once
#include <cpputil/Buffer.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc3550/Rfc3550.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

static bool isRtcpPacketType(uint8_t type)
{
  return ((type >= PT_RTCP_SR) &&(type <= PT_RTCP_RESERVED));
}
static bool isByePacket(const RtcpPacketBase& packet)
{
  return packet.getPacketType() == PT_RTCP_BYE;
}
static bool isSrPacket(const RtcpPacketBase& packet)
{
  return packet.getPacketType() == PT_RTCP_SR;
}
static bool isRrPacket(const RtcpPacketBase& packet)
{
  return packet.getPacketType() == PT_RTCP_RR;
}

static bool isStandardRtcpReport(const RtcpPacketBase& packet)
{
  switch (packet.getPacketType())
  {
  case PT_RTCP_SR:
  case PT_RTCP_RR:
  case PT_RTCP_SDES:
  case PT_RTCP_BYE:
  case PT_RTCP_APP:
  {
    return true;
  }
  default:
  {
    return false;
  }
  }
}

class RtcpRrReportBlock
{
  friend std::ostream& operator<< (std::ostream& stream, const RtcpRrReportBlock& reportBlock);
public:

  RtcpRrReportBlock();

  uint32_t getReporteeSSRC() const { return m_uiReporteeSSRC; }
  void setReporteeSSRC(uint32_t val) { m_uiReporteeSSRC = val; }
  uint8_t getFractionLost() const { return m_uiFractionLost; }
  void setFractionLost(uint8_t val) { m_uiFractionLost = val; }
  int32_t getCumulativeNumberOfPacketsLost() const { return m_uiCumulativeNumberOfPacketsLost; }
  void setCumulativeNumberOfPacketsLost(int32_t val) { m_uiCumulativeNumberOfPacketsLost = val; }
  uint32_t getExtendedHighestSNReceived() const { return m_uiExtendedHighestSNReceived; }
  void setExtendedHighestSNReceived(uint32_t val) { m_uiExtendedHighestSNReceived = val; }
  uint32_t getInterarrivalJitter() const { return m_uiInterarrivalJitter; }
  void setInterarrivalJitter(uint32_t val) { m_uiInterarrivalJitter = val; }
  uint32_t getLastSr() const { return m_uiLastSr; }
  void setLastSr(uint32_t val) { m_uiLastSr = val; }
  uint32_t getDelaySinceLastSr() const { return m_uiDelaySinceLastSr; }
  void setDelaySinceLastSr(uint32_t val) { m_uiDelaySinceLastSr = val; }

  // Bitstream methods
  void write(OBitStream& ob) const;
  void read(IBitStream& ib);

private:
  uint32_t m_uiReporteeSSRC;
  uint8_t  m_uiFractionLost;
  uint32_t m_uiCumulativeNumberOfPacketsLost;
  uint32_t m_uiExtendedHighestSNReceived;
  uint32_t m_uiInterarrivalJitter;
  uint32_t m_uiLastSr;
  uint32_t m_uiDelaySinceLastSr;
};

std::ostream& operator<< (std::ostream& ostr, const RtcpRrReportBlock& block);

class RtcpSr : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpSr> ptr;

  // default named constructor
  static ptr create();
  static ptr create( uint32_t uiSSRC, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw, 
    uint32_t uiRtpTimestamp, uint32_t uiSendersPacketCount, uint32_t uiSendersOctetCount );
  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );

  explicit RtcpSr();

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  explicit RtcpSr(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);

  explicit RtcpSr( uint32_t uiSSRC, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw,
    uint32_t uiRtpTimestamp, uint32_t uiSendersPacketCount, uint32_t uiSendersOctetCount );

  uint32_t getSSRC() const { return m_uiSSRC; }
  void setSSRC(uint32_t val) { m_uiSSRC = val; }
  uint32_t getNtpTimestampMsw() const { return m_uiNtpTimestampMsw; }
  void setNtpTimestampMsw(uint32_t val) { m_uiNtpTimestampMsw = val; }
  uint32_t getNtpTimestampLsw() const { return m_uiNtpTimestampLsw; }
  void setNtpTimestampLsw(uint32_t val) { m_uiNtpTimestampLsw = val; }
  uint32_t getRtpTimestamp() const { return m_uiRtpTimestamp; }
  void setRtpTimestamp(uint32_t val) { m_uiRtpTimestamp = val; }
  uint32_t getSendersPacketCount() const { return m_uiSendersPacketCount; }
  void setSendersPacketCount(uint32_t val) { m_uiSendersPacketCount = val; }
  uint32_t getSendersOctetCount() const { return m_uiSendersOctetCount; }
  void setSendersOctetCount(uint32_t val) { m_uiSendersOctetCount = val; }

  bool addReportBlock(RtcpRrReportBlock reportBlock);

  const std::vector<RtcpRrReportBlock>& getReceiverReportBlocks() const
  {
    return m_vReports;
  }

protected:
  void writeTypeSpecific(OBitStream& ob) const;
  void readTypeSpecific(IBitStream& ib);

private:
  uint32_t m_uiSSRC;
  uint32_t m_uiNtpTimestampMsw;
  uint32_t m_uiNtpTimestampLsw;
  uint32_t m_uiRtpTimestamp;
  uint32_t m_uiSendersPacketCount;
  uint32_t m_uiSendersOctetCount;
  std::vector<RtcpRrReportBlock> m_vReports;
};

class RtcpRr : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpRr> ptr;

  static ptr create();
  static ptr create(uint32_t uiSSRC);
  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );

  RtcpRr();

  RtcpRr(uint32_t uiSSRC);

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpRr(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);

  uint32_t getReporterSSRC() const { return m_uiReporterSSRC; }
  void setReporterSSRC(uint32_t val) { m_uiReporterSSRC = val; }

  bool addReportBlock(RtcpRrReportBlock reportBlock);
  std::vector<RtcpRrReportBlock> getReceiverReportBlocks() const
  {
    return m_vReports;
  }

protected:
  void writeTypeSpecific(OBitStream& ob) const;
  void readTypeSpecific(IBitStream& ib);

private:
  uint32_t m_uiReporterSSRC;
  std::vector<RtcpRrReportBlock> m_vReports;
};

class RtcpSdesReportBlock : public SdesInformation
{
public:
  RtcpSdesReportBlock();

  RtcpSdesReportBlock(uint32_t uiSSRC_CSRC);

  // returns length of items in octets
  uint32_t getOctetCount() const;

  uint32_t getSSRC_CSRC1() const { return m_uiSSRC_CSRC1; }
  void setSSRC_CSRC1(uint32_t val) { m_uiSSRC_CSRC1 = val; }

  uint32_t getWordCount() const;

  uint32_t getPadding() const;

  void write(OBitStream& ob) const;

  void read(IBitStream& ib);

private:
  bool moreSdesItemsToBeRead(IBitStream& ib);

  void writeListItemIfNotEmpty(OBitStream& ob, uint8_t uiType, const std::string& sValue) const;

  void readListItem(IBitStream& ib);

  uint32_t addLength(const std::string& sValue) const;

  uint32_t m_uiSSRC_CSRC1;
};

class RtcpSdes : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpSdes> ptr;

  static ptr create();

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );

  RtcpSdes();

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpSdes(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);

  void addSdesReport(RtcpSdesReportBlock report);
  std::vector<RtcpSdesReportBlock> getSdesReports() const { return m_vReports; }

private:
  void writeTypeSpecific(OBitStream& ob) const;
  void readTypeSpecific(IBitStream& ib);

  std::vector<RtcpSdesReportBlock> m_vReports;
};

class RtcpApp : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpApp> ptr;

  static ptr create();

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );

  RtcpApp();

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpApp(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);

  uint32_t getSSRC() const { return m_uiSSRC; }
  void setSSRC(uint32_t val) { m_uiSSRC = val; }

  void setAppData(uint8_t uiSubtype, char packetName[4], uint8_t* uiData, uint32_t uiLenBytes);

private:
  uint32_t getPadding() const;

  void recalculateLength();

  void writeTypeSpecific(OBitStream& ob) const;

  void readTypeSpecific(IBitStream& ib);

  uint32_t m_uiSSRC;
  uint8_t m_PacketName[4];
  Buffer m_appData;
};

class RtcpBye : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpBye> ptr;

  static ptr create();

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords );

  static ptr create(uint32_t uiSSRC);

  RtcpBye();

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpBye(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords);
  RtcpBye(uint32_t uiSSRC);

  std::string getReasonForLeaving() const { return m_sOptionalReasonForLeaving; }
  void setReasonForLeaving(std::string val) 
  { 
    m_sOptionalReasonForLeaving = val;
    recalculateLength();
  }

  void addSSRC(uint32_t uiSSRC);
  std::vector<uint32_t> getSSRCs() const { return m_vSSRCs; }

private:
  uint32_t getPadding() const;
  void recalculateLength();
  void writeTypeSpecific(OBitStream& ob) const;
  void readTypeSpecific(IBitStream& ib);
  std::vector<uint32_t> m_vSSRCs;
  std::string m_sOptionalReasonForLeaving;
};

} // rfc3550
} // rtp_plus_plus
