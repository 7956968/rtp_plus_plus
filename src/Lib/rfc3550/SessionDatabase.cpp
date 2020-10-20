#include <CorePch.h>
#include <rtp++/rfc3550/SessionDatabase.h>
#include <functional>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <rtp++/RtpSessionState.h>
#include <rtp++/rfc3550/ComputeRtcpInterval.h>
#include <rtp++/rfc3611/XrMemberEntry.h>

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace rfc3550
{

SessionDatabase::ptr SessionDatabase::create(RtpSessionState& rtpSessionState,
                                             const RtpSessionParameters &rtpParameters )
{
  return std::unique_ptr<SessionDatabase>( new SessionDatabase(rtpSessionState, rtpParameters) );
}

SessionDatabase::SessionDatabase( RtpSessionState& rtpSessionState,
                                  const RtpSessionParameters &rtpParameters )
  :m_rtpSessionState(rtpSessionState),
    m_rtpParameters(rtpParameters),
    m_uiRtcpBandwidthFraction(RECOMMENDED_RTCP_BANDWIDTH_PERCENTAGE),
    m_dTransmissionInterval(0),
    m_bUseReducedMinimum(false),
    m_bXrEnabled(rtpParameters.isXrEnabled())
{
  srand((unsigned)time(NULL));
  RFC3550_initialization();

  // insert own SSRC(s) into DB here
  initialiseLocalSSRCs();

  LOG_IF(INFO, m_bXrEnabled) << "This session is RTCP-XR enabled";
}

SessionDatabase::~SessionDatabase()
{
  std::for_each(m_memberDb.begin(), m_memberDb.end(), [](MemberEntry_t& entry)
  {
    MemberEntry* pEntry = entry.second;
    delete pEntry;
  });
}

void SessionDatabase::updateSSRCs()
{
  // re-initialise local SSRCS: if the SSRC already exists it won't be added
  // Note: the old SSRC will be timed out by checkMemberDatabase
  initialiseLocalSSRCs();
}

void SessionDatabase::initialiseLocalSSRCs()
{
  const std::vector<uint32_t> vSSRCs = m_rtpSessionState.getLocalSSRCs();
  bool bIsSender = m_rtpSessionState.isSender();
  for (uint32_t uiSSRC : vSSRCs)
  {
    MemberEntry* pMember = insertSSRCIfNotInSessionDb(uiSSRC);
    // No need to validate own SSRC
    pMember->setValidated();
    // override sender attribute since we know we want to send data
    pMember->setSender(bIsSender);
    // disable the loss detection for the own SSRC:
    // profiling has shown this operation to be expensive
    // and there is no need for loss estimation of the own
    // outgoing packets
    pMember->disableDetailedLossDetection();
  }
}

bool SessionDatabase::isSourceValid(uint32_t uiSSRC) const
{
  auto it = m_memberDb.find(uiSSRC);
  if (it != m_memberDb.end())
  {
    return it->second->isValid();
  }
  else
  {
    return false;
  }
}

bool SessionDatabase::isSender(const uint32_t uiSSRC) const
{
  // get our member entry
  auto it = m_memberDb.find(uiSSRC);
  if (it == m_memberDb.end()) return false;
  else
  {
    const MemberEntry* pEntry = it->second;
    // store previous interval history
    return (pEntry->isSender());
  }
}

bool SessionDatabase::isSourceSynchronised(const uint32_t uiSSRC) const
{
  auto it = m_memberDb.find(uiSSRC);
  if (it == m_memberDb.end()) return false;
  else
  {
    const MemberEntry* pEntry = it->second;
    // store previous interval history
    return (pEntry->isRtcpSychronised());
  }
}

bool SessionDatabase::isOurSSRC(uint32_t uiSSRC) const
{
  return (uiSSRC == m_rtpSessionState.getSSRC() ||
          uiSSRC == m_rtpSessionState.getRtxSSRC()
         );
}

RtcpReportData_t SessionDatabase::gatherRTCPReportDataAndBeginNewInterval()
{
  // get our member entry
  auto it = m_memberDb.find(m_rtpSessionState.getSSRC());
  assert(it != m_memberDb.end());
  MemberEntry* pOurEntry = it->second;
  if (pOurEntry->isSender())
  {
    //VLOG(2) << "WE ARE A SENDER";
    m_we_sent = true;
  }
  else
  {
    //VLOG(2) << "WE ARE NOT YET A SENDER";
    m_we_sent = false;
  }

  std::vector<MemberEntry*> vSenderList;

  // we do this by checking if the participant *sent* a packet
  // only add a reportblock if the participant sent something

  std::for_each(m_memberDb.begin(), m_memberDb.end(), [&vSenderList, this]( std::pair<const uint32_t, MemberEntry*>& entry)
  {
    // check if we have received data in the last reporting interval from each participant
    MemberEntry* pEntry = entry.second;
    if (pEntry->isSender())
    {
      if (!isOurSSRC(entry.first))
      {
        pEntry->finaliseRRData();
        vSenderList.push_back(pEntry);
      }
    }
    pEntry->newReportingInterval();
  });

  // add receiver info
  std::vector<MemberEntry*> vReceiverList;
  std::for_each(m_memberDb.begin(), m_memberDb.end(), [&vReceiverList, this]( std::pair<const uint32_t, MemberEntry*>& entry)
  {
    // check if we have received data in the last reporting interval from each participant
    MemberEntry* pEntry = entry.second;
    if (!pEntry->isSender())
    {
      if (!isOurSSRC(entry.first))
      {
        pEntry->finaliseData();
        vReceiverList.push_back(pEntry);
      }
    }
    pEntry->newReportingInterval();
  });

  return std::make_tuple(m_we_sent, vSenderList, vReceiverList);
}

void SessionDatabase::onSendRtpPacket(const RtpPacket& packet, const EndPoint &ep)
{
  // can we call processIncomingRtpPacket here to update the session db state of the local participant???
  // TODO: REMOVE?
  processOutgoingRtpPacket(packet);
}

void SessionDatabase::onSendRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint &ep)
{
  // update initial flag
  if (m_initial)
  {
    m_initial = false;
    m_tp = boost::posix_time::microsec_clock::universal_time();
  }
  else
  {
    m_tp_prev = m_tp;
    m_tp = boost::posix_time::microsec_clock::universal_time();
  }

  m_avg_rtcp_size = 0.0625 * compoundRtcpPacketSize(compoundPacket) + 0.9375 * m_avg_rtcp_size;

#ifdef DEBUG_RTCP_SCHEDULING
  LOG(INFO) << "Updated average RTCP size: " << m_avg_rtcp_size;
#endif

  // HACK TO get DB to process outgoing packets
#ifdef PROCESS_OUTGOING_AS_INCOMING
  // can we call processIncomingRtcpPacket here to update the session db???
  processIncomingRtcpPacket(compoundPacket, ep);
#else
  /* 6.3.3
     For each compound RTCP packet received, the value of avg_rtcp_size is
     updated:

     avg_rtcp_size = (1/16) * packet_size + (15/16) * avg_rtcp_size

     where packet_size is the size of the RTCP packet just received.
  */
  m_avg_rtcp_size = 0.0625 * compoundRtcpPacketSize(compoundPacket) + 0.9375 * m_avg_rtcp_size;
#endif
}

