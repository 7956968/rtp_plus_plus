#include "CorePch.h"
#include <rtp++/rfc6051/Rfc6051.h>

namespace rtp_plus_plus
{
namespace rfc6051
{

const std::string EXTENSION_NAME_RTP_NTP_64 = "urn:ietf:params:rtp-hdrext:ntp-64";
const std::string EXTENSION_NAME_RTP_NTP_56 = "urn:ietf:params:rtp-hdrext:ntp-56";

rfc5285::HeaderExtensionElement RtpSynchronisationExtensionHeader::create(uint32_t uiRtpSyncExtMapId, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw)
{
  const uint32_t uiBytes = RTP_SYNC_HEADER_EXT_64_LENGTH;
  uint8_t data[uiBytes];
  BitWriter writer(data, uiBytes);
  bool bRes = writer.write(uiNtpTimestampMsw, 32);
  assert(bRes);
  bRes = writer.write(uiNtpTimestampLsw, 32);
  assert(bRes);
  return rfc5285::HeaderExtensionElement(uiRtpSyncExtMapId, RTP_SYNC_HEADER_EXT_64_LENGTH, data);
}

rfc5285::HeaderExtensionElement RtpSynchronisationExtensionHeader::create(uint32_t uiRtpSyncExtMapId, uint64_t uiNtpTimestamp)
{
  uint32_t uiNtpTimestampMsw = 0;
  uint32_t uiNtpTimestampLsw = 0;
  RtpTime::split(uiNtpTimestamp, uiNtpTimestampMsw, uiNtpTimestampLsw);
  return create(uiRtpSyncExtMapId, uiNtpTimestampMsw, uiNtpTimestampLsw);
}

boost::optional<uint64_t> RtpSynchronisationExtensionHeader::parseRtpSynchronisationExtensionHeader(const rfc5285::HeaderExtensionElement& headerExtension)
{
  BitReader reader(headerExtension.getExtensionData(), headerExtension.getDataLength());

  if (headerExtension.getDataLength() == RTP_SYNC_HEADER_EXT_64_LENGTH)
  {
    uint32_t uiNtpMsw = 0;
    uint32_t uiNtpLsw = 0;
    if (!reader.read(uiNtpMsw, 32)) return boost::optional<uint64_t>();
    if (!reader.read(uiNtpLsw, 32)) return boost::optional<uint64_t>();
    return boost::optional<uint64_t>(RtpTime::join(uiNtpMsw, uiNtpLsw));
  }
  else if (headerExtension.getDataLength() == RTP_SYNC_HEADER_EXT_56_LENGTH)
  {
    LOG(WARNING) << "56-bit RTP synchronisation extension header unsupported for now";
    return boost::optional<uint64_t>();
  }
  else
  {
    LOG(WARNING) << "Unknown RTP synchronisation extension header length";
    return boost::optional<uint64_t>();
  }
}

} // rfc6051
} // rtp_plus_plus
