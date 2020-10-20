#pragma once
#include <cpputil/Conversion.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4585/Rfc4585.h>

namespace rtp_plus_plus
{

static void upgradeProfileIfNecessary(std::string& sProfile, const std::vector<std::string> RtcpFb)
{
  LOG(INFO) << "RTP profile to " << sProfile << " FB: " << ::toString(RtcpFb);
  std::transform(sProfile.begin(), sProfile.end(), sProfile.begin(), ::toupper);
  if (sProfile == rfc3550::AVP)
  {
    // simple implementation without checking validity of fb message
    if (!RtcpFb.empty())
    {
      LOG(INFO) << "Upgrading RTP profile to " << rfc4585::AVPF << " for FB: " << ::toString(RtcpFb);
      sProfile = rfc4585::AVPF;
    }
  }
}

/**
 * The one way RTT calculation only works assuming that all machines are synchronised
 * with the same NTP server
 *
 * DEPRECATED: now using RFC6051 for OWD calculation
 */
static double extractRTTFromRtpPacket(const RtpPacket& rtpPacket)
{
  uint8_t* pData = const_cast<uint8_t*>(rtpPacket.getPayload().data());
  if ((pData[0] == 'n') &&
      (pData[1] == 't') &&
      (pData[2] == 'p') &&
      (pData[3] == ':'))
  {
    uint32_t uiMswSample = 0, uiLswSample = 0;
    memcpy(&uiMswSample, &pData[4], sizeof(uiMswSample));
    memcpy(&uiLswSample, &pData[8], sizeof(uiLswSample));
    uiMswSample = ntohl(uiMswSample);
    uiLswSample = ntohl(uiLswSample);
    uint64_t uiNtpSample = ((uint64_t)uiMswSample << 32) | uiLswSample;
    // NTP arrival timestamp of RTP packet
    uint64_t uiNtpArrival = rtpPacket.getNtpArrivalTime();
    // for very small RTTs this can be negative
    int64_t iDiff = uiNtpArrival - uiNtpSample;
    uint64_t uiDiff = (iDiff > 0) ? uiNtpArrival - uiNtpSample : 0;
    uint32_t uiSeconds = (uiDiff >> 32);
    uint32_t uiMicroseconds = (uiDiff & 0xFFFFFFFF);
    double dMicroseconds = (uiMicroseconds/(double)UINT_MAX);
#if 0
    VLOG(2) << "SN: " << rtpPacket.getSequenceNumber() << " NTP NOW: " << hex((uint32_t)(uiNtpArrival >> 32)) << "." << hex((uint32_t)uiNtpArrival)
            << " Sample: " << hex(uiMswSample) << "." << hex(uiLswSample)
            << " One way RTT: " << uiSeconds + dMicroseconds << "s" ;
#endif
    return uiSeconds + dMicroseconds;
  }
  return -1.0;
}

} // rtp_plus_plus