void SessionDatabase::processOutgoingRtpPacket(const RtpPacket& packet)
{
  // update local state
  // this method also uses the MemberEntry info to set the extended RTP sequence number on the packet
#if 0
  VLOG(2) << "DBG: SSRC: " << packet.getSSRC() << " in DB: " << (m_memberDb.find(packet.getSSRC()) != m_memberDb.end());
  assert(m_memberDb.find(packet.getSSRC()) != m_memberDb.end());
#endif
  m_memberDb[packet.getSSRC()]->onSendRtpPacket(packet);
}

void SessionDatabase::processIncomingRtpPacket(const RtpPacket& packet, const EndPoint &ep, uint32_t uiRtpTimestampFrequency, bool &bIsSSRCValid, bool &bIsRtcpSyncronised, boost::posix_time::ptime& tPresentation)
{
  /*
   * When an RTP or RTCP packet is received from a participant whose SSRC
   * is not in the member table, the SSRC is added to the table, and the
   * value for members is updated once the participant has been validated
   * as described in Section 6.2.1.
   */
  uint32_t uiSSRC = packet.getSSRC();
  processParticipant(uiSSRC, packet);

  bIsSSRCValid = isSourceValid(uiSSRC);
  // update local state
  // this method also uses the MemberEntry info to set the extended RTP sequence number on the packet
  m_memberDb[packet.getSSRC()]->onReceiveRtpPacket(packet, uiRtpTimestampFrequency, bIsRtcpSyncronised, tPresentation);

  /*
   * The same processing occurs for each
   * CSRC in a validated RTP packet.
   */
  const std::vector<uint32_t> vCSRCs = packet.getHeader().getCSRCs();
  std::for_each(vCSRCs.begin(),
      vCSRCs.end(),
      [this, packet](uint32_t uiCSRC)
  {
    processParticipant(uiCSRC, packet);
  });
}

