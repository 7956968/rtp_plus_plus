#pragma once
#include <vector>
#include <cpputil/Buffer.h>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc3611/Rfc3611.h>

namespace rtp_plus_plus
{
namespace rfc3611
{

enum BlockType
{
  XR_RECEIVER_REFERENCE_TIME_BLOCK = 4,
  XR_DLRR_BLOCK = 5
};

const int XR_RECEIVER_REFERENCE_TIME_BLOCK_LENGTH = 2;
const int XR_DLRR_SUBBLOCK_LENGTH = 3;

struct DlrrBlock
{
    DlrrBlock()
      :SSRC(0),
      LastRR(0),
      DLRR(0)
    {

    }

    DlrrBlock(uint32_t uiSSRC, uint32_t uiLastRR, uint32_t uiDlrr)
      :SSRC(uiSSRC),
      LastRR(uiLastRR),
      DLRR(uiDlrr)
    {

    }

    uint32_t SSRC;
    uint32_t LastRR;
    uint32_t DLRR;
};

struct ReceiverReferenceTimeReportBlock
{
  ReceiverReferenceTimeReportBlock()
    :NtpTimestampMsw(0),
    NtpTimestampLsw(0)
  {

  }
  ReceiverReferenceTimeReportBlock(uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw)
    :NtpTimestampMsw(uiNtpTimestampMsw),
    NtpTimestampLsw(uiNtpTimestampLsw)
  {

  }

  uint32_t NtpTimestampMsw;
  uint32_t NtpTimestampLsw;
};

/**
  * This XR block class handles XR reports in a generic way. The block specific content is stored as a blob,
  * which means that the application layer can handle this as required. This prevents us from having to change
  * the implementation at the lower down level, when creating new block types
  */
class RtcpXrReportBlock
{
//  friend std::ostream& operator<< (std::ostream& stream, const RtcpRrReportBlock& reportBlock);
public:

  RtcpXrReportBlock()
    :m_uiBlockType(0),
    m_uiTypeSpecific(0),
    m_uiBlockLength(0)
  {

  }

  RtcpXrReportBlock(uint8_t uiBlockType, uint8_t uiTypeSpecific)
    :m_uiBlockType(uiBlockType),
    m_uiTypeSpecific(uiTypeSpecific),
    m_uiBlockLength(0)
  {

  }

  RtcpXrReportBlock(uint8_t uiBlockType, uint8_t uiTypeSpecific, uint16_t uiBlockLength)
    :m_uiBlockType(uiBlockType),
    m_uiTypeSpecific(uiTypeSpecific),
    m_uiBlockLength(uiBlockLength)
  {

  }

  uint8_t getBlockType() const { return m_uiBlockType; }
  void setBlockType(uint8_t val) { m_uiBlockType = val; }
  uint8_t getTypeSpecific() const { return m_uiTypeSpecific; }
  void setTypeSpecific(uint8_t val) { m_uiTypeSpecific = val; }
  uint16_t getBlockLength() const
  {
    uint32_t uiLen = m_rawXrBlockContent.getSize() >> 2;
    return static_cast<uint16_t>(uiLen);
  }
  void setBlockLength(uint16_t val) { m_uiBlockLength = val; }
  Buffer getRawXrBlockContent() const { return m_rawXrBlockContent; }
  void setRawXrBlockContent(Buffer rawXrBlockContent) { m_rawXrBlockContent = rawXrBlockContent; }
  // Bitstream methods
  void write(OBitStream& ob) const
  {
    ob.write(m_uiBlockType,           8);
    ob.write(m_uiTypeSpecific,        8);
    // length in words
    uint32_t uiLen = m_rawXrBlockContent.getSize() >> 2;
    ob.write(uiLen, 16);
    const uint8_t* pData = m_rawXrBlockContent.data();
    ob.writeBytes(pData, m_rawXrBlockContent.getSize());
  }
  void read(IBitStream& ib)
  {
    ib.read(m_uiBlockType,         8);
    ib.read(m_uiTypeSpecific,      8);
    ib.read(m_uiBlockLength,      16);
    uint32_t uiLen = m_uiBlockLength << 2;
    m_rawXrBlockContent.setData(new uint8_t[uiLen], uiLen);
    uint8_t* pData = const_cast<uint8_t*>(m_rawXrBlockContent.data());
    ib.readBytes(pData, uiLen);
  }

private:
  uint8_t  m_uiBlockType;
  uint8_t  m_uiTypeSpecific;
  // The length of this report block, including the header, in 32-
  // bit words minus one.  If the block type definition permits,
  // zero is an acceptable value, signifying a block that consists
  // of only the BT, type-specific, and block length fields, with a
  // null type-specific block contents field.
  uint16_t m_uiBlockLength;
  Buffer   m_rawXrBlockContent;
};

class RtcpXr : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpXr> ptr;

