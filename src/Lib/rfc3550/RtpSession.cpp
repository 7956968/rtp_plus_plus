#include "CorePch.h"
#include <rtp++/rfc3550/RtpSession.h>

#include <algorithm>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/get_pointer.hpp>
#include <cpputil/StringTokenizer.h>

#include <rtp++/media/h264/H264FormatDescription.h>
#include <rtp++/mprtp/MpRtpHeaderExtensionParser.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpSession.h>
#include <rtp++/mprtp/MpRtcpParser.h>
#include <rtp++/mprtp/MpRtcpReportManager.h>
#include <rtp++/mprtp/MpRtpHeaderExtensionParser.h>
#include <rtp++/mprtp/MpRtpPlayoutBuffer.h>
#include <rtp++/mprtp/MpRtpSessionDatabase.h>
#include <rtp++/network/MuxedUdpRtpNetworkInterface.h>
#include <rtp++/network/UdpRtpNetworkInterface.h>
#include <rtp++/rfc3550/RtcpTransmissionTimer.h>
#include <rtp++/rfc3550/RtpConstants.h>
#include <rtp++/rfc3550/RtpUtil.h>
#include <rtp++/rfc3550/SessionDatabase.h>
#include <rtp++/rfc3611/RtcpParser.h>
#include <rtp++/rfc4571/PassiveTcpRtpNetworkInterface.h>
#include <rtp++/rfc4571/TcpRtpNetworkInterface.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/Rfc4585RtcpParser.h>
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>
#include <rtp++/rfc4585/Rfc4585RtcpTransmissionTimer.h>
#include <rtp++/rfc4585/RtcpFb.h>
#include <rtp++/rfc5506/Rfc5506.h>
#include <rtp++/rfc5506/Rfc5506RtcpValidator.h>
#include <rtp++/rfc5761/Rfc5761.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6184/Rfc6184Packetiser.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfc6190/Rfc6190Packetiser.h>
#include <rtp++/util/ApplicationParameters.h>

using namespace boost::asio;
using namespace boost::posix_time;

