#pragma once
#include <cstdint>
#include <vector>
#include <rtp++/network/Sctp.h>

namespace rtp_plus_plus
{
namespace sctp
{

/// DEPRECATED: refactored into SctpRtpPolicy
///
struct SctpChannelPolicy
{
  bool Unordered;
  PrPolicy PrRtxPolicy;
  uint32_t PrValue;
  double RttFactor;

  SctpChannelPolicy()
    :Unordered(false),
      PrRtxPolicy(SCTP_PR_POLICY_SCTP_NONE),
      PrValue(0),
      RttFactor(-1.0)
  {

  }

  SctpChannelPolicy(bool bUnordered, PrPolicy ePolicy, uint32_t uiPrValue, double dRttFactor = -1.0)
    :Unordered(bUnordered),
      PrRtxPolicy(ePolicy),
      PrValue(uiPrValue),
      RttFactor(dRttFactor)
  {

  }
};

// NOTE: convention for now is that channel 0 is RTCP, channel 1 is RTP
// If no channels have been configured, we manually create them
class SctpRtpChannelDescriptor
{
public:
  void addChannel(const SctpChannelPolicy& policy)
  {
    m_vChannelPolicies.push_back(policy);
  }

  /**
    * Retrieves the policy list for the SCTP channel
    * WARNING: this method creates default policies if none
    * have been added by this point.
    */
  const std::vector<SctpChannelPolicy>& getPolicies() const
  {
    // if no policies have been configured we manually create
    // the default RTCP and RTP channels
    if (m_vChannelPolicies.empty())
    {
      // RTCP
      LOG(WARNING) << "NO RTCP policy configured: creating default RTCP policy";
      m_vChannelPolicies.push_back(sctp::SctpChannelPolicy(false, sctp::SCTP_PR_POLICY_SCTP_NONE, 0));
    }

    if (m_vChannelPolicies.size() == 1)
    {
      // RTP
      LOG(WARNING) << "NO RTP policy configured: creating default RTP policy";
      m_vChannelPolicies.push_back(sctp::SctpChannelPolicy(false, sctp::SCTP_PR_POLICY_SCTP_RTX, 0));
    }

    return m_vChannelPolicies;
  }
   private:
  mutable std::vector<SctpChannelPolicy> m_vChannelPolicies;
};

}

}
