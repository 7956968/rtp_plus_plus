#include "CorePch.h"
#include <boost/exception/all.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/RtpSession.h>
#include <rtp++/RtpUtil.h>
#include <rtp++/experimental/RtcpHeaderExtension.h>
#include <rtp++/experimental/ExperimentalRtcpParser.h>
#include <rtp++/experimental/Scream.h>
#include <rtp++/media/h264/H264FormatDescription.h>
#include <rtp++/media/h265/H265FormatDescription.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpHeaderExtension.h>
#include <rtp++/mprtp/MpRtcpParser.h>
#include <rtp++/mprtp/MpRtpFeedbackManager.h>
#include <rtp++/mprtp/MpRtpSessionDatabase.h>
#include <rtp++/mprtp/MpRtcpReportManager.h>
#include <rtp++/network/MuxedUdpRtpNetworkInterface.h>
#include <rtp++/network/RtspAdapterRtpNetworkInterface.h>
#include <rtp++/network/SctpRtpNetworkInterface.h>
#include <rtp++/network/UdpRtpNetworkInterface.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3550/RtpHeader.h>
#include <rtp++/rfc3611/RtcpParser.h>
#include <rtp++/rfc3711/Rfc3711.h>
#include <rtp++/rfc4571/MuxedTcpRtpNetworkInterface.h>
#include <rtp++/rfc4571/PassiveTcpRtpNetworkInterface.h>
#include <rtp++/rfc4571/TcpRtpNetworkInterface.h>
#include <rtp++/rfc4585/FeedbackManager.h>
#include <rtp++/rfc4585/Rfc4585RtcpParser.h>
#include <rtp++/rfc4867/Rfc4867.h>
#include <rtp++/rfc4867/Rfc4867Packetiser.h>
#include <rtp++/rfc5506/Rfc5506.h>
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>
#include <rtp++/rfc5761/Rfc5761.h>
#include <rtp++/rfc5762/Rfc5762.h>
#include <rtp++/rfc6051/Rfc6051.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6184/Rfc6184Packetiser.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfc6190/Rfc6190Packetiser.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/rfchevc/RfchevcPacketiser.h>
#include <rtp++/sctp/SctpRtpPolicy.h>
#include <rtp++/TransmissionManager.h>

// #define DEBUG_OUTGOING_RTP

#ifndef EXIT_ON_TRUE
#define EXIT_ON_TRUE(condition, log_message) if ((condition)){\
  LOG(WARNING) << log_message;return false;}
#endif

