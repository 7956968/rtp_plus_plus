#include "CorePch.h"
#include <rtp++/sctp/SctpRtpPolicy.h>
#include <cpputil/Conversion.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>

namespace rtp_plus_plus
{
namespace sctp
{

const int32_t DEFAULT_RTCP = 0;
const int32_t DEFAULT_RTP = 1;

const int32_t TYPE_NOT_SET = -1;
const int32_t FRAME_TYPE_I = 0;
const int32_t FRAME_TYPE_P = 1;
const int32_t FRAME_TYPE_B = 2;
const int32_t FRAME_TYPE_NALU = 3;

const int32_t LAYER_TYPE_BL = 0;
const int32_t LAYER_TYPE_EL = 1;
const int32_t LAYER_TYPE_NALU = 2;

PolicyDescriptor::PolicyDescriptor()
  :Valid(false),
    Channel_Id(0),
    Protocol(PT_UNKNOWN),
    Media(MT_UNKNOWN),
    Layer(TYPE_NOT_SET),
    FrameType(TYPE_NOT_SET),
    D_L_ID(TYPE_NOT_SET),
    Q_T_ID(TYPE_NOT_SET),
    T_ID(TYPE_NOT_SET),
    RttFactor(-1.0)
{

}

const std::string SctpRtpPolicy::DefaultRtcpPolicy = "RTCP:1:3:0";
const std::string SctpRtpPolicy::DefaultRtpPolicy =  "RTP:*:1:3:0";
const uint32_t SctpRtpPolicy::RtcpPolicyId = 0;
const uint32_t SctpRtpPolicy::RtpPolicyId = 1;

boost::optional<SctpRtpPolicy> SctpRtpPolicy::create(const std::vector<std::string>& vPolicyVector)
{
  SctpRtpPolicy sctpPolicy;
  // parse each string
  for (const std::string& sPolicy: vPolicyVector)
  {
    PolicyDescriptor policy = parsePolicy(sPolicy);
    if (policy.Valid)
    {
      if (policy.Protocol == PT_RTCP)
      {
        // update RTCP policy
        sctpPolicy.setRtcpPolicy(policy);
        VLOG(2) << "Updated RTCP policy: " << sPolicy;
      }
      else
      {
        if (policy.Layer == TYPE_NOT_SET)
        {
          // update RTP policy
          sctpPolicy.setRtpPolicy(policy);
          VLOG(2) << "Updated RTP policy: " << sPolicy;
        }
        else
        {
          sctpPolicy.addPolicy(policy);
          VLOG(2) << "Added additional RTP policy: " << sPolicy;
        }
      }
    }
    else
    {
      return boost::optional<SctpRtpPolicy>();
    }
  }
  return boost::optional<SctpRtpPolicy>(sctpPolicy);
}

SctpRtpPolicy SctpRtpPolicy::createDefaultPolicy()
{
  return SctpRtpPolicy();
}

void SctpRtpPolicy::addPolicy(PolicyDescriptor policy)
{
  if (policy.Protocol == PT_RTCP)
  {
    m_rtcpPolicy = policy;
    m_rtcpPolicy.Channel_Id = 0;
  }
  else
  {
    // check if we have a matching policy already
//    uint32_t uiChannel = 0;
    // we can use zero as the default value since
    // it is reserved for RTCP
//    for (size_t i = 0; i < m_vSctpPolicies.size(); ++i)
//    {
//      if (std::get<0>(m_vSctpPolicies[i]) == policy.Unordered &&
//          std::get<1>(m_vSctpPolicies[i]) == policy.SctpPrPolicy &&
//          std::get<2>(m_vSctpPolicies[i]) == policy.PrVal)
//      {
//        // match: the index tell us the channel id
//        uiChannel = i + 1;
//      }
//    }

//    if (uiChannel != 0)
//    {
//      // matching channel exists
//      VLOG(2) << "Matching channel exists: " << uiChannel;
//      policy.Channel_Id =  uiChannel;
//    }
//    else
//    {
//      // new policy
//      m_vSctpPolicies.push_back(std::make_tuple(policy.Unordered, policy.SctpPrPolicy, policy.PrVal));
//      policy.Channel_Id = m_vSctpPolicies.size();
//      VLOG(2) << "Creating new channel: " << policy.Channel_Id;
//    }

    // new policy
//    m_vSctpPolicies.push_back(std::make_tuple(policy.Unordered, policy.SctpPrPolicy, policy.PrVal));
    policy.Channel_Id = m_rtpPolicies.size() + 2;
    VLOG(2) << "Creating new channel: " << policy.Channel_Id;
    m_rtpPolicies.push_back(policy);

//    policy.Channel_Id = m_rtpPolicies.size() + 2;
//    m_rtpPolicies.push_back(policy);
  }
}

PolicyDescriptor SctpRtpPolicy::parsePolicy(const std::string& sPolicy)
{
  PolicyDescriptor policy;
  std::vector<std::string> vTokens = StringTokenizer::tokenize(sPolicy, ": ", true);
  bool bSuccess;
  if (vTokens[0] == "RTCP")
  {
    if (vTokens.size() != 4) return policy;
    policy.Protocol = PT_RTCP;
    policy.Unordered = convert<bool>(vTokens[1], true);
    uint32_t uiPrPolicy = convert<uint32_t>(vTokens[2], bSuccess);
    if (!bSuccess) return policy;
    policy.SctpPrPolicy = static_cast<PrPolicy>(uiPrPolicy);
    policy.PrVal= convert<uint32_t>(vTokens[3], bSuccess);
    if (!bSuccess) return policy;
    policy.Valid = true;
    // RTCP is always channel 0
    policy.Channel_Id = DEFAULT_RTCP;
    return policy;
  }
  else if (vTokens[0] == "RTP")
  {
    if (vTokens.size() < 5) return policy;
    policy.Protocol = PT_RTP;

    size_t policyIndex = 0;
    if (vTokens[1] == "*")
    {
      policyIndex = 2;
    }
    else if (vTokens[1] == "AVC")
    {
      policy.Media = MT_AVC;
      // get frame type
      // AVC
      if (vTokens[2] == "*")
      {
        policy.FrameType = TYPE_NOT_SET;
      }
      else if (vTokens[2] == "I")
      {
        policy.FrameType = FRAME_TYPE_I;
      }
      else if (vTokens[2] == "P")
      {
        policy.FrameType = FRAME_TYPE_P;
      }
      else if (vTokens[2] == "B")
      {
        policy.FrameType = FRAME_TYPE_B;
      }
      else
      {
        // See if this is a NUT
        bool bSuccess;
        uint32_t val = convert<uint32_t>(vTokens[2], bSuccess);
        if (bSuccess && val <= (int)media::h264::NUT_FU_B)
        {
          policy.FrameType = FRAME_TYPE_NALU;
          policy.NaluType = static_cast<media::h264::NalUnitType>(val);
        }
        else
        {
          policy.FrameType = TYPE_NOT_SET;
        }
      }
      policyIndex = 3;
    }
    else if (vTokens[1] == "SVC")
    {
      policy.Media = MT_SVC;
      if (vTokens[2] == "*")
      {
        policy.Layer = TYPE_NOT_SET;
      }
      else if (vTokens[2] == "BL")
      {
        policy.Layer = LAYER_TYPE_BL;
      }
      else if (vTokens[2] == "EL")
      {
        policy.Layer = LAYER_TYPE_EL;
      }
      else
      {
        // See if this is a NUT
        bool bSuccess;
        uint32_t val = convert<uint32_t>(vTokens[2], bSuccess);
        if (bSuccess && val <= (int)media::h264::NUT_FU_B)
        {
          policy.Layer = LAYER_TYPE_NALU;
          policy.NaluType = static_cast<media::h264::NalUnitType>(val);
        }
        else
        {
          // graceful failure
          policy.Layer = TYPE_NOT_SET;
        }
      }
      // DQT
      bool bDummy;
      if (vTokens[3] == "*")
      {
        policy.D_L_ID = TYPE_NOT_SET;
      }
      else
      {
        policy.D_L_ID = convert<uint32_t>(vTokens[3], bDummy);
      }
      // DQT
      if (vTokens[4] == "*")
      {
        policy.Q_T_ID = TYPE_NOT_SET;
      }
      else
      {
        policy.Q_T_ID = convert<uint32_t>(vTokens[4], bDummy);
      }
      // DQT
      if (vTokens[5] == "*")
      {
        policy.T_ID = TYPE_NOT_SET;
      }
      else
      {
        policy.T_ID = convert<uint32_t>(vTokens[5], bDummy);
      }
      policyIndex = 6;
    }
    else
    {
      // unsupported
      return policy;
    }

    policy.Unordered = convert<bool>(vTokens[policyIndex], true);
    uint32_t uiPrPolicy = convert<uint32_t>(vTokens[policyIndex + 1], bSuccess);
    if (!bSuccess) return policy;
    policy.SctpPrPolicy = static_cast<PrPolicy>(uiPrPolicy);
    policy.PrVal= convert<uint32_t>(vTokens[policyIndex + 2], bSuccess);
    if (!bSuccess) return policy;

    if (vTokens.size() == policyIndex + 4)
    {
      policy.RttFactor = convert<double>(vTokens[policyIndex + 3], bSuccess);
      if (!bSuccess) return policy;
    }
    policy.Valid = true;
    return policy;
  }
  // return invalid policy
  return policy;
}

SctpRtpPolicy::SctpRtpPolicy()
  :m_rtcpPolicy(parsePolicy(DefaultRtcpPolicy)),
    m_rtpPolicy(parsePolicy(DefaultRtpPolicy))
{
  m_rtcpPolicy.Channel_Id = DEFAULT_RTCP;
  m_rtpPolicy.Channel_Id = DEFAULT_RTP;
  // m_sctpPolicies.push_back(m_rtpPolicy.Policy);
}

}
}