void SessionDatabase::processIncomingRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep)
{
  std::for_each(compoundPacket.begin(), compoundPacket.end(), [this, ep](RtcpPacketBase::ptr pRtcpPacket)
  {
    processIncomingRtcpPacket(*(pRtcpPacket.get()), ep);
  });

  /* 6.3.3
     For each compound RTCP packet received, the value of avg_rtcp_size is
     updated:

     avg_rtcp_size = (1/16) * packet_size + (15/16) * avg_rtcp_size

     where packet_size is the size of the RTCP packet just received.
  */
  m_avg_rtcp_size = 0.0625 * compoundRtcpPacketSize(compoundPacket) + 0.9375 * m_avg_rtcp_size;

#ifdef DEBUG_RTCP_SCHEDULING
  LOG(INFO) << "Updated average RTCP size: " << m_avg_rtcp_size;
#endif
}

void SessionDatabase::processIncomingRtcpPacket(const RtcpPacketBase& packet, const EndPoint &ep)
{
  doProcessIncomingRtcpPacket(packet, ep);
}

bool SessionDatabase::isNewSSRC(uint32_t uiSSRC) const
{
  return m_memberDb.find(uiSSRC) == m_memberDb.end();
}

void SessionDatabase::processParticipant(uint32_t uiSSRC, const RtpPacket& packet)
{
  // create entry in the database
  MemberEntry* pEntry = insertSSRCIfNotInSessionDb(uiSSRC);
  // This step is important if the SSRC is added to the DB via RTCP
  if (!pEntry->hasBeenIntialisedWithSN())
    pEntry->init(packet);
}

void SessionDatabase::processReceiverReportBlocks(MemberEntry* pEntry, uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const std::vector<rfc3550::RtcpRrReportBlock>& vReports)
{
  // NOTE FIXME TODO : the design needs to be revised: now that the local SSRC variable has been introduced in the MemberEntry
  // we don't have to handle reports at the SessionDatabase level
  std::for_each(vReports.begin(), vReports.end(), [this, pEntry, uiReporterSSRC, uiNtpArrivalTs](const rfc3550::RtcpRrReportBlock& rrBlock)
  {
    if (isOurSSRC(rrBlock.getReporteeSSRC()))
    {
       // update the entry
       pEntry->processReceiverReportBlock(uiReporterSSRC, uiNtpArrivalTs, rrBlock);
    }
    else
    {
       DLOG(INFO) << "Report for other SSRC: " << rrBlock.getReporteeSSRC()
         << " Our SSRC: " << m_rtpSessionState.getSSRC()
         << " RTX SSRC: " << m_rtpSessionState.getRtxSSRC();
    }
  });
}

void SessionDatabase::processSenderReport(const rfc3550::RtcpSr& sr)
{
  MemberEntry* pEntry = insertSSRCIfNotInSessionDb(sr.getSSRC());
  // update the entry
  pEntry->onSrReceived(sr);
  // Moving code to handle report types into MemberEntry
//  const std::vector<rfc3550::RtcpRrReportBlock>& vReports = sr.getReceiverReportBlocks();
//  VLOG(10) << "Received SR with " << vReports.size() << " receiver report blocks";
//  processReceiverReportBlocks(pEntry, sr.getSSRC(), sr.getArrivalTimeNtp(), vReports);
}

void SessionDatabase::processReceiverReport(const rfc3550::RtcpRr& rr)
{
  MemberEntry* pEntry = insertSSRCIfNotInSessionDb(rr.getReporterSSRC());
  // update the entry
  pEntry->onRrReceived(rr);
  // Moving code to handle report types into MemberEntry
//  const std::vector<rfc3550::RtcpRrReportBlock>& vReports = rr.getReceiverReportBlocks();
//#ifdef DEBUG_RTCP
//  VLOG(5) << "Received RR with " << vReports.size() << " receiver report blocks";
//#endif
//  processReceiverReportBlocks(pEntry, rr.getReporterSSRC(), rr.getArrivalTimeNtp(), vReports);
}

