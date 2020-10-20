#pragma once
#include <cstdint>
#include <rtp++/RtcpPacketBase.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

class RtcpValidator
{
public:
  typedef std::unique_ptr<RtcpValidator> ptr;

  virtual ~RtcpValidator(){}

  // allow subclasses to change behaviour
  /**
  * @fn  bool RtpPacketiser::validateCompoundRtcpPacket( CompoundRtcpPacket compoundPacket )
  *
  * @brief Validate compound rtcp packet.
  * 				Packets are validated according to the rules in RFC3550 and Perkins
  * 				- is compound RTCP packet
  * 				- version = 2 for all packets in compound packet
  * 				- first packet is SR or RR
  * 				- if padding is needed, it is only in last packet
  * 				- length field of individual packets must sum to overall length of RTCP packet
  *
  *
  * @param compoundPacket Message describing the compound.
  * @param uiTotalLength This parameter can be set if the packet is parsed from the network. It verifies that the total
  *                      length of the packet is equal to the sum of the lengths of the individual RTCP packets.
  * @return  true if it succeeds, false if it fails. It can be set to 0 to avoid these checks.
  */
  virtual bool validateCompoundRtcpPacket(CompoundRtcpPacket compoundPacket, uint32_t uiTotalLength);
};

}
}
