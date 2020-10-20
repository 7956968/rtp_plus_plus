#pragma once

#include <rtp++/rfc3550/RtpConstants.h>

namespace rfc3550
{

static double fRand(double fMin, double fMax)
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
static double updateIntervalAccordingToMinimum(double dTransmissionInterval, bool bUseReducedMinimum, uint32_t uiSessionBandwidthKbps, bool bInitial)
{
  double dMin = 0.0;
  if (bUseReducedMinimum)
  {
    dMin = 360/(uiSessionBandwidthKbps);
#ifdef DEBUG_RTCP_SCHEDULING
    DLOG(INFO) << "Reduced min: " << dMin;
#endif
  }
  else
  {
    if (bInitial)
      dMin = MINIMUM_RTCP_INTERVAL_SECONDS / 2;
    else
      dMin = MINIMUM_RTCP_INTERVAL_SECONDS;

#ifdef DEBUG_RTCP_SCHEDULING
    DLOG(INFO) << "Standard min: " <<  dMin;
#endif
  }
  return (dTransmissionInterval < dMin ) ? dMin : dTransmissionInterval;
}


/**
  * returns the deterministic reporting interval i.e. without randomisation and adjusted according
  * to the minimum allowed value
  * @param bIsSender whether the particpant is a sender
  * @param uiSenders the number of senders in the RTP session
  * @param uiTotalMembers the number of active members in the RTP session
  * @param dAverageRtcpSize the average size of RTCP packets in bytes computed over time
  * @param dRtcpBw is measured in octets per second
  */
static double computeDeterministicRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders, uint32_t uiTotalMembers, double dAverageRtcpSize, double dRtcpBw)
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

/**
  * This function computes the randomised RTCP transmission interval in seconds
  * The value has NOT been adjusted according to the minimum allowed value
  * @param bIsSender whether the particpant is a sender
  * @param uiSenders the number of senders in the RTP session
  * @param uiTotalMembers the number of active members in the RTP session
  * @param dAverageRtcpSize the average size of RTCP packets in bytes computed over time
  * @param uiRtcpBwKbps is measured in Kb per second
  * @param bUseReducedMinimum indicates if the reduced minimum option of RFC3550 should be used
  * @param bInitial if this the first RTCP packet
  */
static double computeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders, uint32_t uiTotalMembers, double dAverageRtcpSize, uint32_t uiRtcpBwKbps, bool bUseReducedMinimum, bool bInitial, bool bRandomise)
{
  // using standard 5% BW raction for RTCP
    // conversion to kbps = 0.05 * 125
  double dRtcpBw = uiRtcpBwKbps * 6.25;

  double dTransmissionInterval = computeDeterministicRtcpIntervalSeconds(bIsSender, uiSenders, uiTotalMembers, dAverageRtcpSize, dRtcpBw);

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