  static ptr create()
  {
    return boost::make_shared<RtcpXr>();
  }

  static ptr create( uint32_t uiSSRC )
  {
    return boost::make_shared<RtcpXr>(uiSSRC);
  }

  static ptr create( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
  {
    return boost::make_shared<RtcpXr>(uiVersion, uiPadding, uiTypeSpecific, uiLengthInWords);
  }

  RtcpXr()
    :RtcpPacketBase(PT_RTCP_XR),
    m_uiReporterSSRC(0)
  {
    // update length field
    setLength(BASIC_XR_LENGTH);
  }

  RtcpXr( uint32_t uiSSRC )
    :RtcpPacketBase(PT_RTCP_XR),
    m_uiReporterSSRC(uiSSRC)
  {
    // update length field
    setLength(BASIC_XR_LENGTH);
  }

  RtcpXr( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
    :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, PT_RTCP_XR, uiLengthInWords),
    m_uiReporterSSRC(0)
  {

  }

  uint32_t getReporterSSRC() const { return m_uiReporterSSRC; }
  void setReporterSSRC(uint32_t val) { m_uiReporterSSRC = val; }

  void addReportBlock(RtcpXrReportBlock reportBlock)
  {
    m_vReports.push_back(reportBlock);
    // update RC field in base RTCP packet
    setTypeSpecific(m_vReports.size());
    // update length field
    // TODO: could optimize this by just adding the length of the new block
    uint32_t uiBlockLength = 0;
    for (size_t i = 0; i < m_vReports.size(); ++i)
    {
      // the +1 is for the block header which isn't included in the getBlockLength call
      uiBlockLength += m_vReports[i].getBlockLength() + 1;
    }
    setLength(BASIC_XR_LENGTH + uiBlockLength);
  }

  std::vector<RtcpXrReportBlock> getXrReportBlocks() const
  {
    return m_vReports;
  }

protected:
  void writeTypeSpecific(OBitStream& ob) const
  {
    ob.write(m_uiReporterSSRC,                  32);
    std::for_each(m_vReports.begin(), m_vReports.end(), [&ob](RtcpXrReportBlock report)
    {
      report.write(ob);
    }
    );
  }

  void readTypeSpecific(IBitStream& ib)
  {
    ib.read(m_uiReporterSSRC,                  32);
    // this assumes that the
    // RtcpXr( uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint16_t uiLengthInWords )
    // constructor was used. Compensate for the SSRC field by subtracting RFC3550_BASIC_XR_LENGTH
    uint32_t uilengthRemaining = getLength() - BASIC_XR_LENGTH;
    for (size_t i = 0; i < uilengthRemaining; ++i)
    {
      RtcpXrReportBlock reportBlock;
      reportBlock.read(ib); // need to add validation to ib!!!!
      m_vReports.push_back(reportBlock);
      uilengthRemaining -= (reportBlock.getBlockLength() + 1);
    }
  }

private:
  uint32_t m_uiReporterSSRC;
  std::vector<RtcpXrReportBlock> m_vReports;
};

}
}
