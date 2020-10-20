#pragma once
#include <rtp++/sctp/SctpRtpPolicy.h>

namespace rtp_plus_plus
{
namespace test {

BOOST_AUTO_TEST_SUITE(SctpUnitTest)
BOOST_AUTO_TEST_CASE(test_ParseSctpRtpPolicy)
{
  using namespace sctp;

  static const std::string sDefaultRtcpPolicy = "RTCP:1:0:0";
  sctp::PolicyDescriptor policy = sctp::SctpRtpPolicy::parsePolicy(sDefaultRtcpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTCP);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_NONE);

  static const std::string sDefaultRtpPolicy = "RTP:*:1:3:0";
  policy = sctp::SctpRtpPolicy::parsePolicy(sDefaultRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_UNKNOWN);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 0);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sBlRtpPolicy = "RTP:SVC:BL:*:*:*:0:1:100";
  policy = sctp::SctpRtpPolicy::parsePolicy(sBlRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_SVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, LAYER_TYPE_BL);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.Unordered, false);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_TTL);
  BOOST_CHECK_EQUAL( policy.PrVal, 100);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sElRtpPolicy = "RTP:SVC:EL:*:*:*:0:1:50:0.5";
  policy = sctp::SctpRtpPolicy::parsePolicy(sElRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_SVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, LAYER_TYPE_EL);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.Unordered, false);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_TTL);
  BOOST_CHECK_EQUAL( policy.PrVal, 50);
  BOOST_CHECK_EQUAL( policy.RttFactor, 0.5);

  static const std::string sIRtpPolicy = "RTP:AVC:I:1:3:4";
  policy = sctp::SctpRtpPolicy::parsePolicy(sIRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_AVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.FrameType, FRAME_TYPE_I);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 4);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sPRtpPolicy = "RTP:AVC:P:1:3:3";
  policy = sctp::SctpRtpPolicy::parsePolicy(sPRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_AVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.FrameType, FRAME_TYPE_P);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 3);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sBRtpPolicy = "RTP:AVC:B:1:3:2";
  policy = sctp::SctpRtpPolicy::parsePolicy(sBRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_AVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.FrameType, FRAME_TYPE_B);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 2);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sDRtpPolicy = "RTP:SVC:*:0:1:1:1:3:4";
  policy = sctp::SctpRtpPolicy::parsePolicy(sDRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Media, MT_SVC);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Layer, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.D_L_ID, 0);
  BOOST_CHECK_EQUAL( policy.Q_T_ID, 1);
  BOOST_CHECK_EQUAL( policy.T_ID, 1);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 4);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  using namespace media::h264;

  static const std::string sQRtpPolicy = "RTP:SVC:5:1:0:1:1:3:3";
  policy = sctp::SctpRtpPolicy::parsePolicy(sQRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Media, MT_SVC);
  BOOST_CHECK_EQUAL( policy.Layer, LAYER_TYPE_NALU);
  BOOST_CHECK_EQUAL( policy.NaluType, NUT_CODED_SLICE_OF_AN_IDR_PICTURE);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.D_L_ID, 1);
  BOOST_CHECK_EQUAL( policy.Q_T_ID, 0);
  BOOST_CHECK_EQUAL( policy.T_ID, 1);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 3);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);

  static const std::string sTRtpPolicy = "RTP:SVC:14:1:1:0:1:3:2";
  policy = sctp::SctpRtpPolicy::parsePolicy(sTRtpPolicy);
  BOOST_CHECK_EQUAL( policy.Valid, true);
  BOOST_CHECK_EQUAL( policy.Protocol, PT_RTP);
  BOOST_CHECK_EQUAL( policy.Media, MT_SVC);
  BOOST_CHECK_EQUAL( policy.NaluType, NUT_PREFIX_NAL_UNIT);
  BOOST_CHECK_EQUAL( policy.Layer, LAYER_TYPE_NALU);
  BOOST_CHECK_EQUAL( policy.FrameType, TYPE_NOT_SET);
  BOOST_CHECK_EQUAL( policy.D_L_ID, 1);
  BOOST_CHECK_EQUAL( policy.Q_T_ID, 1);
  BOOST_CHECK_EQUAL( policy.T_ID, 0);
  BOOST_CHECK_EQUAL( policy.Unordered, true);
  BOOST_CHECK_EQUAL( policy.SctpPrPolicy, SCTP_PR_POLICY_SCTP_RTX);
  BOOST_CHECK_EQUAL( policy.PrVal, 2);
  BOOST_CHECK_EQUAL( policy.RttFactor, -1.0);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