namespace rtp_plus_plus
{

using media::MediaSample;

RtpSession::ptr RtpSession::create(boost::asio::io_service& rIoService,
                                       const RtpSessionParameters& rtpParameters,
                                       RtpReferenceClock& rtpReferenceClock,
                                       const GenericParameters &applicationParameters,
                                       bool bIsSender)
{
  return boost::make_shared<RtpSession>(boost::ref(rIoService), boost::ref(rtpParameters), boost::ref(rtpReferenceClock), applicationParameters, bIsSender);
}

RtpSession::ptr RtpSession::create(boost::asio::io_service& rIoService,
                                   ExistingConnectionAdapter& adapter,
                                   const RtpSessionParameters& rtpParameters,
                                   RtpReferenceClock& rtpReferenceClock,
                                   const GenericParameters &applicationParameters,
                                   bool bIsSender)
{
  return boost::make_shared<RtpSession>(boost::ref(rIoService), adapter, boost::ref(rtpParameters), boost::ref(rtpReferenceClock), applicationParameters, bIsSender);
}

RtpSession::RtpSession(boost::asio::io_service& ioService,
                           const RtpSessionParameters& rtpParameters,
                           RtpReferenceClock& rtpReferenceClock,
                           const GenericParameters& applicationParameters, bool bIsSender)
  :m_rIoService(ioService),
    m_parameters(rtpParameters),
    m_referenceClock(rtpReferenceClock),
    m_applicationParameters(applicationParameters),
    m_bIsSender(bIsSender),
    m_bTryExtractNtpTimestamp(false),
    m_eRapidSyncMode(RS_NONE),
    m_uiRapidSyncExtmapId(0),
#if 0
    m_pFeedbackManager(nullptr),
#endif
    m_state(SS_STOPPED),
    m_rtpSessionState(rtpParameters.getPayloadType()),
    m_uiByeSentCount(0),
    m_uiLastAUStartSN(0),
    m_uiLastAUPacketCount(0),
    m_uiRtcpRtpHeaderExtmapId(0),
    m_bIsMpRtpSession(mprtp::isMpRtpSession(rtpParameters)),
    m_uiMpRtpExtmapId(0)
{
  VLOG(5) << "Constructor";
  configureApplicationParameters(applicationParameters);
}

RtpSession::RtpSession(boost::asio::io_service& ioService,
                       ExistingConnectionAdapter& adapter,
                       const RtpSessionParameters& rtpParameters,
                       RtpReferenceClock& rtpReferenceClock,
                       const GenericParameters& applicationParameters, bool bIsSender)
  :m_rIoService(ioService),
    m_parameters(rtpParameters),
    m_referenceClock(rtpReferenceClock),
    m_applicationParameters(applicationParameters),
    m_existingConnectionAdapter(adapter),
    m_bIsSender(bIsSender),
    m_bTryExtractNtpTimestamp(false),
    m_eRapidSyncMode(RS_NONE),
    m_uiRapidSyncExtmapId(0),
#if 0
    m_pFeedbackManager(nullptr),
#endif
    m_state(SS_STOPPED),
    m_rtpSessionState(rtpParameters.getPayloadType()),
    m_uiByeSentCount(0),
    m_uiLastAUStartSN(0),
    m_uiLastAUPacketCount(0),
    m_uiRtcpRtpHeaderExtmapId(0),
    m_bIsMpRtpSession(mprtp::isMpRtpSession(rtpParameters)),
    m_uiMpRtpExtmapId(0)
{
  VLOG(5) << "Constructor(ExistingConnectionAdapter)";
  configureApplicationParameters(applicationParameters);
}

RtpSession::~RtpSession()
{
  m_rtpSessionStatistics.finalise();
  VLOG(2) << "RTP statistics:\n" << m_rtpSessionStatistics.getOverallStatistic();
}

void RtpSession::configureApplicationParameters(const GenericParameters& applicationParameters)
{
  boost::optional<uint32_t> uiForceRtpSn = applicationParameters.getUintParameter(app::ApplicationParameters::rtp_sn);
  if (uiForceRtpSn)
  {
    VLOG(2) << "Forcing RTP sequence number to " << *uiForceRtpSn;
    m_rtpSessionState.overrideRtpSequenceNumber(static_cast<uint16_t>(*uiForceRtpSn));
  }
  boost::optional<uint32_t> uiForceRtpTs = applicationParameters.getUintParameter(app::ApplicationParameters::rtp_ts);
  if (uiForceRtpTs)
  {
    VLOG(2) << "Forcing RTP timestamp base to " << *uiForceRtpTs;
    m_rtpSessionState.overrideRtpTimestampBase(static_cast<uint32_t>(*uiForceRtpTs));
  }
  boost::optional<uint32_t> uiForceRtpSSRC = applicationParameters.getUintParameter(app::ApplicationParameters::rtp_ssrc);
  if (uiForceRtpSSRC)
  {
    VLOG(2) << "Forcing RTP SSRC to " << *uiForceRtpSSRC;
    m_rtpSessionState.overrideSSRC(static_cast<uint32_t>(*uiForceRtpSSRC));
  }
#if 0
  boost::optional<bool> bTryExtractNtpTimestamp = applicationParameters.getBoolParameter(app::ApplicationParameters::extract_ntp_ts);
  if (bTryExtractNtpTimestamp)
  {
    VLOG(2) << "Trying to extract NTP timestamp";
    m_bTryExtractNtpTimestamp = *bTryExtractNtpTimestamp;
  }
#else
  // set it if the header extension is in use

#endif
  if (m_parameters.isRetransmissionEnabled())
  {
    VLOG(2) << "Setting RTX payload type";
    m_rtpSessionState.setRtxPayloadType(m_parameters.getRetransmissionPayloadType());
  }

  boost::optional<uint32_t> uiRapidSyncMode = applicationParameters.getUintParameter(app::ApplicationParameters::rapid_sync_mode);
  if (uiRapidSyncMode)
  {
    VLOG(2) << "Rapid Sync Mode: " << *uiRapidSyncMode;
    m_eRapidSyncMode = static_cast<RapidSyncMode>(*uiRapidSyncMode);
  }

  // go through RtpSessionParameters and register extension header handlers
  // the handlers will be used to process incoming RTP packets before
  // processing them through the SessionDatabase and RtpSessionManager
  // TODO
  boost::optional<bool> bSummariseStatistics = applicationParameters.getBoolParameter(app::ApplicationParameters::rtp_summarise_stats);
  if (bSummariseStatistics && *bSummariseStatistics)
  {
    m_rtpSessionStatistics.enableStatisticsSummary();
  }
}

bool RtpSession::isActive() const
{
  return m_state == SS_STARTED;
}

bool RtpSession::isShuttingDown() const
{
  return m_state == SS_SHUTTING_DOWN;
}

boost::system::error_code RtpSession::start()
{
  if (m_state == SS_STOPPED)
  {
    if (initialise())
    {
      m_state = SS_STARTED;
      return boost::system::error_code();
    }
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  else
  {
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

boost::system::error_code RtpSession::stop()
{
  if (m_state == SS_STARTED)
  {
    VLOG(10) << "Shutting down RTP session";
    m_state = SS_SHUTTING_DOWN;
    VLOG(10) << "Shutting down RTCP";
    shutdownRtcp();

    VLOG(10) << "Waiting for shutdown to complete";
    // FIXME: this should only be reset once the shutdown is complete
    // i.e. all asynchronous handlers have been called
    // Commenting this out: for now the solution is to manually call reset
    // which will reset all session state
    // m_state = SS_STOPPED;
    return boost::system::error_code();
  }
  else
  {
    // This can happen if stop has already been called
    VLOG(5) << "Invalid state to call stop: " << m_state;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

std::vector<RtpPacket> RtpSession::doPacketise(const MediaSample& mediaSample, const uint32_t uiRtpTimestamp, const uint32_t uiNtpMws, const uint32_t uiNtpLws)
{
  if (m_state == SS_STARTED)
  {
    assert(m_pPayloadPacketiser);

    std::vector<RtpPacket> rtpPackets = m_pPayloadPacketiser->packetise(mediaSample);

#ifdef DEBUG_PACKETISER
    // This could be the case when the packetiser implementation aggregates media samples
    LOG_IF(WARNING, rtpPackets.empty()) << "RTP packetizer generated 0 packets";
#endif

    // set RTP header values
    setRtpHeaderValues(rtpPackets, uiRtpTimestamp, -1, uiNtpMws, uiNtpLws);

    return rtpPackets;
  }
  else if (m_state == SS_STOPPED)
  {
    LOG_FIRST_N(WARNING, 1) << "Session has not been started.";
  }
  else if (m_state == SS_SHUTTING_DOWN)
  {
    LOG_FIRST_N(INFO, 1) << "Shutting down, not sending any more samples.";
  }
  return std::vector<RtpPacket>();
}

std::vector<RtpPacket> RtpSession::packetise(const MediaSample& mediaSample)
{
  uint32_t uiNtpMws = 0;
  uint32_t uiNtpLws = 0;

  // Note: the timestamp will be ignored if the reference clock is in wall clock mode
  uint32_t uiRtpTimestamp = m_referenceClock.getRtpTimestamp(m_parameters.getRtpTimestampFrequency(),
                                                                  m_rtpSessionState.getRtpTimestampBase(),
                                                                  mediaSample.getStartTime(),
                                                                  uiNtpMws,
                                                                  uiNtpLws);
  return doPacketise(mediaSample, uiRtpTimestamp, uiNtpMws, uiNtpLws);
}

std::vector<RtpPacket> RtpSession::packetise(const MediaSample& mediaSample, const uint32_t uiRtpTimestamp)
{
  // when the RTP timestamp is passed in we don't know what the related NTP timestamp is
  uint32_t uiDummyNtpMws = 0, uiDummyNtpLws = 0;
  return doPacketise(mediaSample, uiRtpTimestamp, uiDummyNtpMws, uiDummyNtpLws);
}

void RtpSession::setRtpHeaderValues(std::vector<RtpPacket>& rtpPackets, uint32_t uiRtpTimestamp,
                                    int32_t iFlowHint, const uint32_t uiNtpMws, const uint32_t uiNtpLws)
{
  if (!rtpPackets.empty())
  {
    for (auto& rtpPacket: rtpPackets)
    {
      // set RTP attributes
      rfc3550::RtpHeader& header = rtpPacket.getHeader();
      header.setPayloadType( m_rtpSessionState.getCurrentPayloadType() );
      header.setSequenceNumber(m_rtpSessionState.getNextSequenceNumber());
      header.setRtpTimestamp(uiRtpTimestamp);
      header.setSSRC(m_rtpSessionState.getSSRC());
      switch (m_eRapidSyncMode)
      {
        case RS_EVERY_RTP_PACKET:
        {
          if (uiNtpMws != 0 && uiNtpLws != 0)
          {
            rfc5285::HeaderExtensionElement rapidSyncHeader = rfc6051::RtpSynchronisationExtensionHeader::create(m_uiRapidSyncExtmapId, uiNtpMws, uiNtpLws);
            rtpPacket.addRtpHeaderExtension(rapidSyncHeader);
          }
          else
          {
            VLOG(6) << "Invalid NTP timestamps - not inserting rapid sync header";
          }
          break;
        }
      }
    }

    switch (m_eRapidSyncMode)
    {
      case RS_EVERY_SAMPLE:
      {
        if (uiNtpMws != 0 && uiNtpLws != 0)
        {
          rfc5285::HeaderExtensionElement rapidSyncHeader = rfc6051::RtpSynchronisationExtensionHeader::create(m_uiRapidSyncExtmapId, uiNtpMws, uiNtpLws);
          rtpPackets[0].addRtpHeaderExtension(rapidSyncHeader);
        }
        else
        {
          VLOG(6) << "Invalid NTP timestamps - not inserting rapid sync header";
        }
        break;
      }
    }
  }
}

std::vector<std::vector<uint16_t> > RtpSession::getRtpPacketisationInfo() const
{
  assert(m_pPayloadPacketiser);
  const std::vector<std::vector<uint32_t> >& vInfo = m_pPayloadPacketiser->getPacketisationInfo();
  // map info from packetiser into sequence numbers
  std::vector<std::vector<uint16_t> > rtpPacketisationInfo(vInfo.size());
  for (size_t i = 0; i < vInfo.size(); ++i)
  {
    const std::vector<uint32_t>& vRtpPacketsForMediaSample = vInfo[i];
    for (size_t j = 0; j < vRtpPacketsForMediaSample.size(); ++j)
    {
      uint16_t uiOffset = vRtpPacketsForMediaSample[j];
      rtpPacketisationInfo[i].push_back(m_uiLastAUStartSN + uiOffset);
    }
  }
  return rtpPacketisationInfo;
}

std::vector<RtpPacket> RtpSession::doPacketise(const std::vector<MediaSample>& vMediaSamples, const uint32_t uiRtpTimestamp, const uint32_t uiNtpMws, const uint32_t uiNtpLws)
{
  if (m_state == SS_STARTED)
  {
    assert(m_pPayloadPacketiser);
    assert(!vMediaSamples.empty());

    std::vector<RtpPacket> rtpPackets = m_pPayloadPacketiser->packetise(vMediaSamples);

    // This could be the case when the packetiser implementation aggregates media samples
    LOG_IF(WARNING, rtpPackets.empty()) << "RTP packetizer generated 0 packets";

    // set RTP header values
    setRtpHeaderValues(rtpPackets, uiRtpTimestamp, -1, uiNtpMws, uiNtpLws);

    // store packetisation info
    if (!rtpPackets.empty())
    {
      m_uiLastAUStartSN = rtpPackets[0].getSequenceNumber();
      m_uiLastAUPacketCount = rtpPackets.size();
    }

    return rtpPackets;
  }
  else if (m_state == SS_STOPPED)
  {
    LOG_FIRST_N(WARNING, 1) << "Session has not been started.";
  }
  else if (m_state == SS_SHUTTING_DOWN)
  {
    LOG_FIRST_N(INFO, 1) << "Shutting down, not sending any more samples.";
  }
  return std::vector<RtpPacket>();
}

std::vector<RtpPacket> RtpSession::packetise(const std::vector<MediaSample>& vMediaSamples)
{
  uint32_t uiNtpMws = 0;
  uint32_t uiNtpLws = 0;

  // Note: the timestamp will be ignored if the reference clock is in wall clock mode
  uint32_t uiRtpTimestamp = m_referenceClock.getRtpTimestamp(m_parameters.getRtpTimestampFrequency(),
                                                             m_rtpSessionState.getRtpTimestampBase(),
                                                             vMediaSamples[0].getStartTime(),
                                                             uiNtpMws,
                                                             uiNtpLws);

  return doPacketise(vMediaSamples, uiRtpTimestamp, uiNtpMws, uiNtpLws);

}

std::vector<RtpPacket> RtpSession::packetise(const std::vector<MediaSample>& vMediaSamples, const uint32_t uiRtpTimestamp)
{
  // when the RTP timestamp is passed in we don't know what the related NTP timestamp is
  uint32_t uiDummyNtpMws = 0, uiDummyNtpLws = 0;
  return doPacketise(vMediaSamples, uiRtpTimestamp, uiDummyNtpMws, uiDummyNtpLws);
}

mprtp::MpRtpSubflowRtpHeader RtpSession::generateMpRtpExtensionHeader(uint16_t uiSubflowId, bool bRtx)
{
  FlowMap_t::iterator it = m_MpRtp.m_mFlows.find(uiSubflowId);
  assert( it != m_MpRtp.m_mFlows.end());
  mprtp::MpRtpFlow& flow = it->second;

  // At this point we need to insert the MPRTP header
  uint16_t uiFlowSpecificSequenceNumber = bRtx? flow.getNextRtxSN() : flow.getNextSN();
#if 0
  if (bRtx) VLOG(2) << "Generating MPRTP header for flow " << uiSubflowId << " FSSN: " << uiFlowSpecificSequenceNumber;
#endif
  return mprtp::MpRtpSubflowRtpHeader(uiSubflowId, uiFlowSpecificSequenceNumber);
}

const EndPoint& RtpSession::lookupEndPoint(uint16_t uiSubflowId)
{
  FlowMap_t::iterator it = m_MpRtp.m_mFlows.find(uiSubflowId);
  assert( it != m_MpRtp.m_mFlows.end());
  mprtp::MpRtpFlow& flow = it->second;
  return flow.getRemoteEndPoint().first;
}

void RtpSession::handleMpRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader)
{
  VLOG(15) << "Handling incoming MPRTP extension header";
  mprtp::MpRtpHeaderParseResult result = mprtp::MpRtpHeaderExtension::parseMpRtpHeader(extensionHeader);
  if (result.m_eType == mprtp::MpRtpHeaderParseResult::MPRTP_SUBFLOW_HEADER)
  {
    RtpPacket& packet = const_cast<RtpPacket&>(rtpPacket);
    assert(result.SubflowHeader);
    packet.setMpRtpSubflowHeader(*result.SubflowHeader);
  }
  else
  {
    LOG(WARNING) << "Unexpected MPRTP extension header type";
  }
}

void RtpSession::sendRtpPacket(const RtpPacket& rtpPacket, uint32_t uiSubflowId)
{
  if (m_bIsMpRtpSession)
  {
    auto it = m_MpRtp.m_mFlowInterfaceMap.find(uiSubflowId);
    if (it == m_MpRtp.m_mFlowInterfaceMap.end())
    {
      LOG(WARNING) << "Invalid subflow ID: " << uiSubflowId << " setting to 0";
      uiSubflowId = 0;
      assert(m_MpRtp.m_mFlowInterfaceMap.find(0) != m_MpRtp.m_mFlowInterfaceMap.end());
    }

    uint32_t uiInterfaceIndex = m_MpRtp.m_mFlowInterfaceMap[uiSubflowId];
    // in the case of a retransmission the extension header will already have been set.
    // update it if necessary
    if (rtpPacket.getMpRtpSubflowHeader())
    {
      mprtp::MpRtpSubflowRtpHeader subflowHeader = generateMpRtpExtensionHeader(uiSubflowId, true);
      RtpPacket& packet = const_cast<RtpPacket&>(rtpPacket);
      packet.setMpRtpSubflowHeader(subflowHeader);
      rfc5285::HeaderExtensionElement extensionHeader = mprtp::MpRtpHeaderExtension::generateMpRtpHeaderExtension(m_uiMpRtpExtmapId, subflowHeader);
      packet.getHeaderExtension().removeHeaderExtension(m_uiMpRtpExtmapId);
      packet.addRtpHeaderExtension(extensionHeader);
      VLOG(5) << LOG_MODIFY_WITH_CARE
              << " RTX Packet sent SN: " << rtpPacket.getSequenceNumber()
              << " Flow Id: " << uiSubflowId
              << " FSSN: " << subflowHeader.getFlowSpecificSequenceNumber()
              << " Time: " << boost::posix_time::microsec_clock::universal_time();
    }
    else
    {
      // insert MPRTP subflow header
      mprtp::MpRtpSubflowRtpHeader subflowHeader = generateMpRtpExtensionHeader(uiSubflowId);
      RtpPacket& packet = const_cast<RtpPacket&>(rtpPacket);
      packet.setMpRtpSubflowHeader(subflowHeader);
      rfc5285::HeaderExtensionElement extensionHeader = mprtp::MpRtpHeaderExtension::generateMpRtpHeaderExtension(m_uiMpRtpExtmapId, subflowHeader);
      packet.addRtpHeaderExtension(extensionHeader);
      VLOG(5) << LOG_MODIFY_WITH_CARE
              << " Packet sent SN: " << rtpPacket.getSequenceNumber()
              << " Flow Id: " << uiSubflowId
              << " FSSN: " << subflowHeader.getFlowSpecificSequenceNumber()
              << " Time: " << boost::posix_time::microsec_clock::universal_time();
    }
    // last method for RtpSessionManager to modify packet
    const EndPoint& ep = lookupEndPoint(uiSubflowId);
    if (m_beforeOutgoingRtp) m_beforeOutgoingRtp(rtpPacket, ep);
    m_vRtpInterfaces[uiInterfaceIndex]->send(rtpPacket, ep);
  }
  else
  {
    // HACK for SCTP: the RtpPacket contains the ID of the SCTP channel
    // The SCTP network interface looks for this id in the end point
    EndPoint endpoint = m_parameters.getRemoteEndPoint(0).first;
    // sending packets as fast as possible
    endpoint.setId(rtpPacket.getId());
    VLOG(6) << "Sending RTP Packet to " << endpoint << " RTP SN " << rtpPacket.getSequenceNumber() << " size: " << rtpPacket.getPayloadSize();
    if (m_beforeOutgoingRtp) m_beforeOutgoingRtp(rtpPacket, endpoint);
    m_vRtpInterfaces[0]->send(rtpPacket, endpoint);
  }
}

// for backwards compatibility
void RtpSession::sendRtpPackets(const std::vector<RtpPacket>& rtpPackets)
{
  if (m_bIsMpRtpSession)
  {
    for (auto& rtpPacket: rtpPackets)
    {
      // HACK: use id as flow selector
      sendRtpPacket(rtpPacket, rtpPacket.getId());
    }
  }
  else
  {
    // HACK for SCTP: the RtpPacket contains the ID of the SCTP channel
    // The SCTP network interface looks for this id in the end point
    EndPoint endpoint = m_parameters.getRemoteEndPoint(0).first;
    // sending packets as fast as possible
    for (auto& rtpPacket: rtpPackets)
    {
      endpoint.setId(rtpPacket.getId());
      if (m_beforeOutgoingRtp) m_beforeOutgoingRtp(rtpPacket, endpoint);
      m_vRtpInterfaces[0]->send(rtpPacket, endpoint);
    }
  }
}

std::vector<MediaSample> RtpSession::depacketise(const RtpPacketGroup& rtpPacketGroup)
{
  return m_pPayloadPacketiser->depacketize(rtpPacketGroup);
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfaces(const RtpSessionParameters& rtpParameters,
                                                                             const GenericParameters& applicationParameters)
{
  // first check for existing parameters
  if (m_existingConnectionAdapter.reuseRtspConnection())
  {
    // NOTE: not sure how this ties up with the MPRTP code....
    return createRtpNetworkInterfacesFromExistingRtspConnection(m_existingConnectionAdapter, rtpParameters, applicationParameters);
  }
  else
  {
    // adding support for secure RTP if ENABLE_LIB_SRTP is defined
    if (rfc3711::isSecureProfile(rtpParameters))
    {
  #ifndef ENABLE_LIB_SRTP
      // early exit if there is no SRTP support
      return std::vector<RtpNetworkInterface::ptr>();
  #endif
    }

    if (!m_bIsMpRtpSession)
    {
      return createRtpNetworkInterfacesSinglePath(rtpParameters, applicationParameters);
    }
    else
    {
      return createRtpNetworkInterfacesMultiPath(rtpParameters, applicationParameters);
    }
  }
}

void RtpSession::configureNetworkInterface(RtpNetworkInterface::ptr& pRtpInterface,
                                           const RtpSessionParameters &rtpParameters,
                                           const GenericParameters& /*applicationParameters*/)
{
  if (rfc3711::isSecureProfile(rtpParameters))
  {
    pRtpInterface->secureRtp(true);
  }

  if (rtpParameters.isXrEnabled())
  {
    pRtpInterface->registerRtcpParser( rfc3611::RtcpParser::create() );
  }

  if (rfc5506::supportsReducedRtcpSize(rtpParameters))
  {
    LOG(INFO) << "Allowing reduced size RTCP validation rules";
    pRtpInterface->setRtcpValidator( rfc5506::RtcpValidator::create() );
  }

  // check if this is AVPF
  if (rfc4585::isAudioVisualProfileWithFeedback(rtpParameters))
  {
    pRtpInterface->registerRtcpParser( rfc4585::RtcpParser::create() );
  }

  if (mprtp::isMpRtpSession(rtpParameters))
  {
    // register MPRTCP parsers
    pRtpInterface->registerRtcpParser(std::move(mprtp::RtcpParser::create()));

    // register MPRTP extension header parser
    boost::optional<rfc5285::Extmap> extmap = m_parameters.lookupExtmap(mprtp::EXTENSION_NAME_MPRTP);
    if (!extmap)
    {
      LOG(WARNING) << "Couldn't lookup MPRTP extmap in RTP session parameters!";
    }
    else
    {
      m_uiMpRtpExtmapId = extmap->getId();
      VLOG(5) << "Registering MPRTP extension header handler";
      registerExtensionHeaderHandler(extmap->getId(), boost::bind(&RtpSession::handleMpRtpExtensionHeader, this, _1, _2));
    }
  }

  // scream
  if (experimental::supportsScreamFb(rtpParameters))
  {
    // register scream parsers
    pRtpInterface->registerRtcpParser(std::move(experimental::RtcpParser::create()));
  }

  // nada
  if (experimental::supportsNadaFb(rtpParameters))
  {
    // register scream parsers
    pRtpInterface->registerRtcpParser(std::move(experimental::RtcpParser::create()));
  }

  // rapid sync header parser
  boost::optional<rfc5285::Extmap> extmap = m_parameters.lookupExtmap(rfc6051::EXTENSION_NAME_RTP_NTP_64);
  if (!extmap)
  {
    VLOG(5) << "No 64-bit rapid sync extmap in RTP session parameters.";
  }
  else
  {
    m_uiRapidSyncExtmapId = extmap->getId();
    m_bTryExtractNtpTimestamp = true;
    VLOG(5) << "Registering 64-bit rapid sync extension header handler for id " << m_uiRapidSyncExtmapId;
    registerExtensionHeaderHandler(extmap->getId(), boost::bind(&RtpSession::handleRapidSyncRtpExtensionHeader, this, _1, _2));
  }

  // rtcp rtp header ext
  extmap = m_parameters.lookupExtmap(experimental::EXTENSION_NAME_RTCP_HEADER_EXT);
  if (extmap)
  {
    VLOG(5) << "Registering experimental RTCP RTP extension header handler.";
    m_uiRtcpRtpHeaderExtmapId = extmap->getId();
    registerExtensionHeaderHandler(extmap->getId(), boost::bind(&RtpSession::handleRtcpRtpExtensionHeader, this, _1, _2));
  }
}

void RtpSession::handleRtcpRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader)
{
  assert(!m_vRtpInterfaces.empty());
  // just using packetiser from first RTP interface
  RtpPacketiser& rtpPacketiser = m_vRtpInterfaces[0]->getRtpPacketiser();
  // parse RTCP and handle same as new RTCP
  CompoundRtcpPacket rtcp = experimental::RtcpHeaderExtension::parse(extensionHeader, rtpPacketiser);
  LOG_FIRST_N(INFO, 1) << "TODO: get correct endpoint? E.g. could set ep on rtpPacket and then retrieve it from there";
  EndPoint ep;
  onIncomingRtcp(rtcp, ep);
}

void RtpSession::handleRapidSyncRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader)
{
  boost::optional<uint64_t> ntpTs = rfc6051::RtpSynchronisationExtensionHeader::parseRtpSynchronisationExtensionHeader(extensionHeader);
  if (ntpTs)
  {
    uint64_t uiNtpTs = *ntpTs;
    // NTP arrival timestamp of RTP packet
    uint64_t uiNtpArrival = rtpPacket.getNtpArrivalTime();
    // for very small RTTs this can be negative
    int64_t iDiff = uiNtpArrival - uiNtpTs;
    uint64_t uiDiff = (iDiff > 0) ? uiNtpArrival - uiNtpTs : 0;
    uint32_t uiSeconds = (uiDiff >> 32);
    uint32_t uiMicroseconds = (uiDiff & 0xFFFFFFFF);
    double dMicroseconds = (uiMicroseconds / (double)UINT_MAX);
#if 0
    VLOG(2) << "SN: " << rtpPacket.getSequenceNumber() << " NTP NOW: " << hex((uint32_t)(uiNtpArrival >> 32)) << "." << hex((uint32_t)uiNtpArrival)
      << " Sample: " << hex(uiMswSample) << "." << hex(uiLswSample)
      << " One way RTT: " << uiSeconds + dMicroseconds << "s";
#endif
    double dOwd = uiSeconds + dMicroseconds;
    RtpPacket& packet = const_cast<RtpPacket&>(rtpPacket);
    packet.setOwdSeconds(dOwd);
  }
  else
  {
    VLOG(6) << "Failed to parse RTP rapid sync extension header";
  }
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfacesFromExistingRtspConnection(ExistingConnectionAdapter adapter,
                                                                                                   const RtpSessionParameters& rtpParameters,
                                                                                                   const GenericParameters& applicationParameters)
{
  std::vector<RtpNetworkInterface::ptr> vRtpInterfaces;
  assert(adapter.reuseRtspConnection());
  uint32_t uiRtpChannel = m_parameters.getTransport().getInterleavedStart();
  std::unique_ptr<RtpNetworkInterface> pRtpInterface = std::unique_ptr<RtspAdapterRtpNetworkInterface>(new RtspAdapterRtpNetworkInterface(adapter.getRtspClientConnection(), uiRtpChannel, uiRtpChannel + 1));
  vRtpInterfaces.push_back(std::move(pRtpInterface));
  return vRtpInterfaces;
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfacesSinglePath(const RtpSessionParameters& rtpParameters,
                                                                                       const GenericParameters& applicationParameters)
{
  std::vector<RtpNetworkInterface::ptr> vRtpInterfaces;
  if (rtpParameters.useRtpOverUdp())
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    VLOG(1) << "ADDR/RTP/RTCP " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

    std::unique_ptr<RtpNetworkInterface> pRtpInterface;
    if (rtpEp.getPort() != rtcpEp.getPort())
    {
      VLOG(2) << "Setting up non-multiplexed UDP RTP/RTCP transport";

      if (m_existingConnectionAdapter.usePortManager())
      {
        // lookup previously created interface
        PortAllocationManager* pPortManager = m_existingConnectionAdapter.getPortManager();
        assert(pPortManager);
        std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket = pPortManager->getBoundUdpSocket(rtpEp.getAddress(), rtpEp.getPort());
        if (!pRtpSocket)
        {
          LOG(WARNING) << "Failed to lookup existing RTP port for " << rtpEp;
          return std::vector<RtpNetworkInterface::ptr>();
        }
        std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket = pPortManager->getBoundUdpSocket(rtcpEp.getAddress(), rtcpEp.getPort());
        if (!pRtcpSocket)
        {
          LOG(WARNING) << "Failed to lookup existing RTCP port for " << rtcpEp;
          return std::vector<RtpNetworkInterface::ptr>();
        }
        pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp, std::move(pRtpSocket), std::move(pRtcpSocket)));
      }
      else
      {
        boost::system::error_code ec;
        pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp, ec));
        if (ec)
        {
          LOG(WARNING) << "Error constructing RTP interfaces: " << ec.message();
          return std::vector<RtpNetworkInterface::ptr>();
        }
      }
    }
    else
    {
      // make sure that the rtcp-mux attribute has been defined
      if (rtpParameters.hasAttribute(rfc5761::RTCP_MUX))
      {
        VLOG(2) << "Setting up multiplexed RTP/RTCP transport";

        if (m_existingConnectionAdapter.usePortManager())
        {
          // lookup previously created interface
          PortAllocationManager* pPortManager = m_existingConnectionAdapter.getPortManager();
          assert(pPortManager);
          std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket = pPortManager->getBoundUdpSocket(rtpEp.getAddress(), rtpEp.getPort());
          if (!pRtpRtcpSocket)
          {
            LOG(WARNING) << "Failed to lookup existing RTP/RTCP socket for " << rtpEp;
            return std::vector<RtpNetworkInterface::ptr>();
          }
          pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>(new MuxedUdpRtpNetworkInterface(m_rIoService, rtpEp, std::move(pRtpRtcpSocket)));
        }
        else
        {
          boost::system::error_code ec;
          pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>(new MuxedUdpRtpNetworkInterface(m_rIoService, rtpEp, ec));
          if (ec)
          {
            LOG(WARNING) << "Error constructing RTP/RTCP interfaces: " << ec.message();
            return std::vector<RtpNetworkInterface::ptr>();
          }
        }
      }
      else
      {
        VLOG(2) << "Error setting up multiplexed UDP RTP/RTCP transport - RTCP-mux not set";
        return std::vector<RtpNetworkInterface::ptr>(); // notify base class of failure to create RTP network interface
      }
    }
    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
  else if (rtpParameters.useRtpOverTcp())
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    std::unique_ptr<RtpNetworkInterface> pRtpInterface;

    const EndPoint remoteRtpEp = rtpParameters.getRemoteEndPoint(0).first;
    if (rtpParameters.hasAttribute(rfc5761::RTCP_MUX))
    {
      VLOG(2) << "Setting up multiplexed TCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc4571::MuxedTcpRtpNetworkInterface>(new rfc4571::MuxedTcpRtpNetworkInterface(m_rIoService, rtpEp, remoteRtpEp, rtpParameters.isActiveConnection()));
    }
    else if (rtpParameters.isActiveConnection())
    {
      VLOG(2) << "Setting up active TCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc4571::TcpRtpNetworkInterface>(new rfc4571::TcpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp));
    }
    else
    {
      VLOG(2) << "Setting up passive TCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc4571::PassiveTcpRtpNetworkInterface>( new rfc4571::PassiveTcpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp));
    }
    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
#ifdef ENABLE_ASIO_DCCP
  else if (rtpParameters.useRtpOverDccp())
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    std::unique_ptr<RtpNetworkInterface> pRtpInterface;

    const EndPoint remoteRtpEp = rtpParameters.getRemoteEndPoint(0).first;
    if (rtpParameters.hasAttribute(rfc5761::RTCP_MUX))
    {
      VLOG(2) << "Setting up multiplexed DCCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc5762::MuxedDccpRtpNetworkInterface>( new rfc5762::MuxedDccpRtpNetworkInterface (m_rIoService, rtpEp, remoteRtpEp, rtpParameters.isActiveConnection() ));
    }
    else if (rtpParameters.isActiveConnection())
    {
      VLOG(2) << "Setting up active DCCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc5762::DccpRtpNetworkInterface>(new rfc5762::DccpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp));
    }
    else
    {
      VLOG(2) << "Setting up passive DCCP RTP/RTCP transport";
      pRtpInterface = std::unique_ptr<rfc5762::PassiveDccpRtpNetworkInterface>(new rfc5762::PassiveDccpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp));
    }
    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
#endif
  else if (rtpParameters.useRtpOverSctp())
  {
#ifdef ENABLE_SCTP_USERLAND
    using namespace sctp;
    // NOTE: it is important that the following code is in sync with the code in the SctpSvcRtpSessionManager
    // since the policies must be the same for the manager to be able to select the appropriate channel
    // per packet
    SctpRtpPolicy policy = SctpRtpPolicy::createDefaultPolicy();

    // get SCTP SVC application parameters
    boost::optional<std::vector<std::string> > vPolicies
        = applicationParameters.getStringsParameter(app::ApplicationParameters::sctp_policies);
    if (vPolicies)
    {
      boost::optional<SctpRtpPolicy> customPolicy = SctpRtpPolicy::create(*vPolicies);
      if (customPolicy)
      {
        policy = *customPolicy;
      }
      else
      {
        LOG(WARNING) << "Failed to parse SCTP policies: using default policies";
      }
    }

    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    // Note: RTCP port is irrelevant
    const EndPoint remoteRtpEp = rtpParameters.getRemoteEndPoint(0).first;
//    sctp::SctpRtpChannelDescriptor sctpRtpChannelDescriptor;
//    // Channel 0: RTCP
//    const PolicyDescriptor& rtcpPolicy = policy.getRtcpPolicy();
//    sctpRtpChannelDescriptor.addChannel(sctp::SctpChannelPolicy(rtcpPolicy.Unordered, rtcpPolicy.SctpPrPolicy, rtcpPolicy.PrVal));
//    // Channel 1: RTP
//    const PolicyDescriptor& rtpPolicy = policy.getRtpPolicy();
//    sctpRtpChannelDescriptor.addChannel(sctp::SctpChannelPolicy(rtpPolicy.Unordered, rtpPolicy.SctpPrPolicy, rtpPolicy.PrVal));

//    // Add other RTP channels
//    const std::vector<PolicyDescriptor>& rtpPolicies = policy.getAdditionalRtpPolicies();
//    for (auto& policy: rtpPolicies)
//    {
//      // only add static descriptors: RTT-based ones have to be added at run-time
//      if (policy.RttFactor == 0.0)
//        sctpRtpChannelDescriptor.addChannel(sctp::SctpChannelPolicy(policy.Unordered, policy.SctpPrPolicy, policy.PrVal));
//    }

    // HACK to not connect SCTP in RtpPacketiser app
    boost::optional<bool> dont_connect
        = applicationParameters.getBoolParameter(app::ApplicationParameters::sctp_dont_connect);
    bool bConnect = (dont_connect ? false : true);

    VLOG(2) << "Setting up SCTP RTP/RTCP transport";
    std::unique_ptr<RtpNetworkInterface> pRtpInterface = std::unique_ptr<sctp::SctpRtpNetworkInterface>(
          new sctp::SctpRtpNetworkInterface(rtpEp,
                                            remoteRtpEp,
                                            rtpParameters.isActiveConnection(),
                                            policy,
                                            bConnect,
                                            applicationParameters));

    vRtpInterfaces.push_back(std::move(pRtpInterface));
#else
    LOG(WARNING) << "SCTP is not supported without the SCTP user land stack";
#endif
  }

  return vRtpInterfaces;
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfacesMultiPath(const RtpSessionParameters& rtpParameters,
                                                                                      const GenericParameters& /*applicationParameters*/)
{
  VLOG(2) << "Creating multipath network interfaces";
  // check if there are bindings
  std::map<int, int> mBindMap;
  // check for MPRTP bind parameters: assumes bind follows interface attribute
  std::vector<std::string> vMpRtp = rtpParameters.getAttributeValues(mprtp::MPRTP);
  int interfaceIndex = -1;
  for (auto it = vMpRtp.begin(); it!=vMpRtp.end(); ++it)
  {
    std::string sVal = *it;
    // changing SDP to also feature bind parameter
    if (boost::algorithm::contains(sVal, "interface"))
    {
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sVal, " /:");
      // interface id should be at index 1
      bool bSuccess = false;
      interfaceIndex = convert<int>(vTokens[1], bSuccess);
      assert(bSuccess);
    }
    else if (boost::algorithm::contains(sVal, "bind"))
    {
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sVal, ":");
      // check for simple mprtp attribute declaration with no interfaces
      assert(vTokens.size() == 2);
      bool bSuccess = false;
      int bindIndex = convert<int>(vTokens[1], bSuccess);
      assert(bSuccess);
      assert(interfaceIndex != -1);
      // the SDP indexes from 1
      mBindMap[interfaceIndex-1] = bindIndex-1;
      interfaceIndex = -1;
    }
  }