void SessionDatabase::processSdesReportBlocks(const std::vector<rfc3550::RtcpSdesReportBlock>& vReports)
{
  std::for_each(vReports.begin(), vReports.end(), [this](const rfc3550::RtcpSdesReportBlock& report)
  {
    uint32_t uiSSRC = report.getSSRC_CSRC1();
    // look up SSRC
    auto it = m_memberDb.find(uiSSRC);
    if (it != m_memberDb.end())
    {
      it->second->updateSdesInfo(report);
    }
  });
}

void SessionDatabase::processBye(const rfc3550::RtcpBye& bye)
{
  const std::vector<uint32_t>& vSSRCs = bye.getSSRCs();
  std::for_each(vSSRCs.begin(), vSSRCs.end(), [this](uint32_t uiSSRC)
  {
    // look up SSRC
    auto it = m_memberDb.find(uiSSRC);
    if (it != m_memberDb.end())
    {
      it->second->onByeReceived();
    }
  });

  // TODO:
  /* Furthermore, to make the transmission rate of RTCP packets more
   adaptive to changes in group membership, the following "reverse
   reconsideration" algorithm SHOULD be executed when a BYE packet is
   received that reduces members to a value less than pmembers:

   o  The value for tn is updated according to the following formula:

   tn = tc + (members/pmembers) * (tn - tc)

   o  The value for tp is updated according the following formula:

   tp = tc - (members/pmembers) * (tc - tp).
   o  The next RTCP packet is rescheduled for transmission at time tn,
   which is now earlier.

   o  The value of pmembers is set equal to members.
   */
}

void SessionDatabase::processXrReport(const rfc3611::RtcpXr& xr)
{
  MemberEntry* pEntry = insertSSRCIfNotInSessionDb(xr.getReporterSSRC());
  // update the entry
  pEntry->onXrReceived(xr);
}

void SessionDatabase::handleOtherRtcpPacketTypes(const RtcpPacketBase& rtcpPacket, const EndPoint& ep)
{
  switch (rtcpPacket.getPacketType())
  {
  default:
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Non-standard RTCP packet type: " << (uint32_t)rtcpPacket.getPacketType() << " not handled";
    break;
  }
  }
}

void SessionDatabase::doProcessIncomingRtcpPacket(const RtcpPacketBase& packet, const EndPoint &ep)
{
  // check if SSRC is in member DB: if not - add it as validated since RTCP validation
  // has already taken place
  // Assuming that we are only dealing with compound packets for now:
  // - this means that any SDES, APP or BYE is preceded by an SR or RR
  // - As a result we will add SSRCs into the session database if the packet is an SR and RR and can assume
  //   that the SSRC will be in the DB by the time the SDES, etc is processed
  switch (packet.getPacketType())
  {
    case PT_RTCP_SR:
      {
        const rfc3550::RtcpSr& sr = static_cast<const rfc3550::RtcpSr&>(packet);
        processSenderReport(sr);
        break;
      }
    case PT_RTCP_RR:
      {
        const rfc3550::RtcpRr& rr = static_cast<const rfc3550::RtcpRr&>(packet);
        processReceiverReport(rr);
        break;
      }
    case PT_RTCP_SDES:
      {
        const rfc3550::RtcpSdes& sdes = static_cast<const rfc3550::RtcpSdes&>(packet);
        processSdesReportBlocks(sdes.getSdesReports());
        break;
      }
    case PT_RTCP_APP:
      {
        // ignore for now
        LOG_FIRST_N(WARNING, 1) << "RTCP APP is not implemented";
        break;
      }
    case PT_RTCP_BYE:
      {
        const rfc3550::RtcpBye& bye = static_cast<const rfc3550::RtcpBye&>(packet);
        processBye(bye);
        break;
      }
    case rfc3611::PT_RTCP_XR:
    {
      if (m_bXrEnabled)
      {
        const rfc3611::RtcpXr& sr = static_cast<const rfc3611::RtcpXr&>(packet);
        processXrReport(sr);
      }
      else
      {
        LOG_FIRST_N(WARNING, 1) << "XR received but XRs are not enabled for this RTP session";
      }
      break;
    }

    default:
    {
      VLOG(COMPONENT_LOG_LEVEL) << "Non-standard RTCP packet type: " << (uint32_t)packet.getPacketType() << " - attempting parse";
      handleOtherRtcpPacketTypes(packet, ep);
    }
  }
}

