#pragma once
#include <cpputil/BitReader.h>
#include <cpputil/BitWriter.h>
#include <cpputil/IBitStream.h>
#include <rtp++/rfc5285/HeaderExtensionElement.h>
#include <rtp++/mprtp/MpRtpHeader.h>

namespace rtp_plus_plus
{
namespace mprtp
{

/**
 * @brief The MpRtpHeaderParseResult struct is a generic struct that can store either an
 * MPRTP subflow header or an MPRTP connectivity check.
 */
struct MpRtpHeaderParseResult
{
public:
  enum HeaderType
  {
    MPRTP_SUBFLOW_HEADER,
    MPRTP_CONNECTIVITY_CHECK,
    MPRTP_INVALID
  };

  MpRtpHeaderParseResult()
    :m_eType(MPRTP_INVALID)
  {

  }

  MpRtpHeaderParseResult(const MpRtpSubflowRtpHeader& pSubflowHeader)
    :m_eType(MPRTP_SUBFLOW_HEADER),
      SubflowHeader(boost::optional<MpRtpSubflowRtpHeader>(pSubflowHeader))
  {

  }

  MpRtpHeaderParseResult(const MpRtpConnectivityCheck& pConnectivityCheck)
    :m_eType(MPRTP_SUBFLOW_HEADER),
      ConnectivityCheck(boost::optional<MpRtpConnectivityCheck>(pConnectivityCheck))
  {

  }

  HeaderType m_eType;
  boost::optional<MpRtpSubflowRtpHeader> SubflowHeader;
  boost::optional<MpRtpConnectivityCheck> ConnectivityCheck;
};

/**
 * @brief The MpRtpExtensionHeader class
 */
class MpRtpHeaderExtension
{
public:

  static rfc5285::HeaderExtensionElement generateMpRtpHeaderExtension(uint32_t uiMpRtpExtmapId, const MpRtpSubflowRtpHeader& subflowHeader)
  {
    const int SUBFLOW_HEADER_LENGTH = 5;
    uint8_t data[SUBFLOW_HEADER_LENGTH];
    BitWriter writer(data, SUBFLOW_HEADER_LENGTH);
    bool bRes = writer.write(MPID_SUBFLOW_RTP_HEADER, 4);
    assert(bRes);
    bRes = writer.write(MPRTP_SUBFLOW_RTP_HEADER_LENGTH, 4);
    assert(bRes);
    bRes = writer.write(subflowHeader.getFlowId(), 16);
    assert(bRes);
    bRes = writer.write(subflowHeader.getFlowSpecificSequenceNumber(), 16);
    assert(bRes);
    return rfc5285::HeaderExtensionElement(uiMpRtpExtmapId, MPRTP_SUBFLOW_RTP_HEADER_LENGTH + 1, data);
  }

  static MpRtpHeaderParseResult parseMpRtpHeader(const rfc5285::HeaderExtensionElement& headerExtension)
  {
    BitReader reader(headerExtension.getExtensionData(), headerExtension.getDataLength() + 1);
    uint32_t uiType = 0;
    uint32_t uiLen = 0;

    if (!reader.read(uiType, 4)) return MpRtpHeaderParseResult();
    if (!reader.read(uiLen, 4)) return MpRtpHeaderParseResult();
    switch (uiType)
    {
      case MPID_SUBFLOW_RTP_HEADER:
      {
        uint16_t uiId = 0;
        uint16_t uiFssn = 0;
        if (!reader.read(uiId, 16)) return MpRtpHeaderParseResult();
        if (!reader.read(uiFssn, 16)) return MpRtpHeaderParseResult();
        return MpRtpHeaderParseResult(MpRtpSubflowRtpHeader(uiId, uiFssn));
      }
      case MPID_CONNECTIVITY_CHECK:
      {
        return MpRtpHeaderParseResult(MpRtpConnectivityCheck());
      }
      default:
      {
        LOG(WARNING) << "Unknown MPRTP extension header type";
        return MpRtpHeaderParseResult();
      }
    }
  }
};

} // mprtp
} // rtp_plus_plus