  std::vector<RtpNetworkInterface::ptr> vRtpInterfaces;
  for (std::size_t i = 0; i < rtpParameters.getLocalEndPointCount(); ++i)
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(i).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(i).second;
    LOG(INFO) << "LOCAL ADDR/RTP/RTCP " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

    std::unique_ptr<RtpNetworkInterface> pRtpInterface;
    if (rtpEp.getPort() != rtcpEp.getPort())
    {
      VLOG(2) << "Setting up non-multiplexed multipath RTP/RTCP transport";
      if (m_existingConnectionAdapter.usePortManager())
      {
        // lookup previously created interface
        PortAllocationManager* pPortManager = m_existingConnectionAdapter.getPortManager();
        assert(pPortManager);
        std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket = pPortManager->getBoundUdpSocket(rtpEp.getAddress(), rtpEp.getPort());
        if (!pRtpSocket)
        {
          LOG(WARNING) << "Failed to lookup existing RTP port for " << rtpEp;
          return std::vector<RtpNetworkInterface::ptr>();
        }
        std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket = pPortManager->getBoundUdpSocket(rtcpEp.getAddress(), rtcpEp.getPort());
        if (!pRtcpSocket)
        {
          LOG(WARNING) << "Failed to lookup existing RTCP port for " << rtcpEp;
          return std::vector<RtpNetworkInterface::ptr>();
        }
        pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp, std::move(pRtpSocket), std::move(pRtcpSocket)));
      }
      else
      {
        boost::system::error_code ec;
        pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp, ec));
        if (ec)
        {
          LOG(WARNING) << "Error constructing RTP interfaces: " << ec.message();
          return std::vector<RtpNetworkInterface::ptr>();
        }
      }
    }
    else
    {
      // make sure that the rtcp-mux attribute has been defined
      if (rfc5761::muxRtpRtcp(rtpParameters))
      {
        VLOG(2) << "Setting up multiplexed multipath RTP/RTCP transport";
        if (m_existingConnectionAdapter.usePortManager())
        {
          // lookup previously created interface
          PortAllocationManager* pPortManager = m_existingConnectionAdapter.getPortManager();
          assert(pPortManager);
          std::unique_ptr<boost::asio::ip::udp::socket> pRtpRtcpSocket = pPortManager->getBoundUdpSocket(rtpEp.getAddress(), rtpEp.getPort());
          if (!pRtpRtcpSocket)
          {
            LOG(WARNING) << "Failed to lookup existing RTP/RTCP socket for " << rtpEp;
            return std::vector<RtpNetworkInterface::ptr>();
          }
          pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>(new MuxedUdpRtpNetworkInterface(m_rIoService, rtpEp, std::move(pRtpRtcpSocket)));
        }
        else
        {
          boost::system::error_code ec;
          pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>(new MuxedUdpRtpNetworkInterface(m_rIoService, rtpEp, ec));
          if (ec)
          {
            LOG(WARNING) << "Error constructing RTP/RTCP interfaces: " << ec.message();
            return std::vector<RtpNetworkInterface::ptr>();
          }
        }
      }
      else
      {
        LOG(WARNING) << "Error setting up multiplexed multipath RTP/RTCP transport - RTCP-mux not set";
        return std::vector<RtpNetworkInterface::ptr>(); // return empty vector here: the class will fail to create the RTP session
      }
    }

    vRtpInterfaces.push_back(std::move(pRtpInterface));

    // Now that we have created a local interface: create flows
    // initialise flows: each flow consists of a 5-tuple of IP-port pairs and protocol
    for (int j = 0; j < (int)rtpParameters.getRemoteEndPointCount(); ++j)
    {
      if (mBindMap.empty() || mBindMap.find(i) == mBindMap.end())
      {
        // bind all local interfaces to all remote interfaces
        // map flow id to interface index
        m_MpRtp.m_mFlowInterfaceMap[m_MpRtp.m_uiLastFlowId] = vRtpInterfaces.size() - 1;
        m_MpRtp.m_mFlows[m_MpRtp.m_uiLastFlowId] = mprtp::MpRtpFlow(m_MpRtp.m_uiLastFlowId, rtpParameters.getLocalEndPoint(i), rtpParameters.getRemoteEndPoint(j));
        m_MpRtp.m_uiLastFlowId++;
      }
      else
      {
        if (j == mBindMap[i])
        {
          // bind all local interfaces to all remote interfaces
          // map flow id to interface index
          m_MpRtp.m_mFlowInterfaceMap[m_MpRtp.m_uiLastFlowId] = vRtpInterfaces.size() - 1;
          m_MpRtp.m_mFlows[m_MpRtp.m_uiLastFlowId] = mprtp::MpRtpFlow(m_MpRtp.m_uiLastFlowId, rtpParameters.getLocalEndPoint(i), rtpParameters.getRemoteEndPoint(j));
          m_MpRtp.m_uiLastFlowId++;
          LOG(INFO) << "Found bound interface. Local interface: " << i+1 << " remote interface: " << j+1;
          break;
        }
        else
        {
          LOG(INFO) << "Searching for bound interface. Local interface: " << i+1 << " remote interface: " << j+1;
        }
      }
    }
  }

  VLOG(2) << "Creating multipath network interfaces done: #flows: " << m_MpRtp.m_mFlows.size();

  return vRtpInterfaces;
}

