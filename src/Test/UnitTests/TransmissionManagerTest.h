#pragma once
#include <boost/asio/io_service.hpp>
#include <rtp++/RtpSessionState.h>
#include <rtp++/TransmissionManager.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(TransmissionManagerTest)

BOOST_AUTO_TEST_CASE(test_storePacketForRetransmission)
{
  uint8_t uiPayloadType = 96;
  RtpSessionState rtpSessionState(uiPayloadType);
  boost::asio::io_service ioService;
  TxBufferManagementMode eMode = TxBufferManagementMode::ACK_MODE;
  uint32_t uiRtxTimeMs = 100;
  TransmissionManager tm(rtpSessionState, ioService, eMode, uiRtxTimeMs);

  RtpPacket rtpPacket;
  const uint32_t uiSN = 12345;
  rtpPacket.setExtendedSequenceNumber(uiSN);
  rtpPacket.getHeader().setPayloadType(uiPayloadType);
  
  tm.storePacketForRetransmission(rtpPacket);

  TransmissionManager::RtpPacketInfo* pInfo = tm.lookupRtpPacketInfo(uiSN);
  BOOST_CHECK_EQUAL(pInfo != NULL, true);
  pInfo = tm.lookupRtpPacketInfo(uiSN + 1);
  BOOST_CHECK_EQUAL(pInfo == nullptr, true);
}

BOOST_AUTO_TEST_CASE(test_getLastNReceivedSequenceNumbers)
{
  uint8_t uiPayloadType = 96;
  RtpSessionState rtpSessionState(uiPayloadType);
  boost::asio::io_service ioService;
  TxBufferManagementMode eMode = TxBufferManagementMode::ACK_MODE;
  uint32_t uiRtxTimeMs = 100;
  uint32_t uiBufSize = 64;
  TransmissionManager tm(rtpSessionState, ioService, eMode, uiRtxTimeMs, uiBufSize);
  
  RtpPacket rtpPacket;
  rtpPacket.getHeader().setPayloadType(uiPayloadType);

  uint32_t uiSN = 100;
  uint32_t uiCount = 100;
  for (size_t i = 0; i < uiCount; ++i)
  {
    rtpPacket.setExtendedSequenceNumber(uiSN++);
    tm.onPacketReceived(rtpPacket);
  }

  std::vector<uint32_t> sns = tm.getLastNReceivedSequenceNumbers(1);
  BOOST_CHECK_EQUAL(sns.size(), 1);
  BOOST_CHECK_EQUAL(sns[0], 199);

  sns = tm.getLastNReceivedSequenceNumbers(5);
  BOOST_CHECK_EQUAL(sns.size(), 5);
  BOOST_CHECK_EQUAL(sns[0], 195);
  BOOST_CHECK_EQUAL(sns[1], 196);
  BOOST_CHECK_EQUAL(sns[2], 197);
  BOOST_CHECK_EQUAL(sns[3], 198);
  BOOST_CHECK_EQUAL(sns[4], 199);

  // test to see that we can only go back as far as there are elements in the buffer
  sns = tm.getLastNReceivedSequenceNumbers(100);
  BOOST_CHECK_EQUAL(sns.size(), uiBufSize); // buffer size is only 64
  BOOST_CHECK_EQUAL(sns[0], 199 - (uiBufSize - 1));
  BOOST_CHECK_EQUAL(sns[uiBufSize - 1], 199);
}
BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
