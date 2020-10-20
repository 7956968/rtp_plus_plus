#pragma once
#include <rtp++/rfc2326/RtspUtil.h>


namespace rtp_plus_plus
{
namespace test {

const std::string DESCRIBE="DESCRIBE rtsp://127.0.0.1:8554 RTSP/1.0\r\n"\
    "CSeq: 22\r\n"\
    "User-Agent: ./openRTSP (LIVE555 Streaming Media v2013.07.16)\r\n"\
    "Accept: application/sdp\r\n\r\n";

const std::string DESCRIBE_RESPONSE="RTSP/1.0 200 OK\r\n"\
    "CSeq: 3\r\n"\
    "Date: Sat, Jul 20 2013 12:07:25 GMT\r\n"\
    "Content-Base: rtsp://127.0.0.1:8554/test.aac/\r\n"\
    "Content-Type: application/sdp\r\n"\
    "Content-Length: 514\r\n\r\n"\
    "v=0\r\n"\
    "o=- 1374322036525878 1 IN IP4 10.0.0.7\r\n"\
    "s=AAC Audio, streamed by the LIVE555 Media Server\r\n"\
    "i=test.aac\r\n"\
    "t=0 0\r\n"\
    "a=tool:LIVE555 Streaming Media v2013.07.16\r\n"\
    "a=type:broadcast\r\n"\
    "a=control:*\r\n"\
    "a=range:npt=0-\r\n"\
    "a=x-qt-text-nam:AAC Audio, streamed by the LIVE555 Media Server\r\n"\
    "a=x-qt-text-inf:test.aac\r\n"\
    "m=audio 0 RTP/AVP 96\r\n"\
    "c=IN IP4 0.0.0.0\r\n"\
    "b=AS:96\r\n"\
    "a=rtpmap:96 MPEG4-GENERIC/44100/2\r\n"\
    "a=fmtp:96 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210\r\n"\
    "a=control:track1\r\n";

const std::string NOT_FOUND="RTSP/1.0 404 Stream Not Found\r\n"\
    "CSeq: 2\r\n"\
    "Date: Sat, Jul 20 2013 11:56:48 GMT\r\n";

const std::string SETUP="SETUP rtsp://127.0.0.1:8554/test.aac/track1 RTSP/1.0\r\n"\
    "CSeq: 4\r\n"\
    "User-Agent: ./openRTSP (LIVE555 Streaming Media v2013.07.16)\r\n"\
    "Transport: RTP/AVP;unicast;client_port=33544-33545\r\n\r\n";

const char interleaved_rtp[] = {0x24, 0x00, 0x00, 0x01, 0x05};
const char interleaved_non_rtp[] = {0x25, 0x00, 0x00, 0x01, 0x05};

BOOST_AUTO_TEST_SUITE(NetworkTest)
BOOST_AUTO_TEST_CASE(test_matchRTSP)
{
  boost::regex regex("(\r\n\r\n)|(\\$)");
  boost::smatch what;
  std::string sRtp(interleaved_rtp);
  std::string sNonRtp(interleaved_non_rtp);
  std::string sJunk = "sdfksfsdrs";
  BOOST_CHECK_EQUAL(boost::regex_search(SETUP, what, regex), true);
  BOOST_CHECK_EQUAL(boost::regex_search(sRtp, what, regex), true);
  BOOST_CHECK_EQUAL(boost::regex_search(sJunk, what, regex), false);
  BOOST_CHECK_EQUAL(boost::regex_search(sNonRtp, what, regex), false);
  int i = 0;
}

BOOST_AUTO_TEST_CASE(test_extractRtspInfo)
{
  std::string sRequest;
  std::string sRtspUri;
  std::string sRtspVersion;
  BOOST_CHECK_EQUAL(rfc2326::extractRtspInfo(DESCRIBE, sRequest, sRtspUri, sRtspVersion), true);
  BOOST_CHECK_EQUAL(sRequest, "DESCRIBE");
  BOOST_CHECK_EQUAL(sRtspUri, "rtsp://127.0.0.1:8554");
  BOOST_CHECK_EQUAL(sRtspVersion, "1.0");
  BOOST_CHECK_EQUAL(rfc2326::extractRtspInfo(NOT_FOUND, sRequest, sRtspUri, sRtspVersion), false);
  BOOST_CHECK_EQUAL(rfc2326::extractRtspInfo(DESCRIBE_RESPONSE, sRequest, sRtspUri, sRtspVersion), false);
}

BOOST_AUTO_TEST_CASE(test_getCSeq)
{
  uint32_t uiCSeq = 0;
  BOOST_CHECK_EQUAL(rfc2326::getCSeq(DESCRIBE, uiCSeq), true);
  BOOST_CHECK_EQUAL(uiCSeq, 22);
  BOOST_CHECK_EQUAL(rfc2326::getCSeq(DESCRIBE_RESPONSE, uiCSeq), true);
  BOOST_CHECK_EQUAL(uiCSeq, 3);
  BOOST_CHECK_EQUAL(rfc2326::getCSeq(NOT_FOUND, uiCSeq), true);
  BOOST_CHECK_EQUAL(uiCSeq, 2);
}

BOOST_AUTO_TEST_CASE(test_getContentLength)
{
  uint32_t uiContentLength = 0;
  BOOST_CHECK_EQUAL(rfc2326::getContentLength(DESCRIBE, uiContentLength), false);
  BOOST_CHECK_EQUAL(uiContentLength, 0);
  BOOST_CHECK_EQUAL(rfc2326::getContentLength(NOT_FOUND, uiContentLength), false);
  BOOST_CHECK_EQUAL(uiContentLength, 0);
  BOOST_CHECK_EQUAL(rfc2326::getContentLength(DESCRIBE_RESPONSE, uiContentLength), true);
  BOOST_CHECK_EQUAL(uiContentLength, 514);
}

BOOST_AUTO_TEST_CASE(test_getTransport)
{
  rfc2326::Transport transport;
  BOOST_CHECK_EQUAL(rfc2326::getTransport(DESCRIBE, transport), false);
  BOOST_CHECK_EQUAL(rfc2326::getTransport(NOT_FOUND, transport), false);
  BOOST_CHECK_EQUAL(rfc2326::getTransport(DESCRIBE_RESPONSE, transport), false);
  BOOST_CHECK_EQUAL(rfc2326::getTransport(SETUP, transport), true);
  BOOST_CHECK_EQUAL(transport.getClientPortStart(),33544);
  BOOST_CHECK_EQUAL(transport.isUnicast(), true);
  BOOST_CHECK_EQUAL(transport.isMulticast(), false);
  BOOST_CHECK_EQUAL(transport.getSpecifier(), "RTP/AVP");
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