PayloadPacketiserBase::ptr RtpSession::createPayloadPacketiser(const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters)
{
  boost::optional<uint32_t> mtu = applicationParameters.getUintParameter(app::ApplicationParameters::mtu);
  uint32_t uiMtu = mtu ? *mtu : app::ApplicationParameters::defaultMtu;

  if (rtpParameters.isRetransmissionEnabled())
  {
    // provision bytes for RTX
    uiMtu -= 2;
  }

  if (mprtp::isMpRtpSession(rtpParameters))
  {
    uiMtu -= 5; // one byte for extension header, one for MPRTP header?
  }

  // experimental ACK in RTP header extension
  boost::optional<rfc5285::Extmap> extmap = m_parameters.lookupExtmap(experimental::EXTENSION_NAME_RTCP_HEADER_EXT);
  if (extmap)
  {
    LOG(INFO) << "RTCP RTP header extension is enabled: reducing MTU by " << 200;
    // we can make this configurable via SDP
    uiMtu -= 200;
  }

  if (rtpParameters.getEncodingName() == rfc6184::H264 )
  {
    rfc6184::Rfc6184Packetiser* pPacketiser = new rfc6184::Rfc6184Packetiser(uiMtu, rtpParameters.getSessionBandwidthKbps());
    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
    if (fmtp.size() > 0)
    {
      std::string sFmtp = fmtp[0];
      media::h264::H264FormatDescription description(sFmtp);
      rfc6184::Rfc6184Packetiser::PacketizationMode eMode = static_cast<rfc6184::Rfc6184Packetiser::PacketizationMode>(convert<uint32_t>(description.PacketizationMode, 0));
      // configure packetization mode
      pPacketiser->setPacketizationMode(eMode);

      boost::optional<bool> disableStap = applicationParameters.getBoolParameter(app::ApplicationParameters::disable_stap);
      if (disableStap)
        pPacketiser->setDisableStap(*disableStap);
    }
    return PayloadPacketiserBase::ptr(pPacketiser);
  }
  else if (rtpParameters.getEncodingName() == rfc6190::H264_SVC )
  {
    rfc6190::Rfc6190PayloadPacketiser* pPacketiser = new rfc6190::Rfc6190PayloadPacketiser(uiMtu, rtpParameters.getSessionBandwidthKbps());
    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
    if (fmtp.size() > 0)
    {
      std::string sFmtp = fmtp[0];
      media::h264::H264FormatDescription description(sFmtp);
      rfc6184::Rfc6184Packetiser::PacketizationMode eMode = static_cast<rfc6184::Rfc6184Packetiser::PacketizationMode>(convert<uint32_t>(description.PacketizationMode, 0));
      // configure packetization mode
      pPacketiser->setPacketizationMode(eMode);
    }
    return PayloadPacketiserBase::ptr(pPacketiser);
  }
  else if (rtpParameters.getEncodingName() == rfchevc::H265 )
  {
    boost::optional<uint32_t> mtu = applicationParameters.getUintParameter(app::ApplicationParameters::mtu);
    uint32_t uiMtu = mtu ? *mtu : app::ApplicationParameters::defaultMtu;
    rfchevc::RfchevcPacketiser* pPacketiser = new rfchevc::RfchevcPacketiser(uiMtu, rtpParameters.getSessionBandwidthKbps());
    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
    if (fmtp.size() > 0)
    {
      std::string sFmtp = fmtp[0];
      media::h265::H265FormatDescription description(sFmtp);
      rfchevc::RfchevcPacketiser::PacketizationMode eMode = static_cast<rfchevc::RfchevcPacketiser::PacketizationMode>(convert<uint32_t>(description.PacketizationMode, 0));
      // configure packetization mode
      pPacketiser->setPacketizationMode(eMode);
    }
    return PayloadPacketiserBase::ptr(pPacketiser);
  }
  else if (rtpParameters.getEncodingName() == rfc4867::AMR)
  {
    // TODO: get frames per RTP from config?
    rfc4867::Rfc4867Packetiser* pPacketiser = new rfc4867::Rfc4867Packetiser(uiMtu, rtpParameters.getSessionBandwidthKbps());
    return PayloadPacketiserBase::ptr(pPacketiser);
  }
  else
  {
    return PayloadPacketiserBase::create();
  }
}

