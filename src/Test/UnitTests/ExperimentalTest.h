#pragma once
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/experimental/ExperimentalRtcp.h>

namespace rtp_plus_plus {
namespace experimental {
namespace test {

BOOST_AUTO_TEST_SUITE(RtcpGenericAckTest)

BOOST_AUTO_TEST_CASE(test_bitmask_calculations)
{
  uint16_t uiSN = 1;
  rfc4585::RtcpGenericAck::ptr pRtcp = rfc4585::RtcpGenericAck::create(uiSN);
  // check masks and bases
  const std::vector<uint16_t>& vBases = pRtcp->getBases();
  const std::vector<uint16_t>& vMasks = pRtcp->getMasks();
  BOOST_CHECK_EQUAL(vBases.size(), 1);
  BOOST_CHECK_EQUAL(vMasks.size(), 1);
  BOOST_CHECK_EQUAL(vBases[0], 1);
  BOOST_CHECK_EQUAL(vMasks[0], 0);

  pRtcp->addAck(uiSN + 1);
  BOOST_CHECK_EQUAL(vBases.size(), 1);
  BOOST_CHECK_EQUAL(vMasks.size(), 1);
  BOOST_CHECK_EQUAL(vBases[0], 2);
  BOOST_CHECK_EQUAL(vMasks[0], 1);

  pRtcp->addAck(uiSN + 3);
  BOOST_CHECK_EQUAL(vBases.size(), 1);
  BOOST_CHECK_EQUAL(vMasks.size(), 1);
  BOOST_CHECK_EQUAL(vBases[0], 4);
  BOOST_CHECK_EQUAL(vMasks[0], 6);

  pRtcp->addAck(uiSN + 16);
  BOOST_CHECK_EQUAL(vBases.size(), 1);
  BOOST_CHECK_EQUAL(vMasks.size(), 1);
  BOOST_CHECK_EQUAL(vBases[0], 17);
  BOOST_CHECK_EQUAL(vMasks[0], 53248);

  // test adding of number more than 16 away
  pRtcp->addAck(uiSN + 17);
  BOOST_CHECK_EQUAL(vBases.size(), 2);
  BOOST_CHECK_EQUAL(vMasks.size(), 2);
  BOOST_CHECK_EQUAL(vBases[0], uiSN + 17);
  BOOST_CHECK_EQUAL(vMasks[0], 40961);
  BOOST_CHECK_EQUAL(vBases[1], 1);
  BOOST_CHECK_EQUAL(vMasks[1], 0);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(test_parseExperimentalRtcp_SCREAM)
{
  RtcpScreamFb::ptr pScream = RtcpScreamFb::create(1,2,3,4,5);
  OBitStream ob;
  ob << pScream.get();

  IBitStream ib(ob.data());
  uint32_t uiVersion        = UINT_MAX;
  uint32_t uiPadding        = UINT_MAX;
  uint32_t uiTypeSpecific   = UINT_MAX;
  uint32_t uiPayload        = UINT_MAX;
  uint32_t uiLengthInWords  = UINT_MAX;
  // RTP version: 2 bits
  ib.read(uiVersion, 2);
  // Padding: 1 bit
  ib.read(uiPadding, 1);
  // Type specific: 5 bits
  ib.read(uiTypeSpecific, 5);
  // Payload type: 8 bits
  ib.read(uiPayload, 8);
  // Length in 32bit words: 16 bits
  ib.read(uiLengthInWords, 16);

  RtcpScreamFb::ptr pScream2 = RtcpScreamFb::createRaw(uiVersion, uiPadding, uiLengthInWords);
  pScream2->read(ib);
  BOOST_CHECK_EQUAL(pScream->getJiffy(), pScream2->getJiffy());
  BOOST_CHECK_EQUAL(pScream->getSenderSSRC(), pScream2->getSenderSSRC());
  BOOST_CHECK_EQUAL(pScream->getSourceSSRC(), pScream2->getSourceSSRC());
  BOOST_CHECK_EQUAL(pScream->getTypeSpecific(), pScream2->getTypeSpecific());
  BOOST_CHECK_EQUAL(pScream->getVersion(), pScream2->getVersion());
}

BOOST_AUTO_TEST_CASE(test_parseExperimentalRtcp_NADA)
{
  RtcpNadaFb::ptr pNada = RtcpNadaFb::create(0,1,2,3,4);
  OBitStream ob;
  ob << pNada.get();

  IBitStream ib(ob.data());
  uint32_t uiVersion        = UINT_MAX;
  uint32_t uiPadding        = UINT_MAX;
  uint32_t uiTypeSpecific   = UINT_MAX;
  uint32_t uiPayload        = UINT_MAX;
  uint32_t uiLengthInWords  = UINT_MAX;
  // RTP version: 2 bits
  ib.read(uiVersion, 2);
  // Padding: 1 bit
  ib.read(uiPadding, 1);
  // Type specific: 5 bits
  ib.read(uiTypeSpecific, 5);
  // Payload type: 8 bits
  ib.read(uiPayload, 8);
  // Length in 32bit words: 16 bits
  ib.read(uiLengthInWords, 16);

  RtcpNadaFb::ptr pNada2 = RtcpNadaFb::createRaw(uiVersion, uiPadding, uiLengthInWords);
  pNada2->read(ib);
  BOOST_CHECK_EQUAL(pNada->get_x_n(), pNada2->get_x_n());
  BOOST_CHECK_EQUAL(pNada->get_r_recv(), pNada2->get_r_recv());
  BOOST_CHECK_EQUAL(pNada->get_rmode(), pNada2->get_rmode());
  BOOST_CHECK_EQUAL(pNada->getSenderSSRC(), pNada2->getSenderSSRC());
  BOOST_CHECK_EQUAL(pNada->getSourceSSRC(), pNada2->getSourceSSRC());
  BOOST_CHECK_EQUAL(pNada->getTypeSpecific(), pNada2->getTypeSpecific());
  BOOST_CHECK_EQUAL(pNada->getVersion(), pNada2->getVersion());
}

BOOST_AUTO_TEST_CASE(test_parseExperimentalRtcp_REMB)
{
  RtcpRembFb remb(160, 1, 2);
  std::string sRemb = remb.toString();
  boost::optional<RtcpRembFb> remb2 = RtcpRembFb::parseFromAppData(sRemb);
  BOOST_CHECK_EQUAL(remb2.is_initialized(), true);
  BOOST_CHECK_EQUAL(remb.getBps(), remb2->getBps());
  BOOST_CHECK_EQUAL(remb.getNumSsrcs(), remb2->getNumSsrcs());
  BOOST_CHECK_EQUAL(remb.getSsrcFb(), remb2->getSsrcFb());
}

BOOST_AUTO_TEST_CASE(test_parseExperimentalRtcp_FB_REMB)
{
  RtcpRembFb remb(160, 1, 2);
  std::string sRemb = remb.toString();
  rfc4585::RtcpApplicationLayerFeedback::ptr pRembFb = rfc4585::RtcpApplicationLayerFeedback::create(sRemb, 1, 2);
  OBitStream ob;
  ob << pRembFb.get();

  IBitStream ib(ob.data());
  uint32_t uiVersion        = UINT_MAX;
  uint32_t uiPadding        = UINT_MAX;
  uint32_t uiTypeSpecific   = UINT_MAX;
  uint32_t uiPayload        = UINT_MAX;
  uint32_t uiLengthInWords  = UINT_MAX;
  // RTP version: 2 bits
  ib.read(uiVersion, 2);
  // Padding: 1 bit
  ib.read(uiPadding, 1);
  // Type specific: 5 bits
  ib.read(uiTypeSpecific, 5);
  // Payload type: 8 bits
  ib.read(uiPayload, 8);
  // Length in 32bit words: 16 bits
  ib.read(uiLengthInWords, 16);

  rfc4585::RtcpApplicationLayerFeedback::ptr pRembFb2 = rfc4585::RtcpApplicationLayerFeedback::createRaw(uiVersion, uiPadding, uiLengthInWords);
  pRembFb2->read(ib);
  BOOST_CHECK_EQUAL(pRembFb->getSenderSSRC(), pRembFb2->getSenderSSRC());
  BOOST_CHECK_EQUAL(pRembFb->getSourceSSRC(), pRembFb2->getSourceSSRC());
  BOOST_CHECK_EQUAL(pRembFb->getTypeSpecific(), pRembFb2->getTypeSpecific());
  BOOST_CHECK_EQUAL(pRembFb->getVersion(), pRembFb2->getVersion());

  boost::optional<RtcpRembFb> remb2 = RtcpRembFb::parseFromAppData(pRembFb2->getAppData());
  BOOST_CHECK_EQUAL(remb2.is_initialized(), true);

  BOOST_CHECK_EQUAL(remb.getBps(), remb2->getBps());
  BOOST_CHECK_EQUAL(remb.getNumSsrcs(), remb2->getNumSsrcs());
  BOOST_CHECK_EQUAL(remb.getSsrcFb(), remb2->getSsrcFb());
}

} // test
} // experimental
} // rtp_plus_plus
