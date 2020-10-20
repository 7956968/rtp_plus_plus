#include "CorePch.h"
#include <rtp++/mprtp/MpRtpMemberEntry.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cpputil/Utility.h>
#include <rtp++/RtpTime.h>
#include <rtp++/mprtp/FlowSpecificMemberEntry.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpHeader.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3611/Rfc3611.h>

namespace rtp_plus_plus
{
namespace mprtp
{

MpRtpMemberEntry::MpRtpMemberEntry()
{
  setMaxMisorder(MAX_MISORDER);
}

MpRtpMemberEntry::MpRtpMemberEntry(uint32_t uiSSRC)
  : MemberEntry( uiSSRC )
{
  setMaxMisorder(MAX_MISORDER);
  VLOG(5) << "Creating MPRTP member entry";
}

MpRtpMemberEntry::~MpRtpMemberEntry()
{
  VLOG(5) << "Destructor MPRTP member entry";
}

// override this to initialise flow specific sequence numbers
void MpRtpMemberEntry::init(const RtpPacket& rtpPacket)
{
  // init global sequence number space
  MemberEntry::init(rtpPacket);

  boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
  if (pSubflowHeader)
  {
    uint16_t uiFlowId = pSubflowHeader->getFlowId();
    auto it = m_mFlowSpecificMap.find(uiFlowId);
    if (it == m_mFlowSpecificMap.end())
    {
      VLOG(2) << "First RTP packet: initialising MemberEntry " << hex(m_uiSSRC) << " for flow " << uiFlowId;
      m_mFlowSpecificMap[uiFlowId] = createFlowSpecificMemberEntry(uiFlowId);
      m_mFlowSpecificMap[uiFlowId]->initSequence(pSubflowHeader->getFlowSpecificSequenceNumber());
    }
  }
  else
  {
    LOG(WARNING) << "MPRTP packet has no header extension!";
  }

  /*
  // search for MPRTP header extension
  rfc5285::RtpHeaderExtension::ptr pHeaderExtension = rtpPacket.getHeader().getHeaderExtension();
  if (pHeaderExtension)
  {
    // Potentially expensive???: create a work around if this proves to be a problem
    MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
    if (pSubflowHeader)
    {
      uint16_t uiFlowId = pSubflowHeader->getFlowId();
      auto it = m_mFlowSpecificMap.find(uiFlowId);
      if (it == m_mFlowSpecificMap.end())
      {
        VLOG(2) << "First RTP packet: initialising MemberEntry " << hex(m_uiSSRC) << " for flow " << uiFlowId;
        m_mFlowSpecificMap[uiFlowId] = createFlowSpecificMemberEntry(uiFlowId);
        m_mFlowSpecificMap[uiFlowId]->initSequence(pSubflowHeader->getFlowSpecificSequenceNumber());
      }
    }
  }*/
}

void MpRtpMemberEntry::onMpRtcp( const MpRtcp& mprtcp )
{
// #define DEBUG_NTP
#ifdef DEBUG_NTP
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  split(mprtcp.getArrivalTimeNtp(), uiNtpMsw, uiNtpLsw);
  DLOG(INFO) << "NTP arrival time: " << hex(uiNtpMsw) << "." << hex(uiNtpLsw) << " " << convertNtpTimestampToPosixTime(mprtcp.getArrivalTimeNtp());
#endif

  MpRtcp::ReportMap_t mReports = mprtcp.getRtcpReportMap();
  std::for_each(mReports.begin(), mReports.end(), [=](MpRtcp::ReportPair_t& reportPair)
  {
    uint16_t uiFlowId = reportPair.first;
    MpRtcpReport report = reportPair.second;
    CompoundRtcpPacket vReports = report.getReports();
    std::for_each(vReports.begin(), vReports.end(), [=](RtcpPacketBase::ptr& pRtcpPacket)
    {
      RtcpPacketBase& packet = *(pRtcpPacket.get());
      // copy NTP arrival timestamp since the packet is contained inside the mprtcp message
      // and has not been timestamped
      packet.setArrivalTimeNtp(mprtcp.getArrivalTimeNtp());

      auto it = m_mFlowSpecificMap.find(uiFlowId);
      if (it == m_mFlowSpecificMap.end())
      {
        VLOG(2) << "Initialising MemberEntry " << hex(m_uiSSRC) << " for flow " << uiFlowId;
        m_mFlowSpecificMap[uiFlowId] = std::move(createFlowSpecificMemberEntry(uiFlowId));
      }

      switch (packet.getPacketType())
      {
      case rfc3550::PT_RTCP_SR:
        {
          VLOG(10) <<  "Processing MPRTP SR for flow id " << uiFlowId;
          rfc3550::RtcpSr& sr = const_cast<rfc3550::RtcpSr&>(static_cast<const rfc3550::RtcpSr&>(packet));
          m_mFlowSpecificMap[uiFlowId]->onSrReceived(sr);
          break;
        }
      case rfc3550::PT_RTCP_RR:
        {
          VLOG(10) <<  "Processing MPRTP RR for flow id " << uiFlowId;
          rfc3550::RtcpRr& rr = const_cast<rfc3550::RtcpRr&>(static_cast<const rfc3550::RtcpRr&>(packet));
          m_mFlowSpecificMap[uiFlowId]->onRrReceived(rr);
          break;
        }
      case rfc3611::PT_RTCP_XR:
        {
          VLOG(10) <<  "Processing MPRTP XR for flow id " << uiFlowId;
          rfc3611::RtcpXr& sr = const_cast<rfc3611::RtcpXr&>(static_cast<const rfc3611::RtcpXr&>(packet));
          m_mFlowSpecificMap[uiFlowId]->onXrReceived(sr);
          break;
        }
      }
    });
  });
}

std::unique_ptr<FlowSpecificMemberEntry> MpRtpMemberEntry::createFlowSpecificMemberEntry(uint16_t uiFlowId)
{
  std::unique_ptr<FlowSpecificMemberEntry> pEntry = std::unique_ptr<FlowSpecificMemberEntry>(new FlowSpecificMemberEntry(m_uiSSRC, uiFlowId));
  // Member entries need to know what the local SSRCs are
  // so that they can determine if a report is important
  // and needs to be processed
  for (auto uiSSRC: m_localSSRCs)
  {
    pEntry->addLocalSSRC(uiSSRC);
  }

  // setup forwarding of member updates
  pEntry->setUpdateCallback(std::bind(&MpRtpMemberEntry::onMemberUpdate, this, std::placeholders::_1));
  return pEntry;
}

void MpRtpMemberEntry::finaliseRRData()
{
  // call base class to finalise global RR data
  VLOG(10) << "Finalising MPRTP global data";
  MemberEntry::finaliseRRData();

  std::for_each(m_mFlowSpecificMap.begin(), m_mFlowSpecificMap.end(), [] (FlowSpecificMapEntry_t& pair)
  {
    VLOG(10) << "Finalising MPRTP subflow data: " << pair.first;
    pair.second->finaliseRRData();
  });
}

/// A receiver should call this method so that we can update the state according to the
/// RTCP packet received
void MpRtpMemberEntry::doReceiveRtpPacket(const RtpPacket& rtpPacket)
{
  // Can't call base class method here since multipath packets might never be in sequence and the update_seq algorithm
  // relies on receiving packets in sequence
  // MemberEntry::doReceiveRtpPacket(rtpPacket);
  // call base class method to process rtp packet
  // TODO: refactor
  // replicating base class code: the sequence number jumps on multiple paths
  // don't work with update_seq
  // Nor do the jitter calculations apply!
  m_tLastRtpPacketSent = boost::posix_time::microsec_clock::universal_time();
  m_tLastPacketSent = m_tLastRtpPacketSent;
  m_bIsSender = true;
  ++m_uiRtpPacketsReceivedDuringLastInterval;

  boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
  if (pSubflowHeader)
  {
    uint16_t uiFlowId = pSubflowHeader->getFlowId();
    uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
#ifdef DEBUG_MPRTP
    VLOG(2) << "Received packet with SN " << rtpPacket.getSequenceNumber()
            << " FlowId: " << uiFlowId
            << " FSSN: " << uiFlowSpecificSequenceNumber;
#endif
    auto it = m_mFlowSpecificMap.find(uiFlowId);
    if (it == m_mFlowSpecificMap.end())
    {
      m_mFlowSpecificMap[uiFlowId] = createFlowSpecificMemberEntry(uiFlowId);
      VLOG(2) << "First packet for member " << hex(m_uiSSRC) << " for flow " << uiFlowId;
      // first packet for flow
      m_mFlowSpecificMap[uiFlowId]->initSequence(uiFlowSpecificSequenceNumber);
    }
    else if (!m_mFlowSpecificMap[uiFlowId]->hasBeenIntialisedWithSN())
    {
      // This can occur if RTCP is received for a flow before RTP
      VLOG(2) << "First packet for member " << hex(m_uiSSRC) << " for flow " << uiFlowId;
      // first packet for flow
      m_mFlowSpecificMap[uiFlowId]->initSequence(uiFlowSpecificSequenceNumber);
    }

    // process RTP
    m_mFlowSpecificMap[uiFlowId]->processRtpPacket(uiFlowSpecificSequenceNumber, rtpPacket.getRtpTimestamp(), rtpPacket.getRtpArrivalTimestamp());
  }
  else
  {
    LOG(WARNING) << "MPRTP packet has no header extension!";
  }

  /*
  // search for MPRTP header extension
  rfc5285::RtpHeaderExtension::ptr pHeaderExtension = rtpPacket.getHeader().getHeaderExtension();
  if (pHeaderExtension)
  {
    // Potentially expensive???: create a work around if this proves to be a problem
    MpRtpSubflowRtpHeader* pSubflowHeader = dynamic_cast<MpRtpSubflowRtpHeader*>(pHeaderExtension.get());
    if (pSubflowHeader)
    {
      uint16_t uiFlowId = pSubflowHeader->getFlowId();
      uint16_t uiFlowSpecificSequenceNumber = pSubflowHeader->getFlowSpecificSequenceNumber();
#ifdef DEBUG_MPRTP
      VLOG(2) << "Received packet with SN " << rtpPacket.getSequenceNumber()
              << " FlowId: " << uiFlowId
              << " FSSN: " << uiFlowSpecificSequenceNumber;
#endif
      auto it = m_mFlowSpecificMap.find(uiFlowId);
      if (it == m_mFlowSpecificMap.end())
      {
        m_mFlowSpecificMap[uiFlowId] = createFlowSpecificMemberEntry(uiFlowId);
        VLOG(2) << "First packet for member " << hex(m_uiSSRC) << " for flow " << uiFlowId;
        // first packet for flow
        m_mFlowSpecificMap[uiFlowId]->initSequence(uiFlowSpecificSequenceNumber);
      }
      else if (!m_mFlowSpecificMap[uiFlowId]->hasBeenIntialisedWithSN())
      {
        // This can occur if RTCP is received for a flow before RTP
        VLOG(2) << "First packet for member " << hex(m_uiSSRC) << " for flow " << uiFlowId;
        // first packet for flow
        m_mFlowSpecificMap[uiFlowId]->initSequence(uiFlowSpecificSequenceNumber);
      }

      // process RTP
      m_mFlowSpecificMap[uiFlowId]->processRtpPacket(uiFlowSpecificSequenceNumber, rtpPacket.getRtpTimestamp(), rtpPacket.getRtpArrivalTimestamp());
    }
  }
  */
}

}
}