rfc3550::SessionDatabase::ptr RtpSession::createSessionDatabase(const RtpSessionParameters& rtpParameters,
                                                                const GenericParameters& /*applicationParameters*/)
{
  if (mprtp::isMpRtpSession(rtpParameters))
  {
    return mprtp::MpRtpSessionDatabase::create(m_rtpSessionState, rtpParameters);
  }
  else
  {
    return rfc3550::SessionDatabase::create(m_rtpSessionState, rtpParameters);
  }
}

bool RtpSession::tryScheduleEarlyFeedback(uint16_t uiFlow, boost::posix_time::ptime& tScheduled, uint32_t& uiMs)
{
  if (uiFlow < m_vRtcpReportManagers.size())
  {
    VLOG(2) << "Scheduling early feedback";
    if (m_vRtcpReportManagers[uiFlow]->scheduleEarlyFeedbackIfPossible(tScheduled, uiMs))
    {
      VLOG(15) << "Early feedback scheduled in " << uiMs << "ms";
      return true;
    }
    else
    {
      VLOG(2) << "Not able to schedule early feedback";
      return false;
    }
  }
  else
  {
    VLOG(2) << "Invalid flow for early feedback: " << uiFlow;
    return false;
  }
}

std::vector<rfc3550::RtcpReportManager::ptr>
RtpSession::createRtcpReportManagers(const RtpSessionParameters &rtpParameters,
                                     const GenericParameters& applicationParameters)
{
  std::vector<rfc3550::RtcpReportManager::ptr> vRtcpReportManagers;
  if (mprtp::isMpRtpSession(rtpParameters))
  {
    // create RTCP report manager per flow
    for (std::map<uint16_t, mprtp::MpRtpFlow>::iterator it = m_MpRtp.m_mFlows.begin(); it != m_MpRtp.m_mFlows.end(); ++it)
    {
      uint32_t uiInterfaceId = m_MpRtp.m_mFlowInterfaceMap[it->first];

      const EndPointPair_t& localEps = it->second.getLocalEndPoint();
      const EndPoint& localRtpEp = localEps.first;
      const EndPoint& localRtcpEp = localEps.second;

      const EndPointPair_t& remoteEps = it->second.getRemoteEndPoint();
      const EndPoint& rtpEp = remoteEps.first;
      const EndPoint& rtcpEp = remoteEps.second;
      VLOG(5) << "Creating RTCP manager for flow " << it->first
              << " Local IF: " << localRtpEp.getAddress() << ":" << localRtpEp.getPort() << ":" << localRtcpEp.getPort()
              << " Remote IF: " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

      std::unique_ptr<mprtp::MpRtcpReportManager> pMpRtcpReportManager = std::unique_ptr<mprtp::MpRtcpReportManager>(
            new mprtp::MpRtcpReportManager(m_rIoService, m_rtpSessionState, rtpParameters,
                                           m_referenceClock,
                                           m_pSessionDb,
                                           m_rtpSessionStatistics, it->first));
      // NB: bind interface id and endpoint for later usage
      pMpRtcpReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1, uiInterfaceId, rtcpEp));
      pMpRtcpReportManager->setFeedbackCallback(boost::bind(&RtpSession::onGenerateFeedbackMpRtp, this, it->first, _1));
      vRtcpReportManagers.push_back( std::move(pMpRtcpReportManager));
    }
  }
  else
  {
    if ( rfc3550::isAudioVisualProfile(rtpParameters) )
    {
      rfc3550::RtcpReportManager::ptr pRtcpReportManager = rfc3550::RtcpReportManager::create(m_rIoService,
                                                                                              m_rtpSessionState,
                                                                                              rtpParameters,
                                                                                              m_referenceClock,
                                                                                              m_pSessionDb, m_rtpSessionStatistics);
      pRtcpReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1));
      vRtcpReportManagers.push_back( std::move(pRtcpReportManager));
    }
    else if (rfc4585::isAudioVisualProfileWithFeedback(rtpParameters))
    {
      rfc4585::RtcpReportManager::ptr pFbReportManager = rfc4585::RtcpReportManager::create(m_rIoService,
                                                                                            m_rtpSessionState,
                                                                                            rtpParameters,
                                                                                            m_referenceClock,
                                                                                            m_pSessionDb, m_rtpSessionStatistics);
      boost::optional<double> t_rr_interval = applicationParameters.getDoubleParameter(app::ApplicationParameters::t_rr_interval);
      if (t_rr_interval)
      {
        pFbReportManager->setT_rr_interval(*t_rr_interval);
      }

      // configure callback used for feedback information
      pFbReportManager->setFeedbackCallback(boost::bind(&RtpSession::onGenerateFeedback, this, _1));
      pFbReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1));
      vRtcpReportManagers.push_back( std::move(pFbReportManager));
    }
    else
    {
      // unsupported profile: this will cause the RTP session setup to fail
      LOG(WARNING) << "Unsupported profile: " << rtpParameters.getProfile();
    }
  }
  return vRtcpReportManagers;
}

