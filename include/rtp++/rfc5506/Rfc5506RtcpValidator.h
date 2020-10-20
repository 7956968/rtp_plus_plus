#pragma once
#include <rtp++/rfc3550/Rfc3550RtcpValidator.h>

namespace rtp_plus_plus
{
namespace rfc5506
{

/**
 * This packetiser extends 3550 by allowing reduced size RTCP packet
 * in the RTCP validation rules
 */
class RtcpValidator : public rfc3550::RtcpValidator
{
public:
  typedef std::unique_ptr<RtcpValidator> ptr;
  /**
   * @brief Named constructor
   * @return
   */
  static RtcpValidator::ptr create();
  /**
   * @brief RtcpValidator constructor
   */
  RtcpValidator();
  /**
   * This validation omits the check for compound packets and that RTCP reports have to start with
   * an RR or SR. Instead we just check that the report falls into the RTCP range
   */
  virtual bool validateCompoundRtcpPacket(CompoundRtcpPacket compoundPacket, uint32_t uiTotalLength);

};

} // rfc5506
} // rtp_plus_plus
