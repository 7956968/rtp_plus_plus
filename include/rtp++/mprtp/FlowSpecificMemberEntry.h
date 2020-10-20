#pragma once
#include <rtp++/rfc3611/XrMemberEntry.h>

namespace rtp_plus_plus
{
namespace mprtp
{

/// @def DEBUG_RTCP Additional RTCP debugging information
// #define DEBUG_RTCP

class FlowSpecificMemberEntry : public rfc3611::XrMemberEntry
{
public:
  FlowSpecificMemberEntry(uint32_t uiSSRC, uint16_t uiFlowId);
  ~FlowSpecificMemberEntry();

  // HACK from a design perspective: we could parse the RTP extension header here
  // and then process the packet according to the flow specific case. However since
  // the MpRtpMemberEntry class has already done that, we simply override the following
  // methods to do nothing.
  virtual void init(const RtpPacket& rtpPacket);

  virtual void doReceiveRtpPacket(const RtpPacket& packet);

  // overriding this purely for logging of flow
  void onRrReceived( const rfc3550::RtcpRr& rRr );

private:
  uint16_t m_uiFlowId;

};

}
}
