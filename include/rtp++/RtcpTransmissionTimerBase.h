#pragma once
#include <cstdint>
#include <memory>

// -DDEBUG_RTCP_SCHEDULING: logs debugging information abour report scheduling
// #define DEBUG_RTCP_SCHEDULING

namespace rtp_plus_plus
{

class RtcpTransmissionTimerBase
{
public:
  typedef std::unique_ptr<RtcpTransmissionTimerBase> ptr;

  RtcpTransmissionTimerBase();
  virtual ~RtcpTransmissionTimerBase();

  /// Method takes initial timeout calculation into account
  /// This method should be called when calculating the next RTCP interval
  double computeRtcpIntervalSeconds( bool bIsSender, uint32_t uiSenderCount, uint32_t uiMemberCount,
                                     uint32_t uiAverageRtcpSize, uint32_t uiSessionBandwidthKbps );

private:

  virtual double doComputeRtcpIntervalSeconds(bool bIsSender, uint32_t uiSenderCount,
                                              uint32_t uiMemberCount,
                                              uint32_t uiAverageRtcpSizeBytes,
                                              uint32_t uiSessionBandwidthKbps, bool bInitial) = 0;

  bool m_bInitial;
};

} // rtp_plus_plus
