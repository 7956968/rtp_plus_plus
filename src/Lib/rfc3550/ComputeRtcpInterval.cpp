#include "CorePch.h"
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

double fRand(double fMin, double fMax)
{
  double f = (double)rand() / RAND_MAX;
  return fMin + f * (fMax - fMin);
}

/**
  * This function takes a transmission interval and adjusts it to the minimum profile specific value
  * @param dTransmissionInterval the computed transmission interval
  * @param bUseReducedMinimum whether the reduced minimum defined in RFC3550 should be used
  * @param uiSessionBandwidthKbps the RTP session bandwidth in kbps
  * @param bInitial if this is the first RTCP packet
  */
double updateIntervalAccordingToMinimum(double dTransmissionInterval,
                                               bool bUseReducedMinimum,
                                               uint32_t uiSessionBandwidthKbps,
                                               bool bInitial)
{
  double dMin = 0.0;
  if (bUseReducedMinimum)
  {
    dMin = 360.0/(uiSessionBandwidthKbps);
#ifdef DEBUG_RTCP_SCHEDULING
    DLOG(INFO) << "Reduced min: " << dMin;
#endif
  }
  else
  {
    if (bInitial)
      dMin = MINIMUM_RTCP_INTERVAL_SECONDS / 2.0;
    else
      dMin = MINIMUM_RTCP_INTERVAL_SECONDS;

#ifdef DEBUG_RTCP_SCHEDULING
    DLOG(INFO) << "Standard min: " <<  dMin;
#endif
  }
  return (dTransmissionInterval < dMin ) ? dMin : dTransmissionInterval;
}

double computeDeterministicRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders,
                                                      uint32_t uiTotalMembers,
                                                      double dAverageRtcpSize, double dRtcpBw)
{
  // This implementation of the member db also stores un-validated as well as inactive clients
  uint32_t ui25PercentOfMembers = uiTotalMembers >> 2;

  double dTransmissionInterval = 0.0;

#ifdef DEBUG_RTCP_SCHEDULING
  DLOG(INFO) << "Senders: " << uiSenders
    << " members: " << uiTotalMembers
    << " 25\%: " << ui25PercentOfMembers
    << " Avg RTCP: " << dAverageRtcpSize
    << " RTCP BW: " << dRtcpBw << " B/s";
#endif

  if (
      (uiSenders > 0) &&
      (uiSenders <= ui25PercentOfMembers)
     )
  {
    if (bIsSender)
    {
      dTransmissionInterval = (dAverageRtcpSize * uiSenders)/(dRtcpBw * 0.25);
    }
    else
    {
      dTransmissionInterval = (dAverageRtcpSize * (uiTotalMembers - uiSenders))/(dRtcpBw * 0.75);
    }
  }
  else
  {
    dTransmissionInterval = (dAverageRtcpSize * uiTotalMembers)/(dRtcpBw);
  }

#ifdef DEBUG_RTCP_SCHEDULING
  DLOG(INFO) << "Updated Interval: " << dTransmissionInterval;
#endif

  return dTransmissionInterval;
}

double computeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders,
                                         uint32_t uiTotalMembers, double dAverageRtcpSize,
                                         uint32_t uiRtcpBwKbps, bool bUseReducedMinimum,
                                         bool bInitial, bool bRandomise)
{
  // using standard 5% BW raction for RTCP
    // conversion to kbps = 0.05 * 125
  double dRtcpBw = uiRtcpBwKbps * 6.25;

  double dTransmissionInterval = computeDeterministicRtcpIntervalSeconds(bIsSender, uiSenders,
                                                                         uiTotalMembers,
                                                                         dAverageRtcpSize,
                                                                         dRtcpBw);

  // apply minimum rules
  dTransmissionInterval = updateIntervalAccordingToMinimum(dTransmissionInterval,
                                                           bUseReducedMinimum,
                                                           uiRtcpBwKbps,
                                                           bInitial);
  // apply randomisation
#ifdef DEBUG_RTCP_SCHEDULING
  DLOG(INFO) << "Calculated Interval: " << dTransmissionInterval;
#endif
  // calculate time that the next RTCP report should be sent
  if (bRandomise)
    dTransmissionInterval = dTransmissionInterval * fRand(0.5, 1.5);

  dTransmissionInterval /= 1.21828;
#ifdef DEBUG_RTCP_SCHEDULING
  DLOG(INFO) << "Randomised Interval: " << dTransmissionInterval;
#endif

  // halve interval for first packet
  if (bInitial)
  {
    dTransmissionInterval *= 0.5;
#ifdef DEBUG_RTCP_SCHEDULING
    DLOG(INFO) << "First RTCP Interval: " << dTransmissionInterval;
#endif
  }

  return dTransmissionInterval;
}

}
}
