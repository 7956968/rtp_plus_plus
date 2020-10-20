#pragma once
#include <rtp++/RtcpTransmissionTimerBase.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

class Rfc3550RtcpTransmissionTimer : public RtcpTransmissionTimerBase
{
public:

  Rfc3550RtcpTransmissionTimer(bool bUseReducedMinimum, bool bIsMulticast = false);

private:
  virtual double doComputeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenderCount,
                                              uint32_t uiMemberCount,
                                              uint32_t uiAverageRtcpSizeBytes,
                                              uint32_t uiSessionBandwidthKbps, bool bInitial );

  // RFC3550
  bool m_bUseReducedMinimum;
  bool m_bIsMulticast;
};

}
}