namespace rfc3550
{

static const int RTP_FLOW_INDEX = 0;
static const int RTX_FLOW_INDEX = 1;

RtpSession::MpRtpParameters::MpRtpParameters(const GenericParameters &applicationParameters)
  :m_eSchedulingAlgorithm(MPRTP_ROUND_ROBIN),
  m_uiLastFlowId(0),
  m_bPathWiseSplitAllowed(false),
  m_uiInterfaceIndex(0),
  m_uiCurrentPatternIndex(0),
  m_uiPacketsSentOnCurrentIndex(0)
{
  initialiseMpRtpParameters(applicationParameters);
}

uint16_t RtpSession::MpRtpParameters::selectSubflowForPackets()
{
  switch (m_eSchedulingAlgorithm)
  {
    case MPRTP_RANDOM:
    {
      int index = rand() % m_mFlows.size();
      return index;
    }
    case MPRTP_XYZ:
    {
      if (m_uiPacketsSentOnCurrentIndex == m_vPacketSendPattern[m_uiCurrentPatternIndex])
      {
        m_uiCurrentPatternIndex = (m_uiCurrentPatternIndex + 1) % m_vPacketSendPattern.size();
        m_uiPacketsSentOnCurrentIndex = 1;
        m_uiInterfaceIndex = ++m_uiInterfaceIndex % m_mFlowInterfaceMap.size();
        return m_uiInterfaceIndex;
      }
      else
      {
        ++m_uiPacketsSentOnCurrentIndex;
        return m_uiInterfaceIndex;
      }
      break;
    }
    default:
    {
      // Round robin
      static uint16_t interface_id = 0;
      uint16_t index = interface_id++ % m_mFlowInterfaceMap.size();
      return index;
    }
  }
}

void RtpSession::MpRtpParameters::initialiseMpRtpParameters(const GenericParameters &applicationParameters)
{
  boost::optional<bool> bSplit = applicationParameters.getBoolParameter(ApplicationParameters::mprtp_psa);
  if (bSplit) m_bPathWiseSplitAllowed = *bSplit;

  boost::optional<std::string> sSchedulingAlgorithm = applicationParameters.getStringParameter(ApplicationParameters::mprtp_sa);
  if (sSchedulingAlgorithm)
  {
    if (*sSchedulingAlgorithm == "rnd")
    {
      m_eSchedulingAlgorithm = MPRTP_RANDOM;
    }
    else
    {
      m_eSchedulingAlgorithm = MPRTP_XYZ;
      // see if there's a correct format string
      std::vector<uint32_t> vTokens = StringTokenizer::tokenizeV2<uint32_t>(*sSchedulingAlgorithm, "-", true);
      if (!vTokens.empty())
      {
        for (uint32_t uiVal: vTokens)
        {
          // if there's an invalid value, revert back to RR
          if (uiVal == 0)
            m_eSchedulingAlgorithm = MPRTP_ROUND_ROBIN;
        }
        if (m_eSchedulingAlgorithm == MPRTP_XYZ)
        {
          m_vPacketSendPattern = vTokens;
          VLOG(2) << "MPRTP scheduling pattern: " << toString(m_vPacketSendPattern);
        }
      }
      else
      {
        m_eSchedulingAlgorithm = MPRTP_ROUND_ROBIN;
      }
    }
  }
}

RtpSession::ptr RtpSession::create(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters)
{
  return boost::make_shared<RtpSession>(boost::ref(rIoService), boost::ref(rtpParameters), applicationParameters);
}

RtpSession::RtpSession(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters)
  : RtpSessionBase(rtpParameters, applicationParameters),
    m_rIoService(rIoService),
    m_bFirstSample(true),
    m_dStartOffset(0.0),
    m_bIsMpRtpSession(mprtp::isMpRtpSession(rtpParameters)),
    m_MpRtp(applicationParameters)
{

}

RtpSession::~RtpSession()
{
  VLOG(1) << "[" << this << "] RTP session destructor";
}

boost::system::error_code RtpSession::doStart()
{
  return boost::system::error_code();
}

boost::system::error_code RtpSession::doStop()
{
  boost::mutex::scoped_lock l( m_componentLock );

  DLOG(INFO) << "[" << this << "] Shutting down RTP session";

  VLOG(10) << "[" << this << "] Shutting down timers";
  // cancel all scheduled media sample events
  VLOG(10)  << "[" << this << "] Shutting down Playout timers";
  std::for_each(m_mPlayoutTimers.begin(), m_mPlayoutTimers.end(), [this](const std::pair<deadline_timer*, deadline_timer*>& pair)
  {
    pair.second->cancel();
  });
  // we can clear the timers now
  // the references were just needed
  // to be able to cancel them manually
  // m_mPlayoutTimers.clear();

  // RTX
  VLOG(10) << "[" << this << "] Shutting down RTX timers";
  std::for_each(m_mRtxTimers.begin(), m_mRtxTimers.end(), [this](const std::pair<deadline_timer*, deadline_timer*>& pair)
  {
    pair.second->cancel();
  });
  // m_mRtxTimers.clear();

  // part of contract: must call stop before destructor to enable clean async shutting down of timers
  if (m_pFeedbackManager)
    m_pFeedbackManager->shutdown();

  VLOG(10) << "[" << this << "] RtpSession::doStop complete.";
  return boost::system::error_code();
}

void RtpSession::sendPacket(RtpPacket& rtpPacket, uint16_t uiSubflowId)
{
  FlowMap_t::iterator it = m_MpRtp.m_mFlows.find(uiSubflowId);
  assert( it != m_MpRtp.m_mFlows.end());
  mprtp::MpRtpFlow& flow = it->second;

  // At this point we need to insert the MPRTP header
  uint16_t uiFlowSpecificSequenceNumber = flow.getNextSN();
  mprtp::MpRtpSubflowRtpHeader::ptr pMpRtpExtension = mprtp::MpRtpSubflowRtpHeader::create(uiSubflowId, uiFlowSpecificSequenceNumber);
  rtpPacket.getHeader().setRtpHeaderExtension(pMpRtpExtension);
  const EndPoint& endpoint = flow.getRemoteEndPoint().first;
  uint32_t uiInterfaceIndex = m_MpRtp.m_mFlowInterfaceMap[uiSubflowId];
  m_vRtpInterfaces[uiInterfaceIndex]->send(rtpPacket, endpoint);

  VLOG(2) << LOG_MODIFY_WITH_CARE
          << " Packet sent SN: " << rtpPacket.getHeader().getSequenceNumber()
          << " Flow Id: " << uiSubflowId
          << " FSSN: " << uiFlowSpecificSequenceNumber
          << " Time: " << boost::posix_time::microsec_clock::universal_time();
}

void RtpSession::doSendSample(MediaSample::ptr pSample)
{
  // get end point list
  assert( m_parameters.getRemoteEndPointCount() > 0);

  if (!m_bIsMpRtpSession)
  {
    doSendSampleSinglePath(pSample);
  }
  else
  {
    doSendSampleMultiPath(pSample);
  }
}

void RtpSession::doSendSampleSinglePath(MediaSample::ptr pSample)
{
  // same as the following: more complex subclasses should avoid addressing by index and use flow information
  // to retrieve endpoints and network interfaces, etc.
  const EndPoint& endpoint = m_parameters.getRemoteEndPoint(0).first;
  // const EndPoint& endpoint = m_parameters.getRemoteEndPoint(getPrimaryRtpFlow().getRemoteRtpEndpoint()).first;

  // create an RTP packet using the RTP session state
  std::vector<RtpPacket> rtpPackets = generateRtpPackets(pSample);
  // sending packets as fast as possible
  std::for_each(rtpPackets.begin(), rtpPackets.end(), [this, endpoint](const RtpPacket& rtpPacket)
  {
    m_vRtpInterfaces[0]->send(rtpPacket, endpoint);
  });
}

void RtpSession::doSendSampleMultiPath(MediaSample::ptr pSample)
{
  std::vector<RtpPacket> rtpPackets = generateRtpPackets(pSample);

  if (!m_MpRtp.m_bPathWiseSplitAllowed)
  {
    // fragmentation units are sent over one path
    uint16_t uiSubflowId = m_MpRtp.selectSubflowForPackets();
    std::for_each(rtpPackets.begin(), rtpPackets.end(), [this, uiSubflowId](RtpPacket& rtpPacket)
    {
      sendPacket(rtpPacket, uiSubflowId);
    });
  }
  else
  {
    // select path for each packet
    std::for_each(rtpPackets.begin(), rtpPackets.end(), [this](RtpPacket& rtpPacket)
    {
      uint16_t uiSubflowId = m_MpRtp.selectSubflowForPackets();
      sendPacket(rtpPacket, uiSubflowId);
    });
  }
}

bool RtpSession::sampleAvailable() const
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  return !m_qOutgoing.empty();
}

MediaSample::ptr RtpSession::getNextSample()
{
  boost::mutex::scoped_lock l(m_outgoingSampleLock);
  if (m_qOutgoing.empty()) return MediaSample::ptr();
  MediaSample::ptr pSample = m_qOutgoing.front();
  m_qOutgoing.pop_front();
  return pSample;
}

