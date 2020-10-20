#include "CorePch.h"
#include <rtp++/rfc4585/Rfc4585RtcpTransmissionTimer.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

Rfc4585TransmissionTimer::Rfc4585TransmissionTimer(bool bPointToPoint)
  :m_bPointToPoint(bPointToPoint)
{

}


double Rfc4585TransmissionTimer::doComputeRtcpIntervalSeconds(bool bIsSender,
                                                              uint32_t uiSenderCount,
                                                              uint32_t uiMemberCount,
                                                              uint32_t uiAverageRtcpSizeBytes,
                                                              uint32_t uiSessionBandwidthKbps,
                                                              bool bInitial)
{
  // using standard 5% BW raction for RTCP
  // conversion to kbps = 0.05 * 125
  double dRtcpBw = uiSessionBandwidthKbps * 6.25;
  double dTransmissionInterval = rfc3550::computeDeterministicRtcpIntervalSeconds(bIsSender,
                                                                                  uiSenderCount,
                                                                                  uiMemberCount,
                                                                                  uiAverageRtcpSizeBytes,
                                                                                  dRtcpBw);

  if (bInitial && m_bPointToPoint)
  {
    if (dTransmissionInterval < 1.0) dTransmissionInterval = 1.0;
  }

  // calculate time that the next RTCP report should be sent
  dTransmissionInterval = dTransmissionInterval * rfc3550::fRand(0.5, 1.5);
  dTransmissionInterval /= 1.21828;

  // in 4585 the minimum interval is not applied
  return dTransmissionInterval;
}

} // rfc4585
} // rtp_plus_plus
