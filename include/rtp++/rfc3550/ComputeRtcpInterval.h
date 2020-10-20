#pragma once

namespace rtp_plus_plus
{
namespace rfc3550
{

/**
  * Utility function to generate random double in range
  */
extern double fRand(double fMin, double fMax);

/**
  * returns the deterministic reporting interval i.e. without randomisation and adjusted according
  * to the minimum allowed value
  * @param bIsSender whether the particpant is a sender
  * @param uiSenders the number of senders in the RTP session
  * @param uiTotalMembers the number of active members in the RTP session
  * @param dAverageRtcpSize the average size of RTCP packets in bytes computed over time
  * @param dRtcpBw is measured in octets per second
  */
extern double computeDeterministicRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders,
                                                      uint32_t uiTotalMembers,
                                                      double dAverageRtcpSize, double dRtcpBw);

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
extern double computeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenders,
                                         uint32_t uiTotalMembers, double dAverageRtcpSize,
                                         uint32_t uiRtcpBwKbps, bool bUseReducedMinimum,
                                         bool bInitial, bool bRandomise);

}
}
