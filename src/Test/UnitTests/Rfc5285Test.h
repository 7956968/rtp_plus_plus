#pragma once
#include <cpputil/Buffer.h>
#include <cpputil/FileUtil.h>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/experimental/RtcpHeaderExtension.h>
#include <rtp++/mprtp/MpRtpHeaderExtension.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/rfc4585/Rfc4585RtcpParser.h>
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>
#include <rtp++/rfc6051/Rfc6051.h>

#define RFC5285_TEST_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace test {

using namespace rfc5285;

#if 0
static Buffer readRawRtcpFromFile(const std::string& sFile)
{
  std::string sRr = FileUtil::readFile(sFile, true);
  uint8_t* pBuffer = new uint8_t[sRr.length()];
  sRr.copy((char*)pBuffer, sRr.length());
  Buffer buffer(pBuffer, sRr.length());
  return buffer;
}
#endif

BOOST_AUTO_TEST_SUITE(Rfc4585Test)
BOOST_AUTO_TEST_CASE(test_OneByteHeaderExtension)
{
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_OneByteHeaderExtension";
  uint32_t uiId = 1;
  uint64_t uiTs = 12345;
  RtpHeaderExtension headerExtension;
  headerExtension.addHeaderExtensions(rfc6051::RtpSynchronisationExtensionHeader::create(uiId, uiTs));
  OBitStream ob;
  headerExtension.writeExtensionData(ob);

  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_OneByteHeaderExtension Size of extension: " << ob.bytesUsed();

  IBitStream ib(ob.data());
  RtpHeaderExtension headerExtension2;
  headerExtension2.readExtensionData(ib);
  BOOST_CHECK_EQUAL( headerExtension2.getProfileDefined(), 0xBEDE);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_OneByteHeaderExtension: " << hex(headerExtension2.getProfileDefined());
  BOOST_CHECK_EQUAL( headerExtension.getHeaderExtensionCount(), headerExtension2.getHeaderExtensionCount());
  const std::vector<HeaderExtensionElement>& extensions = headerExtension2.getHeaderExtensions();
  BOOST_CHECK_EQUAL( extensions.size(), 1);
  BOOST_CHECK_EQUAL( extensions.size(), 1);
  boost::optional<uint64_t> uiTs2 = rfc6051::RtpSynchronisationExtensionHeader::parseRtpSynchronisationExtensionHeader(extensions[0]);
  BOOST_CHECK_EQUAL( uiTs, *uiTs2);
}

/**
 * @brief test_MultipleOneByteHeaderExtensions tests that a one byte header extension is used when all extensions are small enough
 */
BOOST_AUTO_TEST_CASE(test_MultipleOneByteHeaderExtensions)
{
  using namespace mprtp;

  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_MultipleOneByteHeaderExtension";
  uint32_t uiTsId = 1;
  uint32_t uiMpRtpId = 2;
  RtpHeaderExtension headerExtension;
  // add sync header
  uint64_t uiTs = 12345;
  headerExtension.addHeaderExtensions(rfc6051::RtpSynchronisationExtensionHeader::create(uiTsId, uiTs));
  // add MPRTP header
  uint32_t uiSubflowId = 5;
  uint32_t uiSubflowFSSN = 12345;
  MpRtpSubflowRtpHeader subflowHeader(uiSubflowId, uiSubflowFSSN);
  headerExtension.addHeaderExtensions(MpRtpHeaderExtension::generateMpRtpHeaderExtension(uiMpRtpId, subflowHeader));

  OBitStream ob;
  headerExtension.writeExtensionData(ob);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_MultipleOneByteHeaderExtensions Size of extension: " << ob.bytesUsed();

  IBitStream ib(ob.data());
  RtpHeaderExtension headerExtension2;
  headerExtension2.readExtensionData(ib);
  BOOST_CHECK_EQUAL( headerExtension2.getProfileDefined(), 0xBEDE);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_OneByteHeaderExtension: " << hex(headerExtension2.getProfileDefined());
  BOOST_CHECK_EQUAL( headerExtension.getHeaderExtensionCount(), headerExtension2.getHeaderExtensionCount());
  const std::vector<HeaderExtensionElement>& extensions = headerExtension2.getHeaderExtensions();
  BOOST_CHECK_EQUAL( extensions.size(), 2);
  // check sync header
  boost::optional<uint64_t> uiTs2 = rfc6051::RtpSynchronisationExtensionHeader::parseRtpSynchronisationExtensionHeader(extensions[0]);
  BOOST_CHECK_EQUAL( uiTs, *uiTs2);
  // check MPRTP header
  boost::optional<MpRtpSubflowRtpHeader> subflowHeader2 = MpRtpHeaderExtension::parseMpRtpHeader(extensions[1]).SubflowHeader;
  BOOST_CHECK_EQUAL( subflowHeader2.is_initialized(), true);
  BOOST_CHECK_EQUAL( subflowHeader2->getFlowId(), uiSubflowId);
  BOOST_CHECK_EQUAL( subflowHeader2->getFlowSpecificSequenceNumber(), uiSubflowFSSN);
}

