#include "CorePch.h"
#include <rtp++/RtcpTransmissionTimerBase.h>

// -DDEBUG_RTCP_SCHEDULING: logs debugging information abour report scheduling
// #define DEBUG_RTCP_SCHEDULING
namespace rtp_plus_plus
{

RtcpTransmissionTimerBase::RtcpTransmissionTimerBase()
  :m_bInitial(true)
{
}

RtcpTransmissionTimerBase::~RtcpTransmissionTimerBase()
{
}

/// Method takes initial timeout calculation into account
/// This method should be called when calculating the next RTCP interval
double RtcpTransmissionTimerBase::computeRtcpIntervalSeconds( bool bIsSender,
                                                              uint32_t uiSenderCount,
                                                              uint32_t uiMemberCount,
                                                              uint32_t uiAverageRtcpSize,
                                                              uint32_t uiSessionBandwidthKbps )
{
  double dInterval = doComputeRtcpIntervalSeconds(bIsSender, uiSenderCount, uiMemberCount,
                                                  uiAverageRtcpSize, uiSessionBandwidthKbps,
                                                  m_bInitial);
  if (m_bInitial) m_bInitial = false;
#ifdef DEBUG_RTCP_SCHEDULING
  VLOG(5) << "RTCP Interval: " << dInterval << " s";
#endif
  return dInterval;
}

}