void RtpSession::insertPacketIntoPlayoutBufferAndScheduleIfRequired(const RtpPacket& rtpPacket, bool& bLate, bool& bDuplicate)
{
  ptime tPlayout;
  // Note: the playout buffer only returns true if a new playout time was calculated
  // for the sample
  if (uSeconds[0]->addRtpPacket(rtpPacket, tPlayout, bLate, bDuplicate))
  {
    // a playout time has now been scheduled: create a timed event
    deadline_timer* pTimer = new deadline_timer(m_rIoService);
#ifdef DEBUG_PLAYOUT_DEADLINE
    ptime tNow = microsec_clock::universal_time();
    long diffMs = (tPlayout - tNow).total_milliseconds();
    VLOG(10) << "Scheduling packet for playout at " << tPlayout << " (in " << diffMs << "ms)";
#endif
    pTimer->expires_at(tPlayout);
    pTimer->async_wait(boost::bind(&RtpSession::onScheduledSampleTimeout, this, _1, pTimer) );
    m_mPlayoutTimers.insert(std::make_pair(pTimer, pTimer));
  }
}

void RtpSession::doHandleIncomingRtp( const RtpPacket& rtpPacket, const EndPoint &ep )
{
  boost::mutex::scoped_lock l( m_componentLock );
  // don't add new data to the playout buffer once
  // we are shutting down: all handlers have been cancelled
  // in stop
  if ( isShuttingDown() ) return;

  if (!m_parameters.isRtxPayloadType(rtpPacket.getHeader().getPayloadType()))
  {
    processStandardRtpPacket(rtpPacket, ep);
  }
  else
  {
    // we don't want to do the extensive validation procedure
    // for retransmission packets: this could be infrequent and
    // the corresponding SSRC could time out frequently
    // NEW NOTE: if the RTCP SRs sent from the source are frequent
    // enough, this will prevent a timeout of the RTX SSRC
    processRetransmission(rtpPacket, ep);
  }
}

void RtpSession::doHandleOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint &ep )
{
#ifdef DEBUG_OUTGOING_RTP
  if (m_uiRtpTsLastRtpPacketSent > 0)
  {
    int diff = rtpPacket.getHeader().getRtpTimestamp() - m_uiRtpTsLastRtpPacketSent;
    DLOG(INFO) << "RTP SN: " << rtpPacket.getSequenceNumber() << " TS: " << rtpPacket.getHeader().getRtpTimestamp() << " send to " << ep << " Diff: " << diff;
  }
  else
  {
    DLOG(INFO) << "First RTP SN: " << rtpPacket.getSequenceNumber() << " TS: " << rtpPacket.getHeader().getRtpTimestamp() << " send to " << ep ;
  }
#endif

  // store RTP TS and local system time mapping
  m_tLastRtpPacketSent = boost::posix_time::microsec_clock::universal_time();
  m_uiRtpTsLastRtpPacketSent = rtpPacket.getHeader().getRtpTimestamp();

  m_vSessionDbs[0]->onSendRtpPacket(rtpPacket, ep);
  m_rtpSessionStatistics.onSendRtpPacket(rtpPacket, ep);
}

void RtpSession::doHandleIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint &ep )
{
#ifdef DEBUG_RTCP
  VLOG(2) << "[" << this << "] Processing incoming RTCP Size: " << compoundRtcp.size();
#endif

  // We might still want to process RTCP for reconsideration purposes etc.
//  if (isShuttingDown())
//  {
//    LOG(INFO) << "Not processing RTCP: shutdown in progress";
//    return;
//  }

  // update database
  m_vSessionDbs[0]->processIncomingRtcpPacket(compoundRtcp, ep);
  m_rtpSessionStatistics.onReceiveRtcpPacket(compoundRtcp, ep);

  // hande NACK packet here if retransmission is supported
  std::for_each(compoundRtcp.begin(), compoundRtcp.end(), [this, ep](const RtcpPacketBase::ptr& pRtcpPacket)
  {
    const RtcpPacketBase& rtcpPacket = *pRtcpPacket.get();
    if (!rfc3550::isStandardRtcpReport(rtcpPacket))
    {
      processRtcpExtension(rtcpPacket, ep);
    }
    else
    {
      // check for bye packet
      switch (rtcpPacket.getPacketType())
      {
      case PT_RTCP_SR:
        {
          const rfc3550::RtcpSr& sr = static_cast<const rfc3550::RtcpSr&>(rtcpPacket);
          uint32_t uiSSRC = sr.getSSRC();
          // HACK for now to configure source SSRC in feedback manager
          if (m_pFeedbackManager && m_pFeedbackManager->getSourceSSRC() == 0)
          {
            m_pFeedbackManager->setSourceSSRC(uiSSRC);
          }
          break;
        }
      case PT_RTCP_BYE:
        {
          const rfc3550::RtcpBye& bye = static_cast<const rfc3550::RtcpBye&>(rtcpPacket);
          // disable RTO estimation when bye is received.
          // Actually we need one RTO per SSRC, but this will
          // have to be implemented when it is deemed necessary
          VLOG(1) << "[" << this << "] BYE received: resetting feedback manager";
          if (m_pFeedbackManager)
            m_pFeedbackManager->reset();
          break;
        }
      }
    }
  });

  VLOG(10) << "[" << this << "] RTCP reports processed successfully: " << compoundRtcp.size();
}

void RtpSession::processRtcpExtension( const RtcpPacketBase& rtcpPacket, const EndPoint &ep )
{
  switch (rtcpPacket.getPacketType())
  {
    case rfc4585::PT_RTCP_GENERIC_FEEDBACK:
    {
      VLOG(15) << "Processing Generic Feedback RTCP";
      handleGenericFeedbackRtcpReport(rtcpPacket, ep);
      break;
    }
    case rfc3611::PT_RTCP_XR:
    {
      VLOG(15) << "Processing XR RTCP";
      handleExtendedRtcpReport(rtcpPacket, ep);
      break;
    }
  }
}