BOOST_AUTO_TEST_CASE(test_TwoByteHeaderExtension)
{
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension";
  using namespace experimental;

  uint32_t uiId = 1;
  RtpHeaderExtension headerExtension;

  CompoundRtcpPacket compoundRtcp;
  uint32_t uiSSRC = 0;
  uint32_t uiNtpTimestampMsw = 1;
  uint32_t uiNtpTimestampLsw = 2;
  uint32_t uiRtpTimestamp = 3;
  uint32_t uiSendersPacketCount = 4;
  uint32_t uiSendersOctetCount = 5;

  rfc3550::RtcpSr::ptr pSr0 = rfc3550::RtcpSr::create(uiSSRC, uiNtpTimestampMsw, uiNtpTimestampLsw, uiRtpTimestamp, uiSendersPacketCount, uiSendersOctetCount);
  compoundRtcp.push_back(pSr0);

  // vector of extended sequence numbers
  std::vector<uint32_t> vReceivedSN;
  uint32_t uiSN = 12345;
  vReceivedSN.push_back(uiSN);
  vReceivedSN.push_back(uiSN + 1);
  vReceivedSN.push_back(uiSN + 2);
  rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(vReceivedSN);
  // get own SSRC
  uint32_t uiSSRC2 = 12345;
  pAck->setSenderSSRC(uiSSRC);
  // get the source SSRC
  pAck->setSourceSSRC(uiSSRC2);
  compoundRtcp.push_back(pAck);

  boost::optional<rfc5285::HeaderExtensionElement> pExt = RtcpHeaderExtension::create(uiId, compoundRtcp);
  BOOST_CHECK_EQUAL( pExt.is_initialized(), true);
  headerExtension.addHeaderExtensions(*pExt);

  OBitStream ob;
  headerExtension.writeExtensionData(ob);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension Size of extension: " << ob.bytesUsed();

  IBitStream ib(ob.data());
  RtpHeaderExtension headerExtension2;
  headerExtension2.readExtensionData(ib);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension: " << hex(headerExtension2.getProfileDefined());
  BOOST_CHECK_EQUAL( (headerExtension2.getProfileDefined() >> 4), 0x100);
  BOOST_CHECK_EQUAL( headerExtension.getHeaderExtensionCount(), headerExtension2.getHeaderExtensionCount());

  const std::vector<HeaderExtensionElement>& extensions = headerExtension2.getHeaderExtensions();
  BOOST_CHECK_EQUAL( extensions.size(), 1);

  RtpPacketiser rtpPacketiser;
  rtpPacketiser.setValidateRtcp(false);
  rtpPacketiser.registerRtcpParser(rfc4585::RtcpParser::create());
  CompoundRtcpPacket extractedRtcp = RtcpHeaderExtension::parse(*pExt, rtpPacketiser);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "Parsed " << extractedRtcp.size() << " RTCP packets";
  BOOST_CHECK_EQUAL( extractedRtcp.size(), 2);

  rfc3550::RtcpSr& sr = static_cast<rfc3550::RtcpSr&>(*(extractedRtcp[0].get()));
  BOOST_CHECK_EQUAL( sr.getSSRC(), uiSSRC);
  BOOST_CHECK_EQUAL( sr.getNtpTimestampLsw(), uiNtpTimestampLsw);
  BOOST_CHECK_EQUAL( sr.getNtpTimestampMsw(), uiNtpTimestampMsw);
  BOOST_CHECK_EQUAL( sr.getRtpTimestamp(), uiRtpTimestamp);
  BOOST_CHECK_EQUAL( sr.getSendersOctetCount(), uiSendersOctetCount);
  BOOST_CHECK_EQUAL( sr.getSendersPacketCount(), uiSendersPacketCount);

  rfc4585::RtcpGenericAck& ack = static_cast<rfc4585::RtcpGenericAck&>(*(extractedRtcp[1].get()));
  std::vector<uint16_t> acks = ack.getAcks();
  BOOST_CHECK_EQUAL( acks.size(), 3);
  BOOST_CHECK_EQUAL( acks[0], uiSN);
  BOOST_CHECK_EQUAL( acks[1], uiSN + 1);
  BOOST_CHECK_EQUAL( acks[2], uiSN + 2);
}

