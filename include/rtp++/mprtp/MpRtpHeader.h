#pragma once
#include <cstdint>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/rfc5285/RtpHeaderExtension.h>

namespace rtp_plus_plus
{
namespace mprtp
{

const uint8_t  MPRTP_SUBFLOW_RTP_HEADER_LENGTH = 4;
const uint8_t  MPRTP_CONNECTIVITY_CHECK_LENGTH = 0;

const uint8_t   MPID_SUBFLOW_RTP_HEADER = 0x00; // length = 6
const uint8_t   MPID_CONNECTIVITY_CHECK = 0x01; // length = 0

/**
 * @brief The MpRtpSubflowRtpHeader class
 */
class MpRtpSubflowRtpHeader
{
public:

  MpRtpSubflowRtpHeader()
    :m_uiSubflowId(0),
      m_uiSubflowSpecificSN(0)
  {
  }

  MpRtpSubflowRtpHeader(uint16_t uiSubflowId, uint16_t uiSubflowSpecificSN)
    :m_uiSubflowId(uiSubflowId),
      m_uiSubflowSpecificSN(uiSubflowSpecificSN)
  {
  }

  uint16_t getFlowId() const { return m_uiSubflowId; }
  void setFlowId(uint16_t uiFlowId) { m_uiSubflowId = uiFlowId; }

  uint16_t getFlowSpecificSequenceNumber() const { return m_uiSubflowSpecificSN; }
  void setFlowSpecificSequenceNumber(uint16_t uiSubflowSpecificSN) { m_uiSubflowSpecificSN = uiSubflowSpecificSN; }

private:
  uint16_t m_uiSubflowId;
  uint16_t m_uiSubflowSpecificSN;
};

/**
 * @brief The MpRtpConnectivityCheck class
 */
class MpRtpConnectivityCheck
{
public:
  MpRtpConnectivityCheck()
  {
  }
};

} // mprtp
} // rtp_plus_plus