void RtpSession::handleGenericFeedbackRtcpReport( const RtcpPacketBase& rtcpPacket, const EndPoint &ep )
{
  uint8_t uiFormat = rtcpPacket.getTypeSpecific();
  switch (uiFormat)
  {
  case rfc4585::TL_FB_GENERIC_NACK:
  {
    const rfc4585::RtcpGenericNack& nack = static_cast<const rfc4585::RtcpGenericNack&>(rtcpPacket);
    std::vector<uint16_t> nacks = nack.getNacks();
    VLOG(10) << "Received generic NACK containing " << nacks.size();
    std::for_each(nacks.begin(), nacks.end(), [this, ep](uint16_t uiSN)
    {
      onRtxRequest(uiSN, ep);
    });
    break;
  }
  default:
  {
    LOG_FIRST_N(WARNING, 1) << "Unknown generic feedback report type: " << uiFormat;
  }
  }
}

void RtpSession::handleExtendedRtcpReport( const RtcpPacketBase& rtcpPacket, const EndPoint &ep )
{
  VLOG(10) << "TODO: implement session level XR processing here if required";
}

void RtpSession::doHandleOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep )
{
  m_vSessionDbs[0]->onSendRtcpPacket(compoundRtcp, ep);
  m_rtpSessionStatistics.onSendRtcpPacket(compoundRtcp, ep);
}

void RtpSession::onScheduledSampleTimeout(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // trigger sample callback
    PlayoutBufferNode node = m_vReceiverBuffers[0]->getNextPlayoutBufferNode();
    PacketDataList_t dataList = node.getPacketData();
    // call the appropriate depacketizer to aggregate the packets, do error concealment, etc
    std::vector<MediaSample::ptr> vSamples = m_vPayloadPacketisers[0]->depacketize(dataList);
    boost::mutex::scoped_lock l(m_outgoingSampleLock);
    m_qOutgoing.insert(m_qOutgoing.end(), vSamples.begin(), vSamples.end());
  }

  std::size_t uiRemoved = m_mPlayoutTimers.erase(pTimer);
  assert(uiRemoved == 1);
  delete pTimer;
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfaces(const RtpSessionParameters& rtpParameters)
{
  if (!m_bIsMpRtpSession)
  {
    return createRtpNetworkInterfacesSinglePath(rtpParameters);
  }
  else
  {
    return createRtpNetworkInterfacesMultiPath(rtpParameters);
  }
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfacesSinglePath(const RtpSessionParameters& rtpParameters)
{
  std::vector<RtpNetworkInterface::ptr> vRtpInterfaces;
  if (!rtpParameters.useTcp())
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    VLOG(1) << "ADDR/RTP/RTCP " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

    std::unique_ptr<RtpNetworkInterface> pRtpInterface;
    if (rtpEp.getPort() != rtcpEp.getPort())
    {
      pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>( new UdpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp));
    }
    else
    {
      // make sure that the rtcp-mux attribute has been defined
      if (rtpParameters.hasAttribute(rfc5761::RTCP_MUX))
        pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>( new MuxedUdpRtpNetworkInterface (m_rIoService, rtpEp ));
      else
        return vRtpInterfaces; // notify base class of failure to create RTP network interface
    }

    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
  else
  {
    // check if we are dealing with RTP/RTCP mux
    const EndPoint rtpEp = rtpParameters.getLocalEndPoint(0).first;
    const EndPoint rtcpEp = rtpParameters.getLocalEndPoint(0).second;
    std::unique_ptr<RtpNetworkInterface> pRtpInterface;
    if (rtpParameters.isActiveTcp())
    {
      pRtpInterface = std::unique_ptr<rfc4571::TcpRtpNetworkInterface>( new rfc4571::TcpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp)) ;
    }
    else
    {
      pRtpInterface = std::unique_ptr<rfc4571::PassiveTcpRtpNetworkInterface>( new rfc4571::PassiveTcpRtpNetworkInterface(m_rIoService, rtpEp, rtcpEp)) ;
    }
    vRtpInterfaces.push_back(std::move(pRtpInterface));
  }
  return vRtpInterfaces;
}

std::vector<RtpNetworkInterface::ptr> RtpSession::createRtpNetworkInterfacesMultiPath(const RtpSessionParameters& rtpParameters)
{
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
      pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface( m_rIoService, rtpEp, rtcpEp ));
    }
    else
    {
      // make sure that the rtcp-mux attribute has been defined
      if (rfc5761::muxRtpRtcp(rtpParameters))
        pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>( new MuxedUdpRtpNetworkInterface(m_rIoService, rtpEp ));
      else
        return vRtpInterfaces; // return false here: the base class will fail to create the RTP session
    }

    vRtpInterfaces.push_back(std::move(pRtpInterface));

    // Now that we have created a local interface: create flows
    // initialise flows: each flow consists of a 5-tuple of IP-port pairs and protocol
    for (std::size_t j = 0; j < rtpParameters.getRemoteEndPointCount(); ++j)
    {
      // map flow id to interface index
      m_MpRtp.m_mFlowInterfaceMap[m_MpRtp.m_uiLastFlowId] = vRtpInterfaces.size() - 1;
      m_MpRtp.m_mFlows[m_MpRtp.m_uiLastFlowId] = mprtp::MpRtpFlow(m_MpRtp.m_uiLastFlowId, rtpParameters.getLocalEndPoint(i), rtpParameters.getRemoteEndPoint(j));
      m_MpRtp.m_uiLastFlowId++;
    }
  }
  return vRtpInterfaces;
}