uint32_t SessionDatabase::getActiveMemberCount() const
{
  return std::count_if(m_memberDb.begin(), m_memberDb.end(), [](MemberEntry_t entry)
  {
    return entry.second->isValid();
  });
}

uint32_t SessionDatabase::getInactiveMemberCount() const
{
  return std::count_if(m_memberDb.begin(), m_memberDb.end(), [](MemberEntry_t entry)
  {
    return entry.second->isInactive();
  });
}

uint32_t SessionDatabase::getUnvalidatedMemberCount() const
{
  return std::count_if(m_memberDb.begin(), m_memberDb.end(), [](MemberEntry_t entry)
  {
    return entry.second->isUnvalidated();
  });
}

uint32_t SessionDatabase::getTotalMemberCount() const
{
  return m_memberDb.size();
}

uint32_t SessionDatabase::getSenderCount() const
{
  // assert (m_senders == getSenderCount());
  return std::count_if(m_memberDb.begin(), m_memberDb.end(), [](MemberEntry_t entry)
  {
    return entry.second->isSender();
  });
}

void SessionDatabase::checkMemberDatabase()
{
  // iterate over member database and remove timed out participants
  /*
    At occasional intervals, the participant MUST check to see if any of
    the other participants time out.  To do this, the participant
    computes the deterministic (without the randomization factor)
    calculated interval Td for a receiver, that is, with we_sent false.
    Any other session member who has not sent an RTP or RTCP packet since
    time tc - MTd (M is the timeout multiplier, and defaults to 5) is
    timed out.  This means that its SSRC is removed from the member list,
    and members is updated.  A similar check is performed on the sender
    list.  Any member on the sender list who has not sent an RTP packet
    since time tc - 2T (within the last two RTCP report intervals) is
    removed from the sender list, and senders is updated.
  */

  // RG TODO:
  // When using 4585, do we need to change this computation?

  double dTransmissionInterval = rfc3550::computeRtcpIntervalSeconds(false,
                                                            getSenderCount(),
                                                            getActiveMemberCount(),
                                                            getAverageRtcpSize(),
                                                            m_rtpParameters.getSessionBandwidthKbps(),
                                                            false,
                                                            false,
                                                            false); // randomisation should not be applied to timeouts

  boost::posix_time::ptime tCurrent = boost::posix_time::microsec_clock::universal_time();

  uint32_t uiTimeoutMs = static_cast<uint32_t>(dTransmissionInterval * 1000 * TIMEOUT_MULTIPLIER + 0.5);
  boost::posix_time::ptime tTimeout = tCurrent - boost::posix_time::milliseconds(uiTimeoutMs);

  // go through participant list to check
  uint32_t uiSenderTimeoutMs = static_cast<uint32_t>(dTransmissionInterval * 1000 * 2 + 0.5);
  boost::posix_time::ptime tSenderTimeout = tCurrent - boost::posix_time::milliseconds(uiSenderTimeoutMs);

#ifdef DDEBUG_SSRC_TIMEOUT
  VLOG(5) << "SSRC timeout :" << uiTimeoutMs << "ms sender timeout: " << uiSenderTimeoutMs << "ms Deterministic: " << dTransmissionInterval << "s";
#endif

  // go through participant list
  std::vector<uint32_t> vRemovalList;
  std::for_each(m_memberDb.begin(), m_memberDb.end(), [this, &tCurrent, &tTimeout, &tSenderTimeout, &vRemovalList]( MemberEntry_t& entry)
  {
    MemberEntry* pEntry = entry.second;
    // make sure we don't want to time ourselves out
    if ( !isOurSSRC(entry.first) )
    {
      // check for timeout and for inactive members
      if (pEntry->getTimeLastPacketSent() < tTimeout || pEntry->isInactiveAndCanBeRemoved(tCurrent))
      {
        VLOG(2) << "Marking participant for removal: " << hex(entry.first)
                << " Now: " << tCurrent
                << " Last packet sent: " << pEntry->getTimeLastPacketSent()
                << " Timeout: " << tTimeout;
        vRemovalList.push_back(entry.first);
      }
    }

    // Don't want to timeout our own SSRC or RTX SSRC
    // if ( !isOurSSRC(entry.first) )
    {
      // NOTE: RG: how do we handle RX sessions in which there are not frequent
      // RTP packets. Currently we are still sending RTCP to keep the session alive
      // For that reason checking what time the last packet was sent instead of what time
      // the last RTP packet was sent
      // TODO: could still add code to identify which SSRC belongs to a sender RX session
      // update the sender status of members
      if (pEntry->isSender() && pEntry->getTimeLastPacketSent() < tSenderTimeout)
      {
        VLOG(2) << "Participant " << hex(entry.first) << " is no longer a sender";
        pEntry->setSender(false);
      }
    }
  });

  // remove from session database
  std::for_each(vRemovalList.begin(), vRemovalList.end(), [this](uint32_t uiSSRC)
  {
    auto it = m_memberDb.find(uiSSRC);
    if (it != m_memberDb.end())
    {
      VLOG(2) << "Removing participant " << hex(uiSSRC) << " from session database";
      delete it->second;
      m_memberDb.erase(it);
    }
  });
}

