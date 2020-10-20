#include "CorePch.h"
#include <rtp++/rfc3550/Rfc3550RtcpValidator.h>

#if 0
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif

namespace rtp_plus_plus
{
namespace rfc3550
{

#define EXIT_ON_TRUE(condition, log_message) if ((condition)){\
  LOG(WARNING) << log_message;return false;}

bool RtcpValidator::validateCompoundRtcpPacket(CompoundRtcpPacket compoundPacket, uint32_t uiTotalLength)
{
  EXIT_ON_TRUE(compoundPacket.empty(), "Empty compound RTCP packet");

  // check if is compound packet
  if (uiTotalLength != 0)
      EXIT_ON_TRUE((((uint32_t)compoundPacket[0]->getLength() + 1) << 2) == uiTotalLength, "No compound RTCP packet");

  // check versions
  for (size_t i = 0; i < compoundPacket.size(); ++i)
  {
    EXIT_ON_TRUE((compoundPacket[i]->getVersion() != RTP_VERSION_NUMBER), "Invalid version number");
  }

  // check if first packet is SR or RR
  EXIT_ON_TRUE((compoundPacket[0]->getPacketType() != PT_RTCP_SR) &&
    (compoundPacket[0]->getPacketType() != PT_RTCP_RR), 
    "First packet is no SR/RR: " << (uint32_t)compoundPacket[0]->getPacketType());

  // check padding
  for (size_t i = 0; i < compoundPacket.size() - 1; ++i)
  {
    EXIT_ON_TRUE((compoundPacket[i]->getPadding()), "Padding before last RTCP packet");
  }

  // commenting out this check: it causes validation to fail when one or more RTCP packets
  // could not be parsed (e.g. due to unknown RTCP packet types)
  // This should not cause the validation to fail
#if 0
  if (uiTotalLength != 0)
  {
    // check total length: (add lengths + number of packets (for common header)) * 4 (measured in words)
#if 0
    // with boost lambda
    uint32_t uiSumLengths = ((std::accumulate(compoundPacket.begin(),
      compoundPacket.end(),
      0,
      boost::lambda::_1 + boost::lambda::bind(&RtcpPacketBase::getLength, *boost::lambda::_2))
      ) + compoundPacket.size()) * 4;
#endif
    uint32_t uiSumLengths = (std::accumulate(compoundPacket.begin(),
      compoundPacket.end(),
      compoundPacket.size(),
      [](int iLength, RtcpPacketBase::ptr pPacket){ return iLength + pPacket->getLength();}))*4;

      EXIT_ON_TRUE(uiSumLengths != uiTotalLength, "RTCP lengths don't match packet length: " << uiSumLengths << "!=" << uiTotalLength);
  }
#endif

  return true;
}

}
}