void RtpSession::configureNetworkInterface(std::unique_ptr<RtpNetworkInterface>& pRtpInterface, const RtpSessionParameters &rtpParameters)
{
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
    pRtpInterface->registerExtensionHeaderParser(mprtp::MpRtpExtensionHeaderParser::create());
  }
}

std::vector<PayloadPacketiserBase::ptr> RtpSession::createPayloadPacketisers(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters)
{
  std::vector<PayloadPacketiserBase::ptr> vPayloadPacketisers;
  if (rtpParameters.getMimeType() == rfc6184::VIDEO_H264 )
  {
    boost::optional<uint32_t> mtu = applicationParameters.getUintParameter(ApplicationParameters::mtu);
    uint32_t uiMtu = mtu ? *mtu : ApplicationParameters::defaultMtu;
    rfc6184::Rfc6184Packetiser::ptr pPacketiser = rfc6184::Rfc6184Packetiser::create(uiMtu, rtpParameters.getSessionBandwidthKbps());
    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
    if (fmtp.size() > 0)
    {
      std::string sFmtp = fmtp[0];
      H264FormatDescription description(sFmtp);
      rfc6184::Rfc6184Packetiser::PacketizationMode eMode = static_cast<rfc6184::Rfc6184Packetiser::PacketizationMode>(convert<uint32_t>(description.PacketizationMode, 0));
      // configure packetization mode
      pPacketiser->setPacketizationMode(eMode);
    }
    vPayloadPacketisers.push_back( std::move(pPacketiser) );
  }
  else if (rtpParameters.getMimeType() == rfc6190::VIDEO_H264_SVC )
  {
    boost::optional<uint32_t> mtu = applicationParameters.getUintParameter(ApplicationParameters::mtu);
    uint32_t uiMtu = mtu ? *mtu : ApplicationParameters::defaultMtu;
    rfc6190::Rfc6190PayloadPacketiser::ptr pPacketiser = rfc6190::Rfc6190PayloadPacketiser::create(uiMtu, rtpParameters.getSessionBandwidthKbps());
//    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
//    if (fmtp.size() > 0)
//    {
//      std::string sFmtp = fmtp[0];
//      H264FormatDescription description(sFmtp);
//      rfc6184::Rfc6184Packetiser::PacketizationMode eMode = static_cast<rfc6184::Rfc6184Packetiser::PacketizationMode>(convert<uint32_t>(description.PacketizationMode, 0));
//      // configure packetization mode
//      pPacketiser->setPacketizationMode(eMode);
//    }
    vPayloadPacketisers.push_back( std::move(pPacketiser) );
  }
  return vPayloadPacketisers;
}

std::vector<SessionDatabase::ptr> RtpSession::createSessionDatabases(const RtpSessionParameters& rtpParameters)
{
  std::vector<SessionDatabase::ptr> vSessionDbs;
  if (mprtp::isMpRtpSession(rtpParameters))
  {
    vSessionDbs.push_back(mprtp::MpRtpSessionDatabase::create(rtpParameters));
  }
  else
  {
    vSessionDbs.push_back(SessionDatabase::create(rtpParameters));
  }
  return vSessionDbs;
}

std::vector<RtpPlayoutBuffer::ptr> RtpSession::createRtpPlayoutBuffers(const RtpSessionParameters &rtpParameters)
{
  std::vector<RtpPlayoutBuffer::ptr> vReceiverBuffers;
  std::vector<SessionDatabase::ptr> vSessionDbs;
  if (mprtp::isMpRtpSession(rtpParameters))
  {
    vReceiverBuffers.push_back(mprtp::MpRtpPlayoutBuffer::create());
  }
  else
  {
    vReceiverBuffers.push_back(RtpPlayoutBuffer::create());
  }
  return vReceiverBuffers;
}

void RtpSession::configureReceiverBuffers(const RtpSessionParameters &rtpParameters, const GenericParameters &applicationParameters)
{
  m_vReceiverBuffers[0]->setClockFrequency(rtpParameters.getRtpTimestampFrequency());
  // set buffer latency if configured in application parameters
  boost::optional<uint32_t> uiBufferLatency = applicationParameters.getUintParameter(ApplicationParameters::buf_lat);
  if (uiBufferLatency) m_vReceiverBuffers[0]->setPlayoutBufferLatency(*uiBufferLatency);
}

void RtpSession::onPacketAssumedLost(uint16_t uiSN)
{
  // Single path RTP
  // for multipath we need to select the "better" path
  // TODO: once 3611 is implemented we should check if there is sufficient time for an RTX
  boost::posix_time::ptime tScheduled;
  uint32_t uiMs = 0;
  VLOG(10) << "Scheduling feedback report";
  if (m_vRtcpReportManagers[0]->scheduleEarlyFeedbackIfPossible(tScheduled, uiMs))
  {
    VLOG(15) << "Early feedback scheduled in " << uiMs << "ms";
  }
  else
  {
    VLOG(20) << "Not able to schedule early feedback";
  }
}

void RtpSession::onPacketAssumedLostInFlow(uint16_t uiFlowId, uint16_t uiFSSN)
{
  // lookup which RTCP report manager is responsible for flow and use it to send feedback
  VLOG(2) << "TODO: implement!";
}

CompoundRtcpPacket RtpSession::onGenerateFeedback()
{
  return m_pFeedbackManager->retrieveFeedbackReports();
}

CompoundRtcpPacket RtpSession::onGenerateFeedbackMpRtp(uint16_t uiFlowId)
{
  return m_pFeedbackManager->retrieveFeedbackReports(uiFlowId);
}

