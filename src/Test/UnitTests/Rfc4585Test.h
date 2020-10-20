#pragma once
#include <boost/thread.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc4585/FeedbackManager.h>
#include <rtp++/rfc4585/Rfc4585.h>

#define RFC4585_TEST_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(Rfc4585Test)
BOOST_AUTO_TEST_CASE(test_FeedbackManager)
{
  VLOG(RFC4585_TEST_LOG_LEVEL) << "test_FeedbackManager";
  rfc3550::SdesInformation sdesInfo;
  // This sdp has NACK and the feedback profile set
  std::string sSdp = "v=0\r\n"\
      "o=- 1346075167783230 1 IN IP4 127.0.0.1\r\n"\
      "s=Session streamed by RTP\r\n"\
      "i=test\r\n"\
      "t=0 0\r\n"\
      "m=video 5004 RTP/AVPF 96 97\r\n"\
      "c=IN IP4 127.0.0.1\r\n"\
      "b=AS:1000\r\n"\
      "a=rtpmap:96 H265/90000\r\n"\
      "a=fmtp:96 packetization-mode=1;profile-level-id=304941;sprop-parameter-sets=J0IAFI1oFglE,AAAAAQ==\r\n"\
      "a=rtpmap:97 rtx/90000\r\n"\
      "a=fmtp:97 apt=96;rtx-time=3000\r\n"\
      "a=rtcp-fb:96 nack\r\n";

  boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(sSdp, false);
  BOOST_CHECK_EQUAL( sdp.is_initialized(), true);

  rfc4566::MediaDescription& media = sdp->getMediaDescription(0);
  RtpSessionParameters rtpParameters(sdesInfo, media, true);
  BOOST_CHECK_EQUAL(rtpParameters.isRetransmissionEnabled(), true);
  BOOST_CHECK_EQUAL(rtpParameters.getRetransmissionTimeout(), 3000);
  BOOST_CHECK_EQUAL(rtpParameters.supportsFeedbackMessage(rfc4585::NACK), true);
  BOOST_CHECK_EQUAL(rtpParameters.getProfile(), rfc4585::AVPF);
  GenericParameters applicationParameters;
  applicationParameters.setStringParameter(app::ApplicationParameters::pred, app::ApplicationParameters::simple);

  boost::asio::io_service ioService;
  rfc4585::FeedbackManager feedbackManager(rtpParameters, applicationParameters, ioService );
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiExtendedSN = 0;
  feedbackManager.onPacketArrival(tNow, uiExtendedSN++);
  feedbackManager.onPacketArrival(tNow, uiExtendedSN++);
  // lost packet
  uint32_t uiLostExtendedSN = uiExtendedSN++;
  feedbackManager.onPacketArrival(tNow, uiExtendedSN++);
  CompoundRtcpPacket rtcp;
  std::vector<uint32_t> lost = feedbackManager.getAssumedLost();
  BOOST_CHECK_EQUAL( lost.size(), 1);
  // simulate sending of NACK and RTX
  boost::this_thread::sleep(boost::posix_time::milliseconds(20));
  tNow = boost::posix_time::microsec_clock::universal_time();
  feedbackManager.onRtxPacketArrival(tNow, uiLostExtendedSN);
}

BOOST_AUTO_TEST_CASE(test_SdpParsing)
{
  VLOG(RFC4585_TEST_LOG_LEVEL) << "test_SdpParsing";
  const std::string sSdp("v=0\r\n"\
                         "o=- 1346075167783230 1 IN IP4 130.149.228.121\r\n"\
                         "s=Session streamed by \"rtp\"\r\n"\
                         "i=test\r\n"\
                         "t=0 0\r\n"\
                         "a=tool:rtp++\r\n"\
                         "a=control:*\r\n"\
                         "a=range:npt=0-\r\n"\
                         "m=video 5004 RTP/AVPF 96 97\r\n"\
                         "c=IN IP4 130.149.228.121\r\n"\
                         "b=AS:1000\r\n"\
                         "a=rtpmap:96 H265/90000\r\n"\
                         "a=fmtp:96 packetization-mode=1;profile-level-id=304941;sprop-parameter-sets=J0IAFI1oFglE,AAAAAQ==\r\n"\
                         "a=rtpmap:97 rtx/90000\r\n"\
                         "a=fmtp:97 apt=96;rtx-time=3000\r\n"\
                         "a=rtcp-fb:96 nack\r\n"\
                         "a=control:track1\r\n");

  boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(sSdp, false);
  BOOST_CHECK_EQUAL( sdp.is_initialized(), true);
  const std::map<std::string, rfc4566::FeedbackDescription>& mFeedback = sdp->getMediaDescription(0).getFeedbackMap();
  auto it = mFeedback.find("96");
  BOOST_CHECK_EQUAL( it->second.FeedbackTypes[0], rfc4585::NACK);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
