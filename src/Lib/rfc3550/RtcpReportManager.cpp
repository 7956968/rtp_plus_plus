#include "CorePch.h"
#include <rtp++/rfc3550/RtcpReportManager.h>
#include <boost/bind.hpp>
// #include <rtp++/IRtpSessionInfoManager.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>
#include <rtp++/rfc3550/RtpConstants.h>
#include <rtp++/rfc3550/SessionDatabase.h>
#include <rtp++/rfc3611/Rfc3611.h>
#include <rtp++/rfc3611/RtcpXrFactory.h>
#include <rtp++/rfc3611/XrMemberEntry.h>

#define USE_REDUCED_MINIMUM false

// #define DEBUG_RTCP

namespace rtp_plus_plus
{
namespace rfc3550
{

RtcpReportManager::ptr RtcpReportManager::create(boost::asio::io_service& ioService,
                                                 RtpSessionState& rtpSessionState,
                                                 const RtpSessionParameters &rtpParameters,
                                                 RtpReferenceClock& referenceClock,
                                                 std::unique_ptr<SessionDatabase>& pSessionDatabase,
                                                 const RtpSessionStatistics& rtpSessionStatistics,
                                                 std::unique_ptr<RtcpTransmissionTimerBase> pTransmissionTimer)
{
  return std::unique_ptr<RtcpReportManager>(
        new RtcpReportManager(ioService, rtpSessionState, rtpParameters, referenceClock, pSessionDatabase, rtpSessionStatistics, std::move(pTransmissionTimer))
        );
}

RtcpReportManager::RtcpReportManager(boost::asio::io_service& ioService,
                                     RtpSessionState& rtpSessionState,
                                     const RtpSessionParameters &rtpParameters,
                                     RtpReferenceClock& referenceClock,
                                     std::unique_ptr<SessionDatabase>& pSessionDatabase,
                                     const RtpSessionStatistics& rtpSessionStatistics,
                                     std::unique_ptr<RtcpTransmissionTimerBase> pTransmissionTimer)
  :m_ioService(ioService),
    m_rtpSessionState(rtpSessionState),
    m_rtcpTimer(ioService),
    m_parameters(rtpParameters),
    m_referenceClock(referenceClock),
    m_pSessionDatabase(pSessionDatabase),
    m_pTransmissionTimer(std::move(pTransmissionTimer)),
    m_bShuttingDown(false),
    m_bInitial(true),
    m_uiRtcpBandwidthFraction(RECOMMENDED_RTCP_BANDWIDTH_PERCENTAGE),
    m_dRtcpBw(0.0),
    m_bUseReducedMinimum(USE_REDUCED_MINIMUM),
    m_rtpSessionStatistics(rtpSessionStatistics),
    m_uiPreviousMembers(0),
    m_dTransmissionInterval(0.0)
{
  // rtcp bw is measured in octets per second (* 1000/8 (= 125))
  m_dRtcpBw = rtpParameters.getSessionBandwidthKbps() * (m_uiRtcpBandwidthFraction / 100.0) * 125;
}

RtcpReportManager::~RtcpReportManager()
{
  VLOG(10) << "~RtcpReportManager";
}

void RtcpReportManager::setRtcpHandler(fnRtcpCb_t rtcpHandler)
{
  m_onRtcp = rtcpHandler;
}

void RtcpReportManager::startReporting()
{
  m_bShuttingDown = false;
  setNextRtcpTimeout();
}

void RtcpReportManager::scheduleFinalReportAndShutdown()
{
  m_bShuttingDown = true;
  m_rtcpTimer.cancel();
}

void RtcpReportManager::setNextRtcpTimeout()
{
  double dTransmissionInterval = m_pTransmissionTimer->computeRtcpIntervalSeconds(
        m_pSessionDatabase->isSender(m_rtpSessionState.getSSRC()),
        m_pSessionDatabase->getSenderCount(),
        m_pSessionDatabase->getActiveMemberCount(),
        m_pSessionDatabase->getAverageRtcpSize(),
        m_parameters.getSessionBandwidthKbps());

  uint32_t uiIntervalMs = static_cast<uint32_t>(dTransmissionInterval * 1000 + 0.5);
  m_dTransmissionInterval = dTransmissionInterval;
  m_uiTransmissionIntervalMs = uiIntervalMs;
#ifdef DEBUG_RTCP_SCHEDULING
  VLOG(10) << "Next RTCP interval expires in " << uiIntervalMs << " ms";
#endif
  m_tPrevious = boost::posix_time::microsec_clock::universal_time();
  m_rtcpTimer.expires_from_now(boost::posix_time::milliseconds(uiIntervalMs));
  m_tNext = m_rtcpTimer.expires_at();
  m_uiPreviousMembers = m_pSessionDatabase->getActiveMemberCount();
  onTimerSet();
  m_rtcpTimer.async_wait(boost::bind(&RtcpReportManager::onRtcpIntervalTimeout, this, _1));
}

void RtcpReportManager::onRtcpIntervalTimeout(const boost::system::error_code& ec)
{
  if (m_bShuttingDown)
  {
    finaliseSession();
  }
  else
  {
    if (!ec)
    {
      // cleanup member DB
      /*
        The participant MUST perform this check at least once per RTCP
        transmission interval.
      */
      m_pSessionDatabase->checkMemberDatabase();
      sendRtcpPacket();
      // schedule next RTCP timeout
      setNextRtcpTimeout();
    }
    else
    {
      if (ec == boost::asio::error::operation_aborted)
      {
        // TODO: this might be the case if we need to recalculate the timer
        VLOG(2) << "Timer cancelled";
      }
      else
      {
        // TODO
        DLOG(INFO) << "[" << this << "] Error: " << ec.message();
      }
    }
  }
}

CompoundRtcpPacket RtcpReportManager::generateCompoundRtcpPacket(bool bWithBye, bool bMinimal)
{
  const RtcpReportData_t reportData = m_pSessionDatabase->gatherRTCPReportDataAndBeginNewInterval();
  CompoundRtcpPacket compoundPacket = generatePeriodicCompoundRtcpPacket(reportData, bMinimal);
  if (bWithBye)
  {
    // add bye
    RtcpBye::ptr pBye = RtcpBye::create(m_rtpSessionState.getSSRC());
    compoundPacket.push_back(pBye);
    // check if there is a retransmission session
    // must only send retransmission session BYE if we are a sender
    if (isRetransmissionSender(m_rtpSessionStatistics.getOverallStatistic()))
    {
      VLOG(2) << "Adding bye for retransmission session";
      pBye->addSSRC(m_rtpSessionState.getRtxSSRC());
    }
    else
    {
      VLOG(10) << "No need to add bye for retransmission session";
    }
  }
  return compoundPacket;
}

void RtcpReportManager::sendRtcpPacket(bool bIsMinimalPacketAllowed)
{
  CompoundRtcpPacket compoundPacket = generateCompoundRtcpPacket(false, bIsMinimalPacketAllowed);
  if (m_onRtcp)
    m_onRtcp(compoundPacket);
  else
    LOG(WARNING) << "No RTCP handler configured!!!";
}

void RtcpReportManager::finaliseSession()
{
  VLOG(5) << "[" << this << "] Shutting down, generating RTCP BYE";
  m_pSessionDatabase->checkMemberDatabase();

  if (m_pSessionDatabase->getActiveMemberCount() > RFC3550_IMMEDIATE_BYE_LIMIT)
  {
    // schedule RTCP BYE according to 3550 6.3.7
    /*
      When the participant decides to leave the system, tp is reset to
      tc, the current time, members and pmembers are initialized to 1,
      initial is set to 1, we_sent is set to false, senders is set to 0,
      and avg_rtcp_size is set to the size of the compound BYE packet.
      The calculated interval T is computed.  The BYE packet is then
      scheduled for time tn = tc + T.
    */
    m_tPrevious = boost::posix_time::microsec_clock::universal_time();
    m_uiPreviousMembers = 1;
    uint32_t uiMembers = 1;
    double dAverageRtcpSize = m_pSessionDatabase->getAverageRtcpSize();
    m_bInitial = true;
    bool bIsSender = false;
    uint32_t uiSenders = 0;
    double dTransmissionInterval = rfc3550::computeRtcpIntervalSeconds(bIsSender,
                                                                uiSenders,
                                                                uiMembers,
                                                                dAverageRtcpSize,
                                                                m_parameters.getSessionBandwidthKbps(),
                                                                m_bUseReducedMinimum,
                                                                m_bInitial,
                                                                true);

    VLOG(10) << "[" << this << "] Sending RTCP BYE after " << dTransmissionInterval << "ms";
    // send packet after a short delay
    scheduleBye(dTransmissionInterval, generateCompoundRtcpPacket(true, false));
  }
  else
  {
    VLOG(10) << "[" << this << "] Sending RTCP BYE immediately";
    // send immediately
    scheduleBye(0, generateCompoundRtcpPacket(true, false));
  }
}

void RtcpReportManager::onScheduleByeTimeout(const boost::system::error_code& ec, const CompoundRtcpPacket& bye)
{
  VLOG(5) << "[" << this << "] RTCP BYE scheduled";
  if (m_onRtcp)
    m_onRtcp(bye);
  else
    LOG(WARNING) << "No RTCP handler configured!!!";
}

CompoundRtcpPacket RtcpReportManager::createAdditionalReports(const RtcpReportData_t& reportData, const CompoundRtcpPacket& compoundPacket)
{
  CompoundRtcpPacket rtcpPacket;
  // if we were a sender
  if (std::get<0>(reportData))
  {
    // For senders
    // DLRR Report Block
    if (m_parameters.isXrEnabled())
    {
      std::string sAttributeValue;
      if (m_parameters.hasXrAttribute(rfc3611::RCVR_RTT, sAttributeValue))
      {
        // TODO: we are not handling the "all" or "sender" SDP attribute values for now, see RFC3611
        // if we have received an XR RRT from a receiver: send the response
        const std::vector<MemberEntry*>& vReceiverList = std::get<2>(reportData);

        if (!vReceiverList.empty())
        {
            VLOG(5) << "Creating RTCP XR DLRR";
            rfc3611::RtcpXr::ptr pXr = rfc3611::RtcpXr::create(m_rtpSessionState.getSSRC());
            // get DLRR info from all receivers
            std::vector<rfc3611::DlrrBlock> vDllrInfo;
            std::for_each(vReceiverList.begin(), vReceiverList.end(), [&](MemberEntry* pEntry)
            {
              MemberEntry& entry = *pEntry;
              // if m_parameters.isXrEnabled() the session database MUST create XR member entries
              rfc3611::XrMemberEntry& xrMemberEntry = static_cast<rfc3611::XrMemberEntry&>(entry);
              rfc3611::DlrrBlock block(xrMemberEntry.getSSRC(), xrMemberEntry.getLastRr(), xrMemberEntry.getDelaySinceLastReceiverReport());
              vDllrInfo.push_back(block);
            });

            rfc3611::RtcpXrReportBlock reportBlock = rfc3611::RtcpXrFactory::createDlrrXrReportBlock(vDllrInfo);
            pXr->addReportBlock(reportBlock);
            rtcpPacket.push_back(pXr);
        }
      }
    }
  }
  else
  {
    // For receivers
    if (m_parameters.isXrEnabled())
    {
      std::string sAttributeValue;
      if (m_parameters.hasXrAttribute(rfc3611::RCVR_RTT, sAttributeValue))
      {
        // TODO: we are not handling the "all" or "sender" SDP attribute values for now, see RFC3611
#if 0
        VLOG(5) << "Creating RTCP XR RRT";
#endif
        rfc3611::RtcpXr::ptr pXr = rfc3611::RtcpXr::create(m_rtpSessionState.getSSRC());
        // Receiver Reference Time Report Block
        uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
        RtpTime::getNTPTimeStamp(uiNtpMsw, uiNtpLsw);
        rfc3611::ReceiverReferenceTimeReportBlock rtBlock(uiNtpMsw, uiNtpLsw);
        rfc3611::RtcpXrReportBlock reportBlock = rfc3611::RtcpXrFactory::createReceiverRttXrReportBlock(rtBlock);
        pXr->addReportBlock(reportBlock);
        rtcpPacket.push_back(pXr);
//        const std::vector<MemberEntry*>& vReceiverList = std::get<2>(reportData);
//        std::for_each(vReceiverList.begin(), vReceiverList.end(), [&](MemberEntry* pEntry)
//        {
//          MemberEntry& entry = *pEntry;
//          rfc3611::XrMemberEntry& xrMemberEntry = static_cast<rfc3611::XrMemberEntry&>(entry);
//          uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
//          getNTPTimeStamp(uiNtpMsw, uiNtpLsw);
//          rfc3611::ReceiverReferenceTimeReportBlock rtBlock(uiNtpMsw, uiNtpLsw);
//          rfc3611::RtcpXrReportBlock reportBlock = rfc3611::RtcpXrFactory::createReceiverRttXrReportBlock(rtBlock);
//          pXr->addReportBlock(reportBlock);
//          rtcpPacket.push_back(pXr);
//        });
      }
    }
  }
  return rtcpPacket;
}

CompoundRtcpPacket RtcpReportManager::generatePeriodicCompoundRtcpPacket(const RtcpReportData_t &reportData, bool bMinimalRtcpPacket)
{
  CompoundRtcpPacket compoundPacket;
  RtcpSr::ptr pSr;
  RtcpRr::ptr pRr;
  uint32_t uiCounter = 0;

  if (!bMinimalRtcpPacket)
  {
    // look if we were a sender: in that case the first packet will be an SR
    if ( std::get<0>(reportData) )
    {
      // TODO: do receiver reports of SSRCs that the sender has received packets from
      // need to be added to the SR?
#ifdef DEBUG_RTCP
      VLOG(2) << "[" << this << "] Creating SR - SSRC: " << hex(m_rtpSessionState.getSSRC());
#endif

      std::vector<RtcpSr::ptr> vSenderReports = generateSenderReports(m_rtpSessionStatistics.getOverallStatistic());
      // HACK: refactor code
      pSr = vSenderReports[0];
      // copy all SRs into compound RTCP packet
      compoundPacket.insert(compoundPacket.end(), vSenderReports.begin(), vSenderReports.end() );
    }

    const std::vector<MemberEntry*>& vSenderList = std::get<1>(reportData);
    std::for_each(vSenderList.begin(), vSenderList.end(), [&](MemberEntry* pEntry)
    {
      if (uiCounter == 0 && (!pSr))
      {
#ifdef DEBUG_RTCP
        VLOG(2) << "[" << this << "] Creating RR - SSRC: " << hex(m_rtpSessionState.getSSRC());
#endif
        pRr = RtcpRr::create(m_rtpSessionState.getSSRC());
        compoundPacket.push_back(pRr);
      }
      MemberEntry& entry = *pEntry;
      RtcpRrReportBlock block;
      // populate report
      block.setReporteeSSRC(entry.getSSRC());
      block.setFractionLost(entry.getLostFraction());
      block.setCumulativeNumberOfPacketsLost(entry.getCumulativeNumberOfPacketsLost());
      block.setExtendedHighestSNReceived(entry.getExtendedHighestSequenceNumber());
      block.setInterarrivalJitter(entry.getJitter());
      block.setLastSr(entry.getLsr());
      block.setDelaySinceLastSr(entry.getDlsr());

      if (pSr)
        pSr->addReportBlock(block);
      else
        pRr->addReportBlock(block);

#ifdef DEBUG_RTT
      uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
      getNTPTimeStamp(uiNtpMsw, uiNtpLsw);
      LOG(INFO) << "DEBUG_RTT: Creating RR: NTP " << hex(uiNtpMsw) << "." << hex(uiNtpLsw)
                <<  " " << convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw) << " " << block;
#else
#ifdef DEBUG_RTCP
      VLOG(2) << "[" << this << "] Adding RR report block for SSRC: " << hex(entry.getSSRC()) << " " << block;
#endif
#endif

      ++uiCounter;

      // if there are 31 RR blocks, reset counter
      if (uiCounter == MAX_SOURCES_PER_RTCP_RR)
      {
        if (pSr)
        {
          // reset SR pointer, subsequent blocks will be added to the Rr
          pSr.reset();
        }
        else
        {
          pRr.reset();
        }
        uiCounter = 0; // this will trigger the creation of a new RR
      }
    });

    // check if any SR/RR were added, if not add empty RR
    if (compoundPacket.empty())
    {
      compoundPacket.push_back(RtcpRr::create(m_rtpSessionState.getSSRC()));
    }

    const SdesInformation sdesInfo = m_parameters.getSdesInfo();
#if 0
    VLOG(2) << "SSRC: " << hex(m_parameters.getSSRC()) << " Generating SDES for CNAME: " << sdesInfo.getCName();
#endif
    // add SDES
    RtcpSdes::ptr pSdes = RtcpSdes::create();
    RtcpSdesReportBlock block(m_rtpSessionState.getSSRC());
    block.setCName(sdesInfo.getCName());
    pSdes->addSdesReport(block);
    compoundPacket.push_back(pSdes);
  }

