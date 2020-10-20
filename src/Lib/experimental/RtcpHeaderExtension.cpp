#include "CorePch.h"
#include <rtp++/experimental/RtcpHeaderExtension.h>

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus {
namespace experimental {

const std::string RTCP_RTP_HEADER_EXT = "rtcp-rtp-header-ext";
const std::string EXTENSION_NAME_RTCP_HEADER_EXT = "urn:ietf:params:rtp-hdrext:rtcp";

boost::optional<rfc5285::HeaderExtensionElement> RtcpHeaderExtension::create(uint32_t uiRtcpExtMapId, const CompoundRtcpPacket& vCompoundRtcp )
{
  // get total length of extension
  uint32_t uiLength = 0;
  for (const RtcpPacketBase::ptr& pRtcp : vCompoundRtcp)
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Adding rtcp packet of length " << pRtcp->getLength();
    uiLength += (4 * pRtcp->getLength()) + 4;
  }
  if (uiLength == 0)
  {
    VLOG(2) << "RTCP packet length 0 or too long for one byte RTP header extension: " << uiLength;
    return boost::optional<rfc5285::HeaderExtensionElement>();
  }

  Buffer extensionData = RtpPacketiser::packetise(vCompoundRtcp);
  rfc5285::HeaderExtensionElement ext(uiRtcpExtMapId, extensionData.getSize(), extensionData.data());
  return boost::optional<rfc5285::HeaderExtensionElement>(ext);
}

CompoundRtcpPacket RtcpHeaderExtension::parse(const rfc5285::HeaderExtensionElement& headerExtension, RtpPacketiser& rtpPacketiser)
{
  uint32_t uiLen = headerExtension.getDataLength();
  VLOG(COMPONENT_LOG_LEVEL) << "Parsing extension of length " << uiLen;
  uint8_t* pData = const_cast<uint8_t*>(headerExtension.getExtensionData());
  // make a copy for Buffer to manage
  uint8_t* pCopy = new uint8_t[uiLen];
  memcpy(pCopy, headerExtension.getExtensionData(), uiLen);
  Buffer buf(pCopy, uiLen);
  CompoundRtcpPacket rtcp = rtpPacketiser.depacketiseRtcp(buf);
  VLOG(COMPONENT_LOG_LEVEL) << "Parsed " << rtcp.size() << " packets";
  return rtcp;
}

} // experimental
} // rtp_plus_plus