BOOST_AUTO_TEST_CASE(test_MultipleTwoByteHeaderExtensions)
{
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension";
  using namespace experimental;
  using namespace mprtp;

  uint32_t uiRtcpId = 1;
  uint32_t uiTsId = 2;
  uint32_t uiMpRtpId = 3;

  RtpHeaderExtension headerExtension;
  // Add RTCP header
  CompoundRtcpPacket compoundRtcp;
  uint32_t uiSSRC = 0;
  uint32_t uiNtpTimestampMsw = 1;
  uint32_t uiNtpTimestampLsw = 2;
  uint32_t uiRtpTimestamp = 3;
  uint32_t uiSendersPacketCount = 4;
  uint32_t uiSendersOctetCount = 5;

  rfc3550::RtcpSr::ptr pSr0 = rfc3550::RtcpSr::create(uiSSRC, uiNtpTimestampMsw, uiNtpTimestampLsw, uiRtpTimestamp, uiSendersPacketCount, uiSendersOctetCount);
  compoundRtcp.push_back(pSr0);

  // vector of extended sequence numbers
  std::vector<uint32_t> vReceivedSN;
  uint32_t uiSN = 12345;
  vReceivedSN.push_back(uiSN);
  vReceivedSN.push_back(uiSN + 1);
  vReceivedSN.push_back(uiSN + 2);
  rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(vReceivedSN);
  // get own SSRC
  uint32_t uiSSRC2 = 12345;
  pAck->setSenderSSRC(uiSSRC);
  // get the source SSRC
  pAck->setSourceSSRC(uiSSRC2);
  compoundRtcp.push_back(pAck);

  boost::optional<rfc5285::HeaderExtensionElement> pExt = RtcpHeaderExtension::create(uiRtcpId, compoundRtcp);
  BOOST_CHECK_EQUAL( pExt.is_initialized(), true);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension adding element id: " << pExt->getId();
  headerExtension.addHeaderExtensions(*pExt);

  // add sync header
  uint64_t uiTs = 12345;
  rfc5285::HeaderExtensionElement el = rfc6051::RtpSynchronisationExtensionHeader::create(uiTsId, uiTs);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension adding element id: " << el.getId();
  headerExtension.addHeaderExtensions(el);

  // add MPRTP header
  uint32_t uiSubflowId = 5;
  uint32_t uiSubflowFSSN = 12345;
  MpRtpSubflowRtpHeader subflowHeader(uiSubflowId, uiSubflowFSSN);
  el = MpRtpHeaderExtension::generateMpRtpHeaderExtension(uiMpRtpId, subflowHeader);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension adding element id: " << el.getId();
  headerExtension.addHeaderExtensions(el);

  OBitStream ob;
  headerExtension.writeExtensionData(ob);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_MultipleTwoByteHeaderExtensions Size of extension: " << ob.bytesUsed();

  IBitStream ib(ob.data());
  RtpHeaderExtension headerExtension2;
  headerExtension2.readExtensionData(ib);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_TwoByteHeaderExtension: " << hex(headerExtension2.getProfileDefined());
  BOOST_CHECK_EQUAL( (headerExtension2.getProfileDefined() >> 4), 0x100);
  BOOST_CHECK_EQUAL( headerExtension.getHeaderExtensionCount(), headerExtension2.getHeaderExtensionCount());

  const std::vector<HeaderExtensionElement>& extensions = headerExtension2.getHeaderExtensions();
  BOOST_CHECK_EQUAL( extensions.size(), 3);

  // check RTCP
  RtpPacketiser rtpPacketiser;
  rtpPacketiser.setValidateRtcp(false);
  rtpPacketiser.registerRtcpParser(rfc4585::RtcpParser::create());
  CompoundRtcpPacket extractedRtcp = RtcpHeaderExtension::parse(*pExt, rtpPacketiser);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "Parsed " << extractedRtcp.size() << " RTCP packets";
  BOOST_CHECK_EQUAL( extractedRtcp.size(), 2);
  rfc3550::RtcpSr& sr = static_cast<rfc3550::RtcpSr&>(*(extractedRtcp[0].get()));
  BOOST_CHECK_EQUAL( sr.getSSRC(), uiSSRC);
  BOOST_CHECK_EQUAL( sr.getNtpTimestampLsw(), uiNtpTimestampLsw);
  BOOST_CHECK_EQUAL( sr.getNtpTimestampMsw(), uiNtpTimestampMsw);
  BOOST_CHECK_EQUAL( sr.getRtpTimestamp(), uiRtpTimestamp);
  BOOST_CHECK_EQUAL( sr.getSendersOctetCount(), uiSendersOctetCount);
  BOOST_CHECK_EQUAL( sr.getSendersPacketCount(), uiSendersPacketCount);
  rfc4585::RtcpGenericAck& ack = static_cast<rfc4585::RtcpGenericAck&>(*(extractedRtcp[1].get()));
  std::vector<uint16_t> acks = ack.getAcks();
  BOOST_CHECK_EQUAL( acks.size(), 3);
  BOOST_CHECK_EQUAL( acks[0], uiSN);
  BOOST_CHECK_EQUAL( acks[1], uiSN + 1);
  BOOST_CHECK_EQUAL( acks[2], uiSN + 2);
  // check sync header
  boost::optional<uint64_t> uiTs2 = rfc6051::RtpSynchronisationExtensionHeader::parseRtpSynchronisationExtensionHeader(extensions[1]);
  BOOST_CHECK_EQUAL( uiTs, *uiTs2);
  // check MPRTP header
  boost::optional<MpRtpSubflowRtpHeader> subflowHeader2 = MpRtpHeaderExtension::parseMpRtpHeader(extensions[2]).SubflowHeader;
  BOOST_CHECK_EQUAL( subflowHeader2.is_initialized(), true);
  BOOST_CHECK_EQUAL( subflowHeader2->getFlowId(), uiSubflowId);
  BOOST_CHECK_EQUAL( subflowHeader2->getFlowSpecificSequenceNumber(), uiSubflowFSSN);
}

