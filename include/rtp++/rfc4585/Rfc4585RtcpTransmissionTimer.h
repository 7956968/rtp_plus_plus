#pragma once
#include <rtp++/RtcpTransmissionTimerBase.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

class Rfc4585TransmissionTimer : public RtcpTransmissionTimerBase
{
public:
  Rfc4585TransmissionTimer(bool bPointToPoint);
private:

  virtual double doComputeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenderCount,
                                              uint32_t uiMemberCount,
                                              uint32_t uiAverageRtcpSizeBytes,
                                              uint32_t uiSessionBandwidthKbps, bool bInitial);

private:
  bool m_bPointToPoint;
//   double m_dMin;
};

}
}
