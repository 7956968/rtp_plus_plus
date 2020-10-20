#pragma once
#include <cpputil/GenericParameters.h>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtcpParser.h>
#include <rtp++/network/AddressDescriptorParser.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpHeaderExtension.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(MpRtpTest)
BOOST_AUTO_TEST_CASE(test_MpRtpPacketisation)
{
  uint32_t uiMpRtpExtmapId = 1;
  const mprtp::MpRtpSubflowRtpHeader subflowHeader(7, 19123);
  rfc5285::HeaderExtensionElement header = mprtp::MpRtpHeaderExtension::generateMpRtpHeaderExtension(uiMpRtpExtmapId, subflowHeader);

  mprtp::MpRtpHeaderParseResult result = mprtp::MpRtpHeaderExtension::parseMpRtpHeader(header);

  BOOST_CHECK_EQUAL(result.m_eType, mprtp::MpRtpHeaderParseResult::MPRTP_SUBFLOW_HEADER);
  mprtp::MpRtpSubflowRtpHeader subflowHeaderParsed = *result.SubflowHeader;
  BOOST_CHECK_EQUAL(subflowHeader.getFlowId(), subflowHeaderParsed.getFlowId());
  BOOST_CHECK_EQUAL(subflowHeader.getFlowSpecificSequenceNumber(), subflowHeaderParsed.getFlowSpecificSequenceNumber());
}