BOOST_AUTO_TEST_CASE(test_ParseRtcpRtpHeaderExtension)
{
  VLOG(RFC5285_TEST_LOG_LEVEL) << "test_ParseRtcpRtpHeaderExtension";
  using namespace experimental;
  uint32_t uiId = 3;

  // vector of extended sequence numbers
  std::vector<uint32_t> vReceivedSN;
  uint32_t uiSN = 12345;
  vReceivedSN.push_back(uiSN);
  vReceivedSN.push_back(uiSN + 1);
  vReceivedSN.push_back(uiSN + 2);
  CompoundRtcpPacket compoundRtcp;
  rfc4585::RtcpGenericAck::ptr pAck = rfc4585::RtcpGenericAck::create(vReceivedSN);
  // get own SSRC
  uint32_t uiSSRC = 54321;
  uint32_t uiSSRC2 = 12345;
  pAck->setSenderSSRC(uiSSRC);
  // get the source SSRC
  pAck->setSourceSSRC(uiSSRC2);
  compoundRtcp.push_back(pAck);
  BOOST_CHECK_EQUAL( pAck->getLength(), 3);

  boost::optional<rfc5285::HeaderExtensionElement> pExt = RtcpHeaderExtension::create(uiId, compoundRtcp);
  BOOST_CHECK_EQUAL( pExt.is_initialized(), true);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "Serialised length: " << pExt->getDataLength();

  RtpPacketiser rtpPacketiser;
  rtpPacketiser.setValidateRtcp(false);
  rtpPacketiser.registerRtcpParser(rfc4585::RtcpParser::create());
  CompoundRtcpPacket extractedRtcp = RtcpHeaderExtension::parse(*pExt, rtpPacketiser);
  VLOG(RFC5285_TEST_LOG_LEVEL) << "Parsed " << extractedRtcp.size() << " RTCP packets";
  BOOST_CHECK_EQUAL( extractedRtcp.size(), 1);

  rfc4585::RtcpGenericAck& ack = static_cast<rfc4585::RtcpGenericAck&>(*(extractedRtcp[0].get()));
  std::vector<uint16_t> acks = ack.getAcks();
  BOOST_CHECK_EQUAL( acks.size(), 3);
  BOOST_CHECK_EQUAL( acks[0], uiSN);
  BOOST_CHECK_EQUAL( acks[1], uiSN + 1);
  BOOST_CHECK_EQUAL( acks[2], uiSN + 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