void SessionDatabase::RFC3550_initialization()
{
  m_uiPrevMembers = 1;
  m_members = 1;
  m_senders = 0;
  // rtcp bw is measured in octets per second (* 1000/8 (= 125))
  m_rtcp_bw = m_rtpParameters.getSessionBandwidthKbps() * (m_uiRtcpBandwidthFraction / 100.0) * 125;
  m_we_sent = false;
  m_avg_rtcp_size = calculateEstimatedRtcpSize();
#ifdef DEBUG_RTCP_SCHEDULING
  LOG(INFO) << "Estimated average RTCP size: " << m_avg_rtcp_size;
#endif
  m_initial = true;
}

uint32_t SessionDatabase::calculateEstimatedRtcpSize() const
{
  // length of SDES
  uint32_t uiLen = 2 + m_rtpParameters.getSdesInfo().getCName().length();
  uint32_t uiWords = (uiLen >> 2);
  if (uiLen % 4 != 0) ++uiWords;

  // calculate size of empty SR with no RR info + SDES with CName
  return 20 + 8 /* IP/UDP */
    + (( 8 /* RR */+ 2 /* SDES */ + uiWords /* CNAME */  ) << 2);
}

MemberEntry* SessionDatabase::lookupMemberEntryInSessionDb(uint32_t uiSSRC)
{
  auto it = m_memberDb.find(uiSSRC);
  if (it == m_memberDb.end())
  {
    return nullptr;
  }
  else
  {
    return it->second;
  }
}

MemberEntry* SessionDatabase::createMemberEntry(uint32_t uiSSRC)
{
  MemberEntry* pNewEntry = NULL;
  if (!m_bXrEnabled)
  {
    pNewEntry = new MemberEntry(uiSSRC);
  }
  else
  {
    pNewEntry = new rfc3611::XrMemberEntry(uiSSRC);
  }
  return pNewEntry;
}

MemberEntry* SessionDatabase::insertSSRCIfNotInSessionDb(uint32_t uiSSRC)
{
  auto it = m_memberDb.find(uiSSRC);
  if (it == m_memberDb.end())
  {
    DLOG(INFO) << "Inserting new SSRC into DB: " << hex(uiSSRC);
    m_memberDb[uiSSRC] = createMemberEntry(uiSSRC);

    // Member entries need to know what the local SSRCs are
    // so that they can determine if a report is important
    // and needs to be processed
    // add local SSRCS for report types that are about the local participant
    m_memberDb[uiSSRC]->addLocalSSRC(m_rtpSessionState.getSSRC());
    if (m_rtpParameters.isRetransmissionEnabled())
    {
      m_memberDb[uiSSRC]->addLocalSSRC(m_rtpSessionState.getRtxSSRC());
    }

    // turn of detailed loss detection: the "analyse" cmd flag has been added which
    // stores all sequence numbers and logs them at the end of the session. This is more
    // accurate in scenarios where packets may arrive out of order
    m_memberDb[uiSSRC]->disableDetailedLossDetection();

    // set update callback here so that we always forward notifications
    m_memberDb[uiSSRC]->setUpdateCallback(std::bind(&SessionDatabase::onMemberUpdate, this, std::placeholders::_1));
  }
  return m_memberDb[uiSSRC];
}

} // rfc3550
} // rtp_plus_plus