std::vector<RtcpReportManager::ptr> RtpSession::createRtcpReportManagers(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters, std::vector<SessionDatabase::ptr>& vSessionDbs)
{
  std::vector<RtcpReportManager::ptr> vRtcpReportManagers;
  if (mprtp::isMpRtpSession(rtpParameters))
  {
#ifndef DEBUG_DISABLE_FEEDBACK_MANAGER
    m_pFeedbackManager = std::unique_ptr<ILossFeedbackManager>( new mprtp::FeedbackManager(rtpParameters, applicationParameters, m_rIoService ));
    m_pFeedbackManager->setLossCallback(boost::bind(&RtpSession::onPacketAssumedLost, this, _1));
    m_pFeedbackManager->setFlowSpecificLossCallback(boost::bind(&RtpSession::onPacketAssumedLostInFlow, this, _1, _2));
#endif

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
      VLOG(2) << "Creating RTCP manager for flow " << it->first
              << " Local IF: " << localRtpEp.getAddress() << ":" << localRtpEp.getPort() << ":" << localRtcpEp.getPort()
              << " Remote IF: " << rtpEp.getAddress() << ":" << rtpEp.getPort() << ":" << rtcpEp.getPort();

      std::unique_ptr<mprtp::MpRtcpReportManager> pMpRtcpReportManager = std::unique_ptr<mprtp::MpRtcpReportManager>(
            new mprtp::MpRtcpReportManager(m_rIoService, rtpParameters,
                                    vSessionDbs[0],
                                    m_rtpSessionStatistics, it->first));
      // NB: bind interface id and endpoint for later usage
      pMpRtcpReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1, uiInterfaceId, rtcpEp));
      pMpRtcpReportManager->setFeedbackCallback(std::bind(&RtpSession::onGenerateFeedbackMpRtp, this, it->first));
//      if (m_pMpFeedbackManager)
//      {
//        // init feedback manager
//        m_pMpFeedbackManager->init(uiInterfaceId, pMpRtcpReportManager.get());

//        // configure callback used for feedback information
//        pMpRtcpReportManager->setFeedbackCallback(std::bind(&mprtp::FeedbackManager::retrieveFeedbackReports, std::ref(m_pMpFeedbackManager), uiInterfaceId));
//      }

      vRtcpReportManagers.push_back( std::move(pMpRtcpReportManager));
    }
  }
  else
  {
    if ( rfc3550::isAudioVisualProfile(rtpParameters) )
    {
      RtcpReportManager::ptr pRtcpReportManager = RtcpReportManager::create(m_rIoService, rtpParameters,
                                                                            vSessionDbs[0], m_rtpSessionStatistics);
      pRtcpReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1));
      vRtcpReportManagers.push_back( std::move(pRtcpReportManager));
    }
    else if (rfc4585::isAudioVisualProfileWithFeedback(rtpParameters))
    {
      rfc4585::RtcpReportManager::ptr pFbReportManager = rfc4585::RtcpReportManager::create(m_rIoService, rtpParameters,
                                                                                            vSessionDbs[0], m_rtpSessionStatistics);

      m_pFeedbackManager = std::unique_ptr<rfc4585::FeedbackManager>( new rfc4585::FeedbackManager(rtpParameters, applicationParameters, m_rIoService ));
      m_pFeedbackManager->setLossCallback(boost::bind(&RtpSession::onPacketAssumedLost, this, _1));

      // configure callback used for feedback information
      pFbReportManager->setFeedbackCallback(std::bind(&RtpSession::onGenerateFeedback, this));
      // pass ownership of pointer
      // pRtcpReportManager = std::unique_ptr<rfc4585::RtcpReportManager>(pFbReportManager);
      pFbReportManager->setRtcpHandler(boost::bind(&RtpSession::onRtcpReportGeneration, this, _1));
      vRtcpReportManagers.push_back( std::move(pFbReportManager));
    }
    else
    {
      // unsupported profile: this will cause the RTP session setup to fail
    }
  }

  return vRtcpReportManagers;
}

std::vector<RtpSessionState> RtpSession::createRtpSessionStates(const RtpSessionParameters& rtpParameters)
{
  std::vector<RtpSessionState> vRtpSessionState;
  // primary RTP flow state
  vRtpSessionState.push_back(RtpSessionState());
  // TODO: add RTX flow state?
  if (rtpParameters.isRetransmissionEnabled())
  {
    vRtpSessionState.push_back(RtpSessionState());
  }
  return vRtpSessionState;
}

std::vector<RtpFlow> RtpSession::createRtpFlows(const RtpSessionParameters& rtpParameters)
{
  std::vector<RtpFlow> vRtpFlows;
  // There is only one instance of each type for standard RTP
  vRtpFlows.push_back(RtpFlow(0, 0, 0, 0, 0, 0, 0));
  // TODO: add RTX flow?
  if (rtpParameters.isRetransmissionEnabled())
  {
    vRtpFlows.push_back(RtpFlow(0, 0, 0, 1, 0, 0, 0));
  }
  return vRtpFlows;
}

void RtpSession::onRtcpReportGeneration(const CompoundRtcpPacket &rtcp)
{
  const EndPoint rtcpEp = m_parameters.getRemoteEndPoint(0).second;
  if (!m_vRtpInterfaces[0]->send(rtcp, rtcpEp))
  {
    LOG(WARNING) << "Failed to send RTCP packet";
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
    LOG(WARNING) << "Failed to send RTCP packet";
    // manually call handler for RTCP to allow base class to cleanly shutdown
    // the RTP session when a BYE packet is sent.
    onOutgoingRtcp(rtcp, rtcpEp);
  }
}