void RtpSession::onGenerateFeedback(CompoundRtcpPacket& feedback)
{
  if (m_onFeedback)
  {
    m_onFeedback(feedback);
  }
#if 0
  if (m_pFeedbackManager)
    return m_pFeedbackManager->retrieveFeedbackReports();
  else
    return CompoundRtcpPacket();
#endif
}

void RtpSession::onGenerateFeedbackMpRtp(uint16_t uiFlowId, CompoundRtcpPacket& feedback)
{
  if (m_onMpFeedback)
  {
    m_onMpFeedback(uiFlowId, feedback);
  }
#if 0
  if (m_pFeedbackManager)
    return m_pFeedbackManager->retrieveFeedbackReports(uiFlowId);
  else
    return CompoundRtcpPacket();
#endif
}

void RtpSession::startReadingFromSockets()
{
  for(auto& pRtpInterface : m_vRtpInterfaces)
  {
    pRtpInterface->recv();
  }
}

bool RtpSession::initialise()
{
  try
  {
    m_state = SS_STOPPED;
    // create components
    m_pPayloadPacketiser = createPayloadPacketiser(m_parameters, m_applicationParameters);
    assert( m_pPayloadPacketiser );

    m_pSessionDb = createSessionDatabase(m_parameters, m_applicationParameters);
    EXIT_ON_TRUE(m_pSessionDb == nullptr, "Failed to construct session database(s)");
    m_pSessionDb->setMemberUpdateCallback(std::bind(&RtpSession::onMemberUpdate, this, std::placeholders::_1));

    m_vRtpInterfaces = createRtpNetworkInterfaces(m_parameters, m_applicationParameters);
    EXIT_ON_TRUE(m_vRtpInterfaces.empty(), "Failed to create RTP network interfaces");
    initialiseRtpNetworkInterfaces(m_parameters, m_applicationParameters);

    // RTCP inialisation MUST only occur after network interface initialisation
    // which is after the RTP network interfaces have been constructed
    m_vRtcpReportManagers = createRtcpReportManagers(m_parameters, m_applicationParameters);
    // TODO: it could be that we want to disable RTCP?
    EXIT_ON_TRUE(m_vRtcpReportManagers.empty(), "Failed to construct RTCP report managers");
    initialiseRtcp();

    // if we start reading from sockets sooner, packets may arrive before the feedback managers, etc have been setup
    startReadingFromSockets();
    return true;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }
  return false;
}