BOOST_AUTO_TEST_CASE(test_MpRtcpPacketisation)
{
  rfc3550::RtcpSr::ptr pSr0 = rfc3550::RtcpSr::create(0, 1, 2, 3, 4, 5);

  rfc3550::RtcpRr::ptr pRr0 = rfc3550::RtcpRr::create(12345);

  rfc3550::RtcpSr::ptr pSr1 = rfc3550::RtcpSr::create(6, 7, 8, 9, 10, 11);

  mprtp::MpRtcp::ptr pMpRtcp = mprtp::MpRtcp::create();
  const uint8_t uiFlow0 = 0;
  const uint8_t uiFlow1 = 1;
  pMpRtcp->addMpRtcpReport(uiFlow0, mprtp::SUBFLOW_SPECIFIC_REPORT, pSr0);
  pMpRtcp->addMpRtcpReport(uiFlow0, mprtp::SUBFLOW_SPECIFIC_REPORT, pRr0);

  pMpRtcp->addMpRtcpReport(uiFlow1, mprtp::SUBFLOW_SPECIFIC_REPORT, pSr1);

  OBitStream ob;
  ob << pMpRtcp.get();

  IBitStream ib(ob.data());

  RtpPacketiser packetiser;
  // register MPRTCP parsers
  std::unique_ptr<RtcpParserInterface> pRtcpParser = std::unique_ptr<mprtp::RtcpParser>( new mprtp::RtcpParser() );
  packetiser.registerRtcpParser(std::move(pRtcpParser));

  RtcpPacketBase::ptr pBaseRtcp = packetiser.parseRtcpPacket(ib);
  mprtp::MpRtcp::ptr pParsedMpRtcp = boost::static_pointer_cast<mprtp::MpRtcp>(pBaseRtcp);
  CompoundRtcpPacket vReports0 = pParsedMpRtcp->getRtcpReports(uiFlow0);
  BOOST_CHECK_EQUAL(vReports0.size(), 2);

  CompoundRtcpPacket vReports1 = pParsedMpRtcp->getRtcpReports(uiFlow1);
  BOOST_CHECK_EQUAL(vReports1.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_ProfileBasedReportManagerConstruction)
{
  const std::string sSDP_AVP = "v=0\n"\
  "o=- 1346075167783230 1 IN IP4 146.64.28.171\n"\
  "s=Session streamed by \"rtp_plus_plus\"\n"\
  "i=rtp_plus_plus\n"\
  "t=0 0\n"\
  "a=tool:rtp++\n"\
  "a=type:broadcast\n"\
  "a=control:*\n"\
  "a=source-filter: incl IN IP4 * 146.64.28.171\n"\
  "a=rtcp-unicast: reflection\n"\
  "a=range:npt=0-\n"\
  "a=x-qt-text-nam:Session streamed by \"rtp_plus_plus\"\n"\
  "m=video 5004 RTP/AVP 96\n"\
  "c=IN IP4 146.64.28.171\n"\
  "b=AS:1000\n"\
  "a=rtpmap:96 H265/90000\n"\
  "a=fmtp:96 packetization-mode=1;profile-level-id=304941;sprop-parameter-sets=J0IAFI1oFglE,AAAAAQ==\n"\
  "a=control:track1";

  const std::string sSDP_AVPF = "v=0\n"\
  "o=- 1346075167783230 1 IN IP4 146.64.28.171\n"\
  "s=Session streamed by \"rtp_plus_plus\"\n"\
  "i=rtp_plus_plus\n"\
  "t=0 0\n"\
  "a=tool:rtp++\n"\
  "a=type:broadcast\n"\
  "a=control:*\n"\
  "a=source-filter: incl IN IP4 * 146.64.28.171\n"\
  "a=rtcp-unicast: reflection\n"\
  "a=range:npt=0-\n"\
  "a=x-qt-text-nam:Session streamed by \"rtp_plus_plus\"\n"\
  "m=video 5004 RTP/AVPF 96\n"\
  "c=IN IP4 146.64.28.171\n"\
  "b=AS:1000\n"\
  "a=rtpmap:96 H265/90000\n"\
  "a=fmtp:96 packetization-mode=1;profile-level-id=304941;sprop-parameter-sets=J0IAFI1oFglE,AAAAAQ==\n"\
  "a=control:track1";

  const std::string sSDP_MPRTP = "v=0\n"\
  "o=- 1346075167783230 1 IN IP4 146.64.28.171\n"\
  "s=Session streamed by \"rtp_plus_plus\"\n"\
  "i=rtp_plus_plus\n"\
  "t=0 0\n"\
  "a=tool:rtp++\n"\
  "a=type:broadcast\n"\
  "a=control:*\n"\
  "a=source-filter: incl IN IP4 * 146.64.28.171\n"\
  "a=rtcp-unicast: reflection\n"\
  "a=range:npt=0-\n"\
  "a=x-qt-text-nam:Session streamed by \"rtp_plus_plus\"\n"\
  "m=video 5004 RTP/AVPF 96\n"\
  "c=IN IP4 146.64.28.171\n"\
  "b=AS:1000\n"\
  "a=rtpmap:96 H265/90000\n"\
  "a=fmtp:96 packetization-mode=1;profile-level-id=304941;sprop-parameter-sets=J0IAFI1oFglE,AAAAAQ==\n"\
  "a=control:track1\n"\
  "a=mprtp";

  boost::optional<rfc4566::SessionDescription> sdpAvp = rfc4566::SdpParser::parse(sSDP_AVP);
  assert (sdpAvp);

  boost::optional<rfc4566::SessionDescription> sdpAvpf = rfc4566::SdpParser::parse(sSDP_AVPF);
  assert (sdpAvpf);

  boost::optional<rfc4566::SessionDescription> sdpMprtp = rfc4566::SdpParser::parse(sSDP_MPRTP);
  assert (sdpMprtp);

  const std::string sNetworkConfig1 = "{"\
  "::0.0.0.0:49170, ::0.0.0.0:49171"\
  "}";
  const std::string sNetworkConfig2 = "{"\
  "::0.0.0.0:49172, ::0.0.0.0:49173"\
  "}";
  const std::string sNetworkConfig3 = "{"\
  "::0.0.0.0:49174, ::0.0.0.0:49175"\
  "}";

  boost::optional<InterfaceDescriptions_t> networkConf1 = AddressDescriptorParser::parse(sNetworkConfig1);
  assert (networkConf1);
  boost::optional<InterfaceDescriptions_t> networkConf2 = AddressDescriptorParser::parse(sNetworkConfig2);
  assert (networkConf2);
  boost::optional<InterfaceDescriptions_t> networkConf3 = AddressDescriptorParser::parse(sNetworkConfig3);
  assert (networkConf3);

  boost::asio::io_service ioService;
  rfc3550::SdesInformation sdesInfo;

  GenericParameters parameters;
  parameters.setUintParameter(app::ApplicationParameters::mtu, 1460);
  parameters.setUintParameter(app::ApplicationParameters::buf_lat, 200);

  /*
  // AVP
  boost::optional<RtpSessionParameters> rtpParametersAvp = MediaSessionFactory::createRtpSessionParameters(sdesInfo, *sdpAvp, *networkConf1, 0, false);
  rfc3550::RtpSessionBase::ptr pRtpSession = rfc3550::RtpSession::create(ioService, *rtpParametersAvp, parameters);
  pRtpSession->start();
  // make sure the correct profile was used
  const std::vector<std::unique_ptr<rfc3550::RtcpReportManager> >& vReportManagers = pRtpSession->getReportManagers();
  rfc3550::RtcpReportManager* pAvpManager = dynamic_cast<rfc3550::RtcpReportManager*>(vReportManagers[0].get());
  BOOST_CHECK_EQUAL(pAvpManager!= 0, true);
  pRtpSession->stop();

  // AVPF
  boost::optional<RtpSessionParameters> rtpParametersAvpf = MediaSessionFactory::createRtpSessionParameters(sdesInfo, *sdpAvpf, *networkConf2, 0, false);
  rfc3550::RtpSessionBase::ptr pRtpSessionAvpf = rfc3550::RtpSession::create(ioService, *rtpParametersAvpf, parameters);
  pRtpSessionAvpf->start();
  // make sure the correct profile was used
  const std::vector<std::unique_ptr<rfc3550::RtcpReportManager> >& vReportManagersAvpf = pRtpSessionAvpf->getReportManagers();
  rfc4585::RtcpReportManager* pAvpfManager = dynamic_cast<rfc4585::RtcpReportManager*>(vReportManagersAvpf[0].get());
  BOOST_CHECK_EQUAL(pAvpfManager!= 0, true);
  pRtpSessionAvpf->stop();

  // MPRTP AVPF
  boost::optional<RtpSessionParameters> rtpParametersMpRtp = MediaSessionFactory::createRtpSessionParameters(sdesInfo, *sdpMprtp, *networkConf3, 0, false);
  rfc3550::RtpSessionBase::ptr pMpRtpSession = mprtp::MpRtpSession::create(ioService, *rtpParametersMpRtp, parameters);
  pMpRtpSession->start();
  // make sure the correct profile was used
  const std::vector<std::unique_ptr<rfc3550::RtcpReportManager> >& vReportManagersMprtp = pMpRtpSession->getReportManagers();
  rfc4585::RtcpReportManager* pMpRtpManager = dynamic_cast<rfc4585::RtcpReportManager*>(vReportManagersMprtp[0].get());
  BOOST_CHECK_EQUAL(pMpRtpManager!= 0, true);
  pMpRtpSession->stop();
  */
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