  // in the case of minimal RTCP reports, add the additional reports here
  CompoundRtcpPacket vAdditionalReports = createAdditionalReports(reportData, compoundPacket);
  if (!vAdditionalReports.empty())
  {
    compoundPacket.insert(compoundPacket.end(), vAdditionalReports.begin(), vAdditionalReports.end());
  }

  return compoundPacket;
}

std::vector<RtcpSr::ptr> RtcpReportManager::generateSenderReports(const ChannelStatistics& rtpStatistic)
{
  std::vector<RtcpSr::ptr> vReports;

  // Note: the timestamp will be ignored if the reference clock is in wall clock mode
  uint32_t uiNtpTimestampMsw = 0;
  uint32_t uiNtpTimestampLsw = 0;
  uint32_t uiRtpTimestamp = m_referenceClock.getRtpAndNtpTimestamps(m_parameters.getRtpTimestampFrequency(),
                                                                    m_rtpSessionState.getRtpTimestampBase(),
                                                                 uiNtpTimestampMsw,
                                                                 uiNtpTimestampLsw);

  // generate sender report for default payload type
  uint8_t uiPt = m_parameters.getPayloadType();

#ifdef DEBUG_RTT
  VLOG(2) << "RTT_DEBUG Creating SR: " << hex(uiNtpTimestampMsw) << "." << hex(uiNtpTimestampLsw)
          << " RTP TS: " << uiRtpTimestamp
          << " Total packets: " << rtpStatistic.getTotalRtpPacketsSent(uiPt)
          << " (" << rtpStatistic.getTotalOctetsSent(uiPt) << " B)";
#endif

  RtcpSr::ptr pSr = RtcpSr::create( m_rtpSessionState.getSSRC(), uiNtpTimestampMsw, uiNtpTimestampLsw, uiRtpTimestamp,
                                    rtpStatistic.getTotalRtpPacketsSent(uiPt),
                                    rtpStatistic.getTotalOctetsSent(uiPt)
                                  );
  vReports.push_back(pSr);

  // for retransmission purposes or for sending multiple payload types in the same RTP session we might have to generate multiple sender reports
  // only send RTX SRs if RTP packets have been sent in the session/last interval
  if (isRetransmissionSender(rtpStatistic))
  {
    uint8_t uiRtxPt = m_parameters.getRetransmissionPayloadType();
    VLOG(5) << "RTT_DEBUG Creating RTX SR: " << hex(uiNtpTimestampMsw) << "." << hex(uiNtpTimestampLsw)
            << " RTP TS: " << uiRtpTimestamp
            << " Total packets: " << rtpStatistic.getTotalRtpPacketsSent(uiRtxPt)
            << " (" << rtpStatistic.getTotalOctetsSent(uiRtxPt) << " B)";
    RtcpSr::ptr pRtxSr = RtcpSr::create( m_rtpSessionState.getRtxSSRC(), uiNtpTimestampMsw, uiNtpTimestampLsw, uiRtpTimestamp,
                                      rtpStatistic.getTotalRtpPacketsSent(uiRtxPt),
                                      rtpStatistic.getTotalOctetsSent(uiRtxPt) );

    vReports.push_back(pRtxSr);
  }
  return vReports;
}

bool RtcpReportManager::isRetransmissionSender(const ChannelStatistics& rtpStatistic) const
{
  // for now checking that at least one RX packet has been sent
  return ((m_parameters.getRetransmissionTimeout() > 0) &&
          (rtpStatistic.getTotalRtpPacketsSent(m_parameters.getRetransmissionPayloadType()) > 0)
         );
}

void RtcpReportManager::scheduleBye(double dTransmissionInterval, const CompoundRtcpPacket& bye)
{
  if (dTransmissionInterval == 0.0)
  {
    // call immediately
    onScheduleByeTimeout(boost::system::error_code(), bye);
  }
  else
  {
    // delay by dTransmissionInterval seconds
    uint32_t uiIntervalMs = static_cast<uint32_t>(dTransmissionInterval * 1000 + 0.5);
    m_rtcpTimer.expires_from_now(boost::posix_time::milliseconds(uiIntervalMs));
    m_rtcpTimer.async_wait(boost::bind(&RtcpReportManager::onScheduleByeTimeout, this, _1, bye));
  }
}

}
}