void RtpSession::initialiseRtcp()
{
  for(auto& pRtcpReportManager : m_vRtcpReportManagers)
  {
    pRtcpReportManager->startReporting();
  }

  m_uiByeSentCount = 0;
}

void RtpSession::shutdownRtcp()
{
  if (!m_vRtcpReportManagers.empty())
  {
    for(auto& pRtcpReportManager : m_vRtcpReportManagers)
    {
      pRtcpReportManager->scheduleFinalReportAndShutdown();
    }
    VLOG(10) << "RTCP report managers shutdown";
  }
  else
  {
    // This step only happens after the BYEs have been sent if there are RTCP report managers
    LOG(INFO) << "No RTCP report manager to shutdown: shutting down RTP interface";
    shutdownNetworkInterfaces();
  }
}

void RtpSession::initialiseRtpNetworkInterfaces(const RtpSessionParameters &rtpParameters,
                                                const GenericParameters& applicationParameters)
{
  for(auto& pRtpInterface : m_vRtpInterfaces)
  {
    configureNetworkInterface(pRtpInterface, rtpParameters, applicationParameters);

    pRtpInterface->setIncomingRtpHandler(boost::bind(&RtpSession::onIncomingRtp, this, _1, _2) );
    pRtpInterface->setIncomingRtcpHandler(boost::bind(&RtpSession::onIncomingRtcp, this, _1, _2) );
    pRtpInterface->setOutgoingRtpHandler(boost::bind(&RtpSession::onOutgoingRtp, this, _1, _2) );
    pRtpInterface->setOutgoingRtcpHandler(boost::bind(&RtpSession::onOutgoingRtcp, this, _1, _2) );
  }
}

void RtpSession::shutdownNetworkInterfaces()
{
  for(auto& pRtpInterface : m_vRtpInterfaces)
  {
    pRtpInterface->shutdown();
  }
  VLOG(10) << "RTP network interfaces shutdown";
}

void RtpSession::onIncomingRtp( const RtpPacket& rtpPacket, const EndPoint& ep )
{
#ifdef DEBUG_INCOMING_RTP
  VLOG(10) << "Received RTP from " << ep.getAddress() << ":" << ep.getPort();
#endif

  // convert arrival time to RTP time
  uint32_t uiRtpTime = RtpTime::convertTimeToRtpTimestamp(rtpPacket.getArrivalTime(), m_parameters.getRtpTimestampFrequency(), m_rtpSessionState.getRtpTimestampBase());
  RtpPacket& rPacket = const_cast<RtpPacket&>(rtpPacket);
  rPacket.setRtpArrivalTimestamp(uiRtpTime);

  // process extension headers
  const rfc5285::RtpHeaderExtension& extension = rtpPacket.getHeaderExtension();
  std::vector<rfc5285::HeaderExtensionElement> vExtensions = extension.getHeaderExtensions();
  VLOG(12) << "RTP header has " << vExtensions.size() << " extensions";
  for (rfc5285::HeaderExtensionElement& ext: vExtensions)
  {
    auto it = m_mExtensionHeaderHandlers.find(ext.getId());
    if (it != m_mExtensionHeaderHandlers.end())
    {
      // invoke registered callback
      it->second(rtpPacket, ext);
    }
    else
    {
      LOG(WARNING) << "Can't locate RTP extension header handler for ID " << ext.getId();
    }
  }

  boost::mutex::scoped_lock l( m_componentLock );

  if (!m_parameters.isRtxPayloadType(rtpPacket.getHeader().getPayloadType()))
  {
    // we can only handle a single SSRC in this RTP session (+ the RTX SSRC)
    if (rtpPacket.getSSRC() != m_rtpSessionState.getRemoteSSRC())
    {
      LOG(INFO) << "New SSRC: " << hex(rtpPacket.getSSRC());
      m_rtpSessionState.setRemoteSSRC(rtpPacket.getSSRC());
    }

    // standard RTP packet
    bool bIsValid = false, bIsRtcpSynchronised = false;
    boost::posix_time::ptime tPresentation;
    m_pSessionDb->processIncomingRtpPacket(rtpPacket, ep, m_parameters.getRtpTimestampFrequency(), bIsValid, bIsRtcpSynchronised, tPresentation);
    // The OWD has been set previously when parsing the RFC6051 extension header
    if (m_bTryExtractNtpTimestamp && rtpPacket.getOwdSeconds() != -1.0)
    {
      VLOG(2) << "SN: " << rtpPacket.getExtendedSequenceNumber() << " One Way Delay: " << rtpPacket.getOwdSeconds() << "s";
    }

    if (m_incomingRtp)
      m_incomingRtp(rtpPacket, ep, bIsValid, bIsRtcpSynchronised, tPresentation);

    m_rtpSessionStatistics.onReceiveRtpPacket(rtpPacket, ep);
  }
  else
  {
    // update session DB: this keeps state of the packet count, etc.
    bool bIsValid = false, bIsRtcpSynchronised = false;
    boost::posix_time::ptime tPresentation;
    m_pSessionDb->processIncomingRtpPacket(rtpPacket, ep, m_parameters.getRtpTimestampFrequency(), bIsValid, bIsRtcpSynchronised, tPresentation);

    if (m_incomingRtp)
      m_incomingRtp(rtpPacket, ep, bIsValid,
                    bIsRtcpSynchronised, tPresentation);
  }
}

