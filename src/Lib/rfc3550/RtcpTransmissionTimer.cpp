#include "CorePch.h"
#include <rtp++/rfc3550/Rfc3550RtcpTransmissionTimer.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

Rfc3550RtcpTransmissionTimer::Rfc3550RtcpTransmissionTimer(bool bUseReducedMinimum, bool bIsMulticast)
  :m_bUseReducedMinimum(bUseReducedMinimum),
    m_bIsMulticast(bIsMulticast)
{

}

double Rfc3550RtcpTransmissionTimer::doComputeRtcpIntervalSeconds(bool bIsSender,
                                                                  uint32_t uiSenderCount,
                                                                  uint32_t uiMemberCount,
                                                                  uint32_t uiAverageRtcpSizeBytes,
                                                                  uint32_t uiSessionBandwidthKbps,
                                                                  bool bInitial )
{
  // Allow quick sync for unicast senders
  if (bInitial && bIsSender && !m_bIsMulticast) return 0.0;

  return rfc3550::computeRtcpIntervalSeconds(bIsSender, uiSenderCount, uiMemberCount,
                                             uiAverageRtcpSizeBytes, uiSessionBandwidthKbps,
                                             m_bUseReducedMinimum, bInitial, true);
}

}
}
