#include "CorePch.h"
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace rfc5506
{

#ifndef EXIT_ON_TRUE
#define EXIT_ON_TRUE(condition, log_message) if ((condition)){\
  LOG(WARNING) << log_message;return false;}
#endif

RtcpValidator::ptr RtcpValidator::create()
{
  return std::unique_ptr<RtcpValidator>( new RtcpValidator() );
}

RtcpValidator::RtcpValidator()
{
  VLOG(COMPONENT_LOG_LEVEL) << "rfc5506::RtcpValidator()";
}

bool RtcpValidator::validateCompoundRtcpPacket(CompoundRtcpPacket compoundPacket, uint32_t uiTotalLength)
{
  EXIT_ON_TRUE(compoundPacket.empty(), "Empty compound RTCP packet");

  // check versions
  for (size_t i = 0; i < compoundPacket.size(); ++i)
  {
    EXIT_ON_TRUE((compoundPacket[i]->getVersion() != rfc3550::RTP_VERSION_NUMBER), "Invalid version number");
  }

  // check if first packet falls into RTCP range
  EXIT_ON_TRUE((compoundPacket[0]->getPacketType() < rfc3550::PT_RTCP_SR),
               "First packet is no valid RTCP packet");

  // check padding
  for (size_t i = 0; i < compoundPacket.size() - 1; ++i)
  {
    EXIT_ON_TRUE((compoundPacket[i]->getPadding()), "Padding before last RTCP packet");
  }

  // commenting out this check: it causes validation to fail when one or more RTCP packets
  // could not be parsed (e.g. due to unknown RTCP packet types)
  // This should not cause the validation to fail
#if 0
  if (uiTotalLength > 0)
  {
    // check total length: (add lengths + number of packets (for common header)) * 4 (measured in words)
    uint32_t uiSumLengths = (std::accumulate(compoundPacket.begin(),
      compoundPacket.end(),
      compoundPacket.size(),
      [](int iLength, RtcpPacketBase::ptr pPacket){ return iLength + pPacket->getLength();})) * 4;
#if 0
    DLOG(INFO) << "Sum1: " << uiSumLengths << " Sum2: " << uiSumLengths2;
#endif
    EXIT_ON_TRUE(uiSumLengths != uiTotalLength, "RTCP lengths don't match packet length");
  }
#endif

  return true;
}

} // rfc5506
} // rtp_plus_plus
