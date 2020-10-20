#include "CorePch.h"
#include <rtp++/mprtp/FlowSpecificMemberEntry.h>
#include <cpputil/Utility.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace mprtp
{

FlowSpecificMemberEntry::FlowSpecificMemberEntry(uint32_t uiSSRC, uint16_t uiFlowId)
  :rfc3611::XrMemberEntry(uiSSRC),
  m_uiFlowId(uiFlowId)
{
  LOG(INFO) << "Creating flow specific member entry for flow ID " << uiFlowId;
}

FlowSpecificMemberEntry::~FlowSpecificMemberEntry()
{
  VLOG(5) << "Flow id: " << m_uiFlowId << " base: " << base_seq << " cycles: " << cycles << " max: " << max_seq << " received: " << received;
}

void FlowSpecificMemberEntry::init(const RtpPacket& rtpPacket)
{
  // do nothing: the MpRtpMemberEntry class initialises the flow specific
  // sequence number space
}

void FlowSpecificMemberEntry::doReceiveRtpPacket(const RtpPacket& packet)
{
  // do nothing: the MpRtpMemberEntry class processes the packet
  // prevent base class from processing MPRTP packet as normal
  // RTP packet by overriding this method
}

void FlowSpecificMemberEntry::onRrReceived( const rfc3550::RtcpRr& rRr )
{
#ifdef DEBUG_RTCP
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  getNTPTimeStamp(uiNtpMsw, uiNtpLsw);
  VLOG(2) << "RR received at " << hex(uiNtpMsw) << "." << hex(uiNtpLsw) << " " << convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw)
          << " from SSRC " << hex(rRr.getReporterSSRC())
          << " NTP arrival TS: " << convertNtpTimestampToPosixTime(rRr.getArrivalTimeNtp());
#endif

  // update jitter and RTT calculations
  std::vector<rfc3550::RtcpRrReportBlock> vReports = rRr.getReceiverReportBlocks();
  uint32_t uiReporterSSRC = rRr.getReporterSSRC();
  uint64_t uiNtpArrivalTs = rRr.getArrivalTimeNtp();
  std::for_each(vReports.begin(), vReports.end(), [this, uiReporterSSRC, uiNtpArrivalTs](const rfc3550::RtcpRrReportBlock& rrBlock)
  {
    // only update local info if the block applies to the local SSRC!
    if (isRelevantToParticipant(rrBlock.getReporteeSSRC()))
    {
      // calculate RTT
      m_uiLsrRR = rrBlock.getLastSr();
#ifdef DEBUG_RTCP
      DLOG(INFO) << "Updated LSR 1 to " << hex(m_uiLsrRR) << " arrival time: " << convertNtpTimestampToPosixTime(uiNtpArrivalTs);
#endif

      if (m_uiLsrRR == 0) return;

      m_uiNtpArrival = static_cast<uint32_t>((uiNtpArrivalTs >> 16) & 0xFFFFFFFF);
      m_uiDlsrRR = rrBlock.getDelaySinceLastSr();
      uint32_t uiRtt = 0;
      int32_t iRtt = m_uiNtpArrival - m_uiDlsrRR - m_uiLsrRR;
#ifdef DEBUG_RTCP
      DLOG(INFO) << "m_uiNtpArrival: " << hex(m_uiNtpArrival)
                 << " m_uiDlsrRR:" << hex(m_uiDlsrRR)
                 << " m_uiLsrRR:" << hex(m_uiLsrRR)
                 << " iRtt:" << iRtt;
#endif
      if (iRtt >= 0)
      {
        uiRtt = static_cast<uint32_t>(iRtt);
      }
      else
        uiRtt = 0;

      m_uiJitter = rrBlock.getInterarrivalJitter();
      m_uiLost = rrBlock.getCumulativeNumberOfPacketsLost();
      m_uiLossFraction = rrBlock.getFractionLost();
      m_dRtt = uiRtt/65536.0;

      VLOG(1) << "[" << this << "]"
              << " Flow Id: " << m_uiFlowId
              << " RR Reporter SSRC: " << hex(uiReporterSSRC)
              << " Reportee: "  << hex(rrBlock.getReporteeSSRC())
              << " Arrival: " << hex(m_uiNtpArrival)
              << " LSR: " << hex(m_uiLsrRR)
              << " DLSR: " << hex(m_uiDlsrRR)
              << " (" << RtpTime::convertDlsrToSeconds(rrBlock.getDelaySinceLastSr()) << "s)"
              << " RTT: " << hex(iRtt) << " unsigned: " << hex(uiRtt) << " ( " << m_dRtt << "s )"
              << " Jitter: " << hex(rrBlock.getInterarrivalJitter())
              << " Lost: " << hex(rrBlock.getCumulativeNumberOfPacketsLost())
              << " Frac: " << hex(rrBlock.getFractionLost());

      MemberUpdate update(m_uiSSRC, m_uiFlowId, m_dRtt, m_uiJitter, m_uiLost, m_uiLossFraction, rrBlock.getExtendedHighestSNReceived());

      if (m_onUpdate)
      {
        m_onUpdate(update);
      }

    }
  });

}

} // mprtp
} // rtp_plus_plus
