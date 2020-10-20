#pragma once
#include <cpputil/Buffer.h>
#include <cpputil/FileUtil.h>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/experimental/RtcpHeaderExtension.h>
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>

namespace rtp_plus_plus
{
namespace test {

static Buffer readRawRtcpFromFile(const std::string& sFile)
{
  std::string sRr = FileUtil::readFile(sFile, true);
  uint8_t* pBuffer = new uint8_t[sRr.length()];
  sRr.copy((char*)pBuffer, sRr.length());
  Buffer buffer(pBuffer, sRr.length());
  return buffer;
}

BOOST_AUTO_TEST_SUITE(RtcpTest)
BOOST_AUTO_TEST_CASE(test_ParseRtcpRr)
{
  // Test single packets
  Buffer buffer = readRawRtcpFromFile("./test_data/rtcp_rr.dat");
  // use reduced rule validator for parsing single RTP apcket
  RtpPacketiser rtpPacketiser;
  rtpPacketiser.setValidateRtcp(false);
  CompoundRtcpPacket rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 1);

  buffer = readRawRtcpFromFile("./test_data/rtcp_bye.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 1);

  buffer = readRawRtcpFromFile("./test_data/rtcp_sdes.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 1);

  // test other reports
  buffer = readRawRtcpFromFile("./test_data/rtcp_rr_2.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 1);

  buffer = readRawRtcpFromFile("./test_data/rtcp_sdes_2.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 1);

  // Test compound packets
  buffer = readRawRtcpFromFile("./test_data/rtcp_rr_sdes.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 2);

  buffer = readRawRtcpFromFile("./test_data/rtcp_rr_sdes_2.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 2);

  rtpPacketiser.setValidateRtcp(true);
  buffer = readRawRtcpFromFile("./test_data/rtcp_rr_sdes_3.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 2);

  buffer = readRawRtcpFromFile("./test_data/rtcp_rr_bye.dat");
  rtcp = rtpPacketiser.depacketiseRtcp(buffer);
  BOOST_CHECK_EQUAL( rtcp.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
