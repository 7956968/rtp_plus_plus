#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <boost/optional.hpp>
#include <cpputil/StringTokenizer.h>
#include <rtp++/network/Sctp.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>

namespace rtp_plus_plus
{

namespace test
{
class SctpTest;
}

namespace sctp
{

// Standard policies
// PolicyString=[RTCP_POLICY | RTP_AVC_POLICY | RTP_SVC_POLICY]

// POLICY=[SCTP_UNORDERED : SCTP_PR : SCTP_VAL]
// DQT_ID=[*|D : *|Q : *|T]
// RTCP_POLICY=[RTCP : POLICY]
// RTP_AVC_POLICY=[RTP : AVC : *|I|P|B|NUT : POLICY : ?RTT_PARAMETER]
// RTP_SVC_POLICY:[RTP : SVC : *|BL|EL|NUT : DQT_ID : POLICY : ?RTT_PARAMETER]

// SCTP_UNORDERED=[0|1]
// SCTP_PR=[0|1|3]

// Semantic depends on SCTP_PR
// SCTP_VAL=(uint32_t)

// Only valid if SCTP_PR=1
// RTT_PARAMETER = [double (0.0)]

// examples
// RTCP
// RTCP:1:3:0                     = RTCP unordered with 0 retransmissions

// AVC
// RTP:AVC:*:1:3:1                = RTP AVC * unordered with 1 retransmission

// SVC
// RTP:SVC:*:*:*:*:1:1:100      = RTP SVC *  unordered with 100ms retransmission
// RTP:SVC:BL:*:*:*:1:1:100     = RTP SVC BL unordered with 100ms retransmission
// RTP:SVC:BL:*:*:*:1:1:100:0.5 = RTP SVC BL unordered with 0.5*RTT retransmission
// RTP:SVC:14:*:*:*:1:1:100     = RTP SVC 14 unordered with 100ms retransmission
// RTP:SVC:20:1:0:0:1:1:100     = RTP SVC 20 unordered with 100ms retransmission

extern const int32_t DEFAULT_RTCP;
extern const int32_t DEFAULT_RTP;

extern const int32_t TYPE_NOT_SET;
extern const int32_t FRAME_TYPE_I;
extern const int32_t FRAME_TYPE_P;
extern const int32_t FRAME_TYPE_B;
extern const int32_t FRAME_TYPE_NALU;
extern const int32_t LAYER_TYPE_BL;
extern const int32_t LAYER_TYPE_EL;
extern const int32_t LAYER_TYPE_NALU;

enum ProtocolType{
  PT_UNKNOWN,
  PT_RTCP,
  PT_RTP
};

enum MediaType
{
  MT_UNKNOWN,
  MT_AVC,
  MT_SVC,
  MT_HEVC,
  MT_SHVC
};

/**
  * Describes a policy for a single channel
  */
struct PolicyDescriptor
{
  PolicyDescriptor();
  bool Valid;
  // Desired channel id: 0 is reserved for default RTCP, 1 for default RTP
  uint32_t Channel_Id;
  // RTCP or RTP
  ProtocolType Protocol;
  // AVC/SVC
  MediaType Media;
  // -1=all (*), 0=BL, 1=EL, 2=NALU
  int32_t Layer;
  // -1=all (*), 0=I, 1=P, 2=B, 3=NALU
  int32_t FrameType;
  // NALU type
  uint32_t NaluType;
  // SVC: HACK: to refactor into more generic descriptors
  int32_t D_L_ID;
  int32_t Q_T_ID;
  int32_t T_ID;
  // PR order
  bool Unordered;
  // PR Policy
  PrPolicy SctpPrPolicy;
  // PR value
  uint32_t PrVal;
  // RTT
  double RttFactor;
};

class SctpRtpPolicy
{
  friend class SctpTest;
  const static uint32_t RtcpPolicyId;
  const static uint32_t RtpPolicyId;

public:
  // The default policies will be used if none have been configured
  static const std::string DefaultRtcpPolicy;
  static const std::string DefaultRtpPolicy;

  /**
    * Creates a custom SCTP policy object if the passed in values are valid.
    */
  static boost::optional<SctpRtpPolicy> create(const std::vector<std::string>& vPolicyVector);
  /**
    * Creates a default SCTP policy object
    */
  static SctpRtpPolicy createDefaultPolicy();

  static PolicyDescriptor parsePolicy(const std::string& sPolicy);

  /**
    * Constructor: should not be used directly. Instead create and createDefaultPolicy should be used.
    */
  SctpRtpPolicy();

  const PolicyDescriptor& getRtcpPolicy() const { return m_rtcpPolicy; }
  const PolicyDescriptor& getRtpPolicy() const { return m_rtpPolicy; }
  const std::vector<PolicyDescriptor>& getAdditionalRtpPolicies() const { return  m_rtpPolicies; }

  void setRtcpPolicy(const PolicyDescriptor& rtcpPolicy)
  {
    m_rtcpPolicy.Channel_Id = 0;
    m_rtcpPolicy = rtcpPolicy;
  }
  void setRtpPolicy(const PolicyDescriptor& rtpPolicy)
  {
    m_rtpPolicy = rtpPolicy;
    m_rtpPolicy.Channel_Id = 1;
//    if (m_vSctpPolicies.empty())
//    {
//      m_vSctpPolicies.push_back(std::make_tuple(rtpPolicy.Unordered, rtpPolicy.SctpPrPolicy, rtpPolicy.PrVal));
//    }
//    else
//    {
//      m_vSctpPolicies[0] = std::make_tuple(rtpPolicy.Unordered, rtpPolicy.SctpPrPolicy, rtpPolicy.PrVal);
//    }
  }

  void addPolicy(PolicyDescriptor policy);

private:

  // Base RTCP policy
  PolicyDescriptor m_rtcpPolicy;
  // Base RTP policy
  PolicyDescriptor m_rtpPolicy;
  // Additional RTP policies that override the base policy
  std::vector<PolicyDescriptor> m_rtpPolicies;

//  typedef std::tuple<bool, PrPolicy, uint32_t> SctpPolicyDescriptor_t;
//  std::vector<SctpPolicyDescriptor_t> m_vSctpPolicies;
};

}
}
