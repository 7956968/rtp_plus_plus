#include "CorePch.h"
// #include <rtp++/IRtpSessionInfoManager.h>
#include <rtp++/RtpTime.h>
#include <rtp++/mprtp/MpRtcpReportManager.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpMemberEntry.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc3550/Rfc3550.h>

namespace rtp_plus_plus
{
namespace mprtp
{

// #define DEBUG_MP_RTCP

MpRtcpReportManager::MpRtcpReportManager(boost::asio::io_service& ioService,
                                         RtpSessionState& rtpSessionState,
                                         const RtpSessionParameters& rtpParameters,
                                         RtpReferenceClock& referenceClock,
                                         std::unique_ptr<rfc3550::SessionDatabase>& pSessionDatabase,
                                         const RtpSessionStatistics& rtpSessionStatistics,
                                         uint16_t uiFlowId)
  :rfc4585::RtcpReportManager(ioService, rtpSessionState, rtpParameters,
                              referenceClock, pSessionDatabase, rtpSessionStatistics),
    m_uiFlowId(uiFlowId)
{
}

CompoundRtcpPacket MpRtcpReportManager::createAdditionalReports(const rfc3550::RtcpReportData_t& reportData, const CompoundRtcpPacket& compoundPacket)
{
  // first call RFC4585 base class method
  CompoundRtcpPacket additionalRtcpPackets = rfc4585::RtcpReportManager::createAdditionalReports(reportData, compoundPacket);
  // Now add MPRTP reports

// #define DISABLE_MPRTCP
#ifdef DISABLE_MPRTCP
  return additionalRtcpPackets;
#endif

#if 0
  VLOG(5) << "Adding MPRTCP packet to RTCP report";
#endif

  // add flow specific reports to compound report
  // look if we were a sender
  MpRtcp::ptr pMpRtcp = MpRtcp::create();
  pMpRtcp->setSSRC(m_rtpSessionState.getSSRC());
  rfc3550::RtcpSr::ptr pSr;
  // should only add flow specific SR if packets have been sent on this flow
  const ChannelStatistics& channelStats = m_rtpSessionStatistics.getFlowSpecificStatistic(m_uiFlowId);
  uint32_t uiTotalPacketsSent = channelStats.getTotalRtpPacketsSent();

  std::vector<rfc3550::RtcpSr::ptr> vSenderReports;
  if ( std::get<0>(reportData) &&
       uiTotalPacketsSent > 0)
  {
    vSenderReports = generateSenderReports(m_rtpSessionStatistics.getFlowSpecificStatistic(m_uiFlowId));
    // TODO FIXME: hack!!! Refactor
    pSr = vSenderReports[0];
  }

  // add receiver reports
  const std::vector<rfc3550::MemberEntry*>& vSenderList = std::get<1>(reportData);
  // TODO: take MTU into account: ignoring for now
  bool bUseSrForRrBlocks = (pSr == 0) ? false : true;
  uint32_t uiCounter = 0;
  rfc3550::RtcpRr::ptr pRr;
  int iLastAddedSrIndex = -1;
  std::for_each(vSenderList.begin(), vSenderList.end(), [&](rfc3550::MemberEntry* pEntry)
  {
    MpRtpMemberEntry& mprtpEntry = static_cast<MpRtpMemberEntry&>(*pEntry);

    // we need to make sure that an entry actually exists for this flow.
    // entries are created when an MPRTP or MPRTCP packet is received.
    if (mprtpEntry.hasFlowSpecificMemberEntry(m_uiFlowId))
    {
      std::unique_ptr<FlowSpecificMemberEntry>& entry = mprtpEntry.getFlowSpecificMemberEntry(m_uiFlowId);

      // check if we have received data in the last reporting interval from each participant
      // check if we need to create a new RR
      if (uiCounter == 0 && !bUseSrForRrBlocks)
      {
        // create new report
        pRr = rfc3550::RtcpRr::create(m_rtpSessionState.getSSRC());
      }

      uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
      RtpTime::getNTPTimeStamp(uiNtpMsw, uiNtpLsw);

      rfc3550::RtcpRrReportBlock block;
      // populate report
      block.setReporteeSSRC(entry->getSSRC());
      block.setFractionLost(entry->getLostFraction());
      block.setCumulativeNumberOfPacketsLost(entry->getCumulativeNumberOfPacketsLost());
      block.setExtendedHighestSNReceived(entry->getExtendedHighestSequenceNumber());
      block.setInterarrivalJitter(entry->getJitter());
      block.setLastSr(entry->getLsr());
      block.setDelaySinceLastSr(entry->getDlsr());

      if (pRr)
        pRr->addReportBlock(block);
      else
        pSr->addReportBlock(block);

      VLOG(2) << "[" << this << "] Creating RR - SSRC: " << hex(m_rtpSessionState.getSSRC()) << block;

      ++uiCounter;

      // if there are 31 RR blocks, reset counter
      if (uiCounter == rfc3550::MAX_SOURCES_PER_RTCP_RR)
      {
        if (bUseSrForRrBlocks)
        {
#ifdef DEBUG_MP_RTCP
          VLOG(2) << "Adding SR for flow " << m_uiFlowId;
#endif
          pMpRtcp->addMpRtcpReport(m_uiFlowId, SUBFLOW_SPECIFIC_REPORT, pSr);
          pSr.reset();
          // check if there's another sender report we can add RR blocks to
          ++iLastAddedSrIndex;

          if ((uint32_t)iLastAddedSrIndex < vSenderReports.size() - 1)
          {
            pSr = vSenderReports[iLastAddedSrIndex + 1];
            bUseSrForRrBlocks = true;
          }
          else
          {
            // we've used up all available SR packets
            bUseSrForRrBlocks = false;
          }
        }
        else
        {
#ifdef DEBUG_MP_RTCP
          VLOG(2) << "Adding RR for flow " << m_uiFlowId;
#endif
          pMpRtcp->addMpRtcpReport(m_uiFlowId, SUBFLOW_SPECIFIC_REPORT, pRr);
        }
        uiCounter = 0; // this will trigger the creation of a new RR
        pRr.reset();
      }
    }
  });

  if (pSr)
  {
#ifdef DEBUG_MP_RTCP
    VLOG(2) << "Adding MPRTCP SR for flow " << m_uiFlowId;
#endif
    pMpRtcp->addMpRtcpReport(m_uiFlowId, SUBFLOW_SPECIFIC_REPORT, pSr);
  }

  if (pRr)
  {
#ifdef DEBUG_MP_RTCP
    VLOG(5) << "Adding RR for flow " << m_uiFlowId;
#endif
    pMpRtcp->addMpRtcpReport(m_uiFlowId, SUBFLOW_SPECIFIC_REPORT, pRr);
  }

  // TODO: create path specific XR reports here
  // TODO: disable global XRs when using MPRTP

  // MPRTCP packet
  additionalRtcpPackets.push_back(pMpRtcp);

  return additionalRtcpPackets;
}

} // mprtp
} // rtp_plus_plus