std::vector<RtpPacket> RtpSession::generateRtpPackets(MediaSample::ptr pMediaSample)
{
  std::vector<RtpPacket> packets = m_vPayloadPacketisers[0]->packetise(pMediaSample);

  LOG_IF(WARNING, packets.empty()) << "RTP packetizer generated 0 packets";
  uint32_t uiRtpTimestamp = convertToRtpTimestamp(pMediaSample->getStartTime(),
      m_parameters.getRtpTimestampFrequency(),
      m_parameters.getRtpTimestampBase()
      );

  std::for_each(packets.begin(), packets.end(), [ this, uiRtpTimestamp ](RtpPacket& packet)
      {
      RtpHeader header = packet.getHeader();
      header.setPayloadType( m_parameters.getPayloadType() );
      header.setSequenceNumber(m_parameters.getNextSequenceNumber());
      header.setRtpTimestamp(uiRtpTimestamp);
      header.setSSRC(m_parameters.getSSRC());
      packet.setRtpHeader(header);
      });
  return packets;
}

void RtpSession::storeRtpPacketForRetransmissionAndSchedulePacketRemoval( const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiRtxTimeMs )
{
  // TODO: add mutexes if access can be mt?
  // Don't store retransmissions in the buffer
  if (!m_parameters.isRtxPayloadType(rtpPacket.getHeader().getPayloadType()))
  {
    VLOG(15) << "Storing and scheduling " << rtpPacket.getSequenceNumber() << " for removal in " << uiRtxTimeMs << "ms";
    deadline_timer* pTimer = new deadline_timer(m_rIoService);
    pTimer->expires_from_now(boost::posix_time::milliseconds(uiRtxTimeMs));
    pTimer->async_wait(boost::bind(&RtpSession::onRemoveRtpPacketTimeout, this, _1, pTimer, rtpPacket.getSequenceNumber()) );

    {
      boost::mutex::scoped_lock l( m_rtxLock );
      m_mRtxMap.insert(std::make_pair(rtpPacket.getSequenceNumber(), rtpPacket));
      m_mRtxTimers.insert(std::make_pair(pTimer, pTimer));
    }
  }
}

void RtpSession::onRemoveRtpPacketTimeout(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, uint16_t uiSN)
{
  VLOG(15) << "Removing " << uiSN << " from rtx";
  // remove timer from map
  {
    boost::mutex::scoped_lock l( m_rtxLock );
    std::size_t uiErased = m_mRtxTimers.erase(pTimer);
    assert(uiErased == 1);
    m_mRtxMap.erase(uiSN);
  }
  delete pTimer;
}

RtpPacket RtpSession::generateRtxPacketSsrcMultiplexing(const RtpPacket& rtpPacket)
{
  // Store original SN
  uint16_t uiOriginalSequenceNumber = rtpPacket.getSequenceNumber();

  // copy original RTP header
  RtpHeader header = rtpPacket.getHeader();
  // update PT to be RTX PT
  header.setPayloadType( m_parameters.getRetransmissionPayloadType() );
  // update PT to be RTX SN
  const RtpFlow& flow = getRtpFlow(RTX_FLOW_INDEX);
  const RtpSessionState& sessionState = getSessionState(flow.getRtpSessionState());
  header.setSequenceNumber(sessionState.getNextSequenceNumber());
  // update RTX SSRC
  header.setSSRC(sessionState.getSSRC());
//  header.setSequenceNumber(m_parameters.getNextRetransmissionSequenceNumber());
  // update RTX SSRC
//  header.setSSRC(m_parameters.getRetransmissionSSRC());

  // create new RTP packet
  RtpPacket packet;
  packet.setRtpHeader(header);

  // use original SN as first 2 bytes of payload

  Buffer newPayload;
  uint32_t uiNewSize = rtpPacket.getPayload().getSize() + 2;
  uint8_t* pNewPayload = new uint8_t[uiNewSize];
  newPayload.setData(pNewPayload, uiNewSize);
  OBitStream out(newPayload);
  out.write(uiOriginalSequenceNumber, 16);
  const uint8_t* pData = rtpPacket.getPayload().data();
  bool bRes = out.writeBytes(pData, rtpPacket.getPayload().getSize());
  assert(bRes);
  packet.setPayload(newPayload);

  VLOG(5) << "Generated RTX packet for SN " << rtpPacket.getSequenceNumber()
          << " old payload size: " << rtpPacket.getPayload().getSize()
          << " new payload size: " << packet.getPayload().getSize();
  return packet;
}

void RtpSession::onRtxRequest(uint16_t uiSN, const EndPoint &ep)
{
  // check if we still have the packet in our retransmission buffer
  // TODO: add mutexes if access can be mt?
  auto it = m_mRtxMap.find(uiSN);
  if (it != m_mRtxMap.end())
  {
    VLOG(3) << "Sending RTX for SN " << uiSN;
    // we still have the packet in our retransmission buffer
    RtpPacket rtxRtpPacket = generateRtxPacketSsrcMultiplexing(it->second);
    // TODO: might need to add some virtual methods to handle MPRTP and RTP
    // in the case of multipath, we need to remove the RTP header extension
    // and need some way of selecting the new path
    const EndPoint& endpoint = m_parameters.getRemoteEndPoint(0).first;
    m_vRtpInterfaces[0]->send(rtxRtpPacket, endpoint);
  }
  else
  {
    VLOG(3) << "Couldn't find RTX " << uiSN << " in buffer";
  }
}