void RtpSession::onOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint& ep )
{
#ifdef DEBUG_OUTGOING_RTP
  if (m_uiRtpTsLastRtpPacketSent > 0)
  {
    int diff = rtpPacket.getHeader().getRtpTimestamp() - m_uiRtpTsLastRtpPacketSent;
    DLOG(INFO) << "RTP SN: " << rtpPacket.getSequenceNumber() << " TS: " << rtpPacket.getHeader().getRtpTimestamp() << " sent to " << ep << " Diff: " << diff;
  }
  else
  {
    DLOG(INFO) << "First RTP SN: " << rtpPacket.getSequenceNumber() << " TS: " << rtpPacket.getHeader().getRtpTimestamp() << " sent to " << ep ;
  }
#endif

  // store RTP TS and local system time mapping
  m_tLastRtpPacketSent = boost::posix_time::microsec_clock::universal_time();
  m_uiRtpTsLastRtpPacketSent = rtpPacket.getHeader().getRtpTimestamp();

  m_pSessionDb->onSendRtpPacket(rtpPacket, ep);
  m_rtpSessionStatistics.onSendRtpPacket(rtpPacket, ep);

  if (m_afterOutgoingRtp) m_afterOutgoingRtp(rtpPacket, ep);
}

void RtpSession::onIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep )
{
#ifdef DEBUG_RTCP
  VLOG(2) << "[" << this << "] Processing incoming RTCP from " << ep << " size: " << compoundRtcp.size();
#endif

  // update database
  m_pSessionDb->processIncomingRtcpPacket(compoundRtcp, ep);
  m_rtpSessionStatistics.onReceiveRtcpPacket(compoundRtcp, ep);

  // process RTCP using session DB before passing to outer layer (RtpSessionManager)
  if (m_incomingRtcp) m_incomingRtcp(compoundRtcp, ep);

  // hande NACK packet here if retransmission is supported
  std::for_each(compoundRtcp.begin(), compoundRtcp.end(), [this, ep](const RtcpPacketBase::ptr& pRtcpPacket)
  {
    const RtcpPacketBase& rtcpPacket = *pRtcpPacket.get();
    // moving handling of other RTCP reports to RtpSessionManager
    if (rfc3550::isStandardRtcpReport(rtcpPacket))
    {
      // check for bye packet
      switch (rtcpPacket.getPacketType())
      {
        case rfc3550::PT_RTCP_SR:
        {
          const rfc3550::RtcpSr& sr = static_cast<const rfc3550::RtcpSr&>(rtcpPacket);
          // HACK for now to configure source SSRC in feedback manager
          break;
        }
        case rfc3550::PT_RTCP_BYE:
        {
          const rfc3550::RtcpBye& bye = static_cast<const rfc3550::RtcpBye&>(rtcpPacket);
          // disable RTO estimation when bye is received.
          // Actually we need one RTO per SSRC, but this will
          // have to be implemented when it is deemed necessary
          VLOG(5) << "[" << this << "] BYE received: resetting feedback manager";
#if 0
          if (m_pFeedbackManager)
            m_pFeedbackManager->reset();
#endif
          if (m_onUpdate)
          {
            MemberUpdate update(bye.getSSRCs()[0], false);
            m_onUpdate(update);
          }
          break;
        }
      }
    }
  });

  VLOG(10) << "[" << this << "] RTCP reports processed successfully: " << compoundRtcp.size();
}

void RtpSession::onOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep )
{
  m_pSessionDb->onSendRtcpPacket(compoundRtcp, ep);
  m_rtpSessionStatistics.onSendRtcpPacket(compoundRtcp, ep);

  // This code forms an integral part of the event loop: it assumes that each RtcpReportManager
  // must send a BYE at the end of the session and that each sent BYE results in this method being
  // called.

  // check if the outgoing packet contains an RTCP BYE: if so, shutdown the network interfaces
  // If all required BYEs have been sent, shutdown the network interfaces
  std::for_each(compoundRtcp.begin(), compoundRtcp.end(), [this](RtcpPacketBase::ptr pRtcpPacket)
  {
    if (pRtcpPacket->getPacketType() == rfc3550::PT_RTCP_BYE)
    {
      ++m_uiByeSentCount;
    }
  });

  if (m_outgoingRtcp) m_outgoingRtcp(compoundRtcp, ep);

  VLOG_IF(5, m_uiByeSentCount > 0) << m_uiByeSentCount << " RTCP BYE(s) sent. " << m_vRtcpReportManagers.size() << " report managers.";
  if (m_uiByeSentCount == m_vRtcpReportManagers.size())
  {
   VLOG(5) << "RTCP BYE(s) sent: shutting down RTP interface";
// #define HACK
#ifdef HACK
#ifdef _WIN32
   Sleep(1000);
#else
   // HACK for SCTP to give it a chance to send the byes
   sleep(1);
#endif
#endif
   shutdownNetworkInterfaces();

   if (m_onRtpSessionComplete)m_onRtpSessionComplete();
  }
}

void RtpSession::onRtcpReportGeneration(const CompoundRtcpPacket &rtcp)
{
  const EndPoint rtcpEp = m_parameters.getRemoteEndPoint(0).second;
  if (!m_vRtpInterfaces[0]->send(rtcp, rtcpEp))
  {
    VLOG(2) << "Failed to send RTCP packet";
    // manually call handler for RTCP to allow base class to cleanly shutdown
    // the RTP session when a BYE packet is sent.
    onOutgoingRtcp(rtcp, rtcpEp);
  }

  // reset state for next interval
  m_rtpSessionStatistics.newInterval();
}

void RtpSession::onRtcpReportGeneration( const CompoundRtcpPacket& rtcp, uint32_t uiInterfaceIndex, const EndPoint& rtcpEp )
{
  assert(uiInterfaceIndex < m_vRtpInterfaces.size());
  if (!m_vRtpInterfaces[uiInterfaceIndex]->send(rtcp, rtcpEp))
  {
    VLOG(2) << "Failed to send RTCP packet";
    // manually call handler for RTCP to allow base class to cleanly shutdown
    // the RTP session when a BYE packet is sent.
    onOutgoingRtcp(rtcp, rtcpEp);
  }
}

void RtpSession::onMemberUpdate(const MemberUpdate& memberUpdate)
{
  if (m_onUpdate) m_onUpdate(memberUpdate);
  // temporary HACK: this should be moved to the manager
  // but for MPRTP scheduling purposes we are storing the RTT,loss,etc
  if (m_bIsMpRtpSession)
  {
    if (memberUpdate.isMemberUpdate() && memberUpdate.getFlowId() != -1)
    {
      VLOG(5) << "Member Update!! Flow Id: " << memberUpdate.getFlowId() << " RTT: " << memberUpdate.getRoundTripTime();
      m_mPathInfo[memberUpdate.getFlowId()] = memberUpdate;
    }
  }
}

uint16_t RtpSession::findSubflowWithSmallestRtt(double& dRtt) const
{
  uint16_t uiFlowId = 0;
  double dMinRTT = 100; // impossibly high RTT in seconds
  for (auto& it : m_mPathInfo)
  {
    if (it.second.getFlowId() != -1 && it.second.getRoundTripTime() <  dMinRTT)
    {
      dMinRTT = it.second.getRoundTripTime();
      uiFlowId = it.first;
      dRtt = dMinRTT;
    }
  }
  return uiFlowId;
}

uint32_t RtpSession::getCumulativeLostAsReceiver() const
{
  rfc3550::MemberEntry* pMemberEntry = m_pSessionDb->lookupMemberEntryInSessionDb(m_rtpSessionState.getRemoteSSRC());
  if (pMemberEntry)
  {
    return pMemberEntry->getCumulativeNumberOfPacketsLost();
  }
  return 0;
}

} // rtp_plus_plus