void RtpSession::processStandardRtpPacket( const RtpPacket& rtpPacket, const EndPoint &ep )
{
  // HACK for now to configure source SSRC in feedback manager
  if (m_pFeedbackManager && m_pFeedbackManager->getSSRC() == 0)
  {
    m_pFeedbackManager->setSSRC(rtpPacket.getSSRC());
  }

  // only do RTO if we can also send NACKs
  if (m_parameters.isRetransmissionEnabled())
  {
    assert(m_pFeedbackManager);
    if (m_bIsMpRtpSession)
    {
      // search for MPRTP header extension
      rfc5285::RtpHeaderExtension::ptr pHeaderExtension = rtpPacket.getHeader().getHeaderExtension();
      if (pHeaderExtension)
      {
        // Potentially expensive???: create a work around if this proves to be a problem
        mprtp::MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
        if (pSubflowHeader)
        {
          uint16_t uiFlowId = pSubflowHeader->getFlowId();
          uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
          m_pFeedbackManager->onPacketArrival(rtpPacket.getArrivalTime(), rtpPacket.getSequenceNumber(), uiFlowId, uiFlowSpecificSequenceNumber);
        }
      }
    }
    else
    {
      m_pFeedbackManager->onPacketArrival(rtpPacket.getArrivalTime(), rtpPacket.getSequenceNumber());
    }
  }

  // standard RTP packet
  m_vSessionDbs[0]->processIncomingRtpPacket(rtpPacket, ep);

  // check if source has been validated before using resources
  if (m_vSessionDbs[0]->isSourceValid(rtpPacket.getSSRC()))
  {
    bool bLate = false, bDuplicate = false;
    if (!m_bValidated)
    {
      m_bValidated = true;
      // copy packets from temporary packets into playout buffer
      // add packet into jitter buffer
      for (size_t i = 0; i < m_vTemporaryRtpBuffer.size(); ++i)
      {
        insertPacketIntoPlayoutBufferAndScheduleIfRequired(m_vTemporaryRtpBuffer[i], bLate, bDuplicate);
      }
      m_vTemporaryRtpBuffer.clear();
    }
    // HACK for NTP measurement
    // check for NTP timestamp in payload
//#define MEASURE_ONE_WAY_RTT
#ifdef MEASURE_ONE_WAY_RTT
      double dRtt = extractRTTFromRtpPacket(rtpPacket);
      if (dRtt != -1)
        LOG_EVERY_N(INFO, 250) << " One Way Delay: " << dRtt << "s" ;
#endif

#define COLLECT_TRACES_ONE_WAY_DELAY
#ifdef COLLECT_TRACES_ONE_WAY_DELAY
      double dRtt = extractRTTFromRtpPacket(rtpPacket);
      if (dRtt != -1)
        VLOG(2) << "SN: " << rtpPacket.getExtendedSequenceNumber() << " One Way Delay: " << dRtt << "s";
#endif

    // add packet into jitter buffer
    insertPacketIntoPlayoutBufferAndScheduleIfRequired(rtpPacket, bLate, bDuplicate);
  }
  else
  {
    m_bValidated = false;
    // store packet temporarily until SSRC has been validated
    m_vTemporaryRtpBuffer.push_back(rtpPacket);
  }

  m_rtpSessionStatistics.onReceiveRtpPacket(rtpPacket, ep);
}

void RtpSession::processRetransmission( const RtpPacket& rtpPacket, const EndPoint &ep )
{
  VLOG(2) << "Processing retransmission with PT " << (int)rtpPacket.getHeader().getPayloadType();
  // handle retransmission: double check configuration
  if (m_parameters.isRetransmissionEnabled())
  {
    // update session DB: this keeps state of the packet count, etc.
    m_vSessionDbs[0]->processIncomingRtpPacket(rtpPacket, ep);

    // get original sequence number
    IBitStream in(rtpPacket.getPayload());
    uint16_t uiOriginalSN = 0;
    in.read(uiOriginalSN, 16);

    // strip original SN
    // TODO: we could get rid of this extra unnecessary data copy
    uint32_t uiNewSize = rtpPacket.getSize() - 2;
    uint8_t* pNewBuffer = new uint8_t[uiNewSize];
    Buffer newPayload(pNewBuffer, uiNewSize);
    memcpy(pNewBuffer, rtpPacket.getPayload().data() + 2, uiNewSize);
    // update payload
    RtpPacket packet = rtpPacket;
    // update original sequence number!!
    packet.getHeader().setSequenceNumber(uiOriginalSN);
    packet.setPayload(newPayload);

    // add packet into jitter buffer
    bool bLate = false, bDuplicate = false;
    insertPacketIntoPlayoutBufferAndScheduleIfRequired(packet, bLate, bDuplicate);

    if (m_pFeedbackManager)
    {
      if (m_bIsMpRtpSession)
      {
        rfc5285::RtpHeaderExtension::ptr pHeaderExtension = rtpPacket.getHeader().getHeaderExtension();
        if (pHeaderExtension)
        {
          // Potentially expensive???: create a work around if this proves to be a problem
          mprtp::MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<mprtp::MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
          if (pSubflowHeader)
          {
            uint16_t uiFlowId = pSubflowHeader->getFlowId();
            uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
            m_pFeedbackManager->onRtxPacketArrival(rtpPacket.getArrivalTime(), uiOriginalSN, uiFlowId, uiFlowSpecificSequenceNumber, bLate, bDuplicate);
          }
        }
      }
      else
      {
        m_pFeedbackManager->onRtxPacketArrival(rtpPacket.getArrivalTime(), uiOriginalSN, bLate, bDuplicate);
      }
    }
  }
  else
  {
    LOG(WARNING) << "Retransmission not enabled for RTP session. Check SDP";
  }
}

}
