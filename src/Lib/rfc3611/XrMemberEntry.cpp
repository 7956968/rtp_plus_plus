#include "CorePch.h"
#include <rtp++/rfc3611/XrMemberEntry.h>
#include <cpputil/Utility.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3611/RtcpXr.h>
#include <rtp++/rfc3611/RtcpXrFactory.h>

namespace rtp_plus_plus
{
namespace rfc3611
{

XrMemberEntry::XrMemberEntry()
  :m_uiXrLrr(0),
  m_uiXrDlrr(0),
  m_uiXrLrr_sender(0),
  m_uiXrDlrr_sender(0),
  m_uiXrNtpArrival(0),
  m_dXrRtt(0.0)
{

}

XrMemberEntry::XrMemberEntry(uint32_t uiSSRC)
  : MemberEntry(uiSSRC),
  m_uiXrLrr(0),
  m_uiXrDlrr(0),
  m_uiXrLrr_sender(0),
  m_uiXrDlrr_sender(0),
  m_uiXrNtpArrival(0),
  m_dXrRtt(0.0)
{

}

void XrMemberEntry::onXrReceived( const rfc3611::RtcpXr& rXr )
{
  onReceiveRtcpPacket();

  // we are currently only handling
  // 1. Receiver Reference Time Report Block
  // 2. DLRR Report Block
  std::vector<rfc3611::RtcpXrReportBlock> vXrBlocks = rXr.getXrReportBlocks();

  for (rfc3611::RtcpXrReportBlock& xrBlock: vXrBlocks)
  {
    switch (xrBlock.getBlockType())
    {
      case XR_RECEIVER_REFERENCE_TIME_BLOCK:
      {
        boost::optional<ReceiverReferenceTimeReportBlock> rrtBlock = rfc3611::RtcpXrFactory::parseReceiverReferenceTimeReportBlock(xrBlock);
        if (!rrtBlock)
        {
          LOG(WARNING) << "Failed to parse Receiver Reference Time Report Block";
          break;
        }
        m_tXrLrr = boost::posix_time::microsec_clock::universal_time();
        // update time last SR received
        uint32_t uiNtpMsw = rrtBlock->NtpTimestampMsw;
        uint32_t uiNtpLsw = rrtBlock->NtpTimestampLsw;
        m_uiXrLrr = ((uiNtpMsw & 0xFFFF) << 16);
        m_uiXrLrr |= ((uiNtpLsw >> 16) &  0xFFFF);

#ifdef DEBUG_RTCP
        DLOG(INFO) << "[" << this << "]" << "RTT_DEBUG SSRC: RRT received at " << hex(uiNtpMsw) << "." << hex(uiNtpLsw)
                   << " from " << hex(m_uiSSRC) << " LSR: " << hex(m_uiXrLrr) << " " << m_tXrLrr;
#endif

        break;
      }
      case XR_DLRR_BLOCK:
      {
        boost::optional<std::vector<rfc3611::DlrrBlock> > blocks = rfc3611::RtcpXrFactory::parseDlrrReportBlock(xrBlock);
        if (!blocks)
        {
          LOG(WARNING) << "Failed to parse DLRR Blocks";
          break;
        }

        // search for the XR block about "us"
        for (rfc3611::DlrrBlock& dlrrBlock: *blocks)
        {
          if (isRelevantToParticipant(dlrrBlock.SSRC))
          {
            // process report about local participant
            processDLRRBlock(rXr.getReporterSSRC(), rXr.getArrivalTimeNtp(), dlrrBlock);
          }
        }
        break;
      }
      default:
      {
        LOG(WARNING) << "Unsupported RTCP XR block type: " << (uint32_t)xrBlock.getBlockType();
      }
    }
  }
}

void XrMemberEntry::finaliseData()
{
  // takes place on the sender
  // this value is only calculated if an XR RTT has been received
  if (!m_tXrLrr.is_not_a_date_time())
  {
    const boost::posix_time::ptime tNow =  boost::posix_time::microsec_clock::universal_time();
    m_tXrDlrr = tNow - m_tXrLrr;
  }
  else
  {
    m_tXrDlrr = boost::posix_time::seconds(0);
  }
#ifdef DEBUG_RTCP
  DLOG(INFO) << "[" << this << "]" << "finalising XR data: LRR: " << hex(m_uiXrLrr) << " " << m_uiXrLrr;
#endif

  // calculate time elapsed in seconds
  uint32_t uiSeconds = static_cast<uint32_t>(m_tXrDlrr.total_seconds());
  uint32_t uiMicroseconds = static_cast<uint32_t>(m_tXrDlrr.fractional_seconds());

  double dDelay = (double)uiSeconds + uiMicroseconds/1000000.0;
  m_uiXrDlrr = RtpTime::convertDelaySinceLastSenderReportToDlsr(dDelay);

#ifdef DEBUG_RTCP
  DLOG(INFO) << "[" << this << "] SSRC: " << hex(m_uiSSRC)
             << " DLSR: " << hex(m_uiXrDlrr) << " = "
             << m_tXrDlrr.total_milliseconds() << "ms ("
             << dDelay <<  "s) Last RRT received: "
             << m_tXrLrr;
#endif
}

void XrMemberEntry::processDLRRBlock(const uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const rfc3611::DlrrBlock& dlrrBlock)
{
  // calculate RTT
  m_uiXrLrr_sender = dlrrBlock.LastRR;
  if (m_uiXrLrr_sender == 0) return;

  m_uiXrNtpArrival = static_cast<uint32_t>((uiNtpArrivalTs >> 16) & 0xFFFFFFFF);
  m_uiXrDlrr_sender = dlrrBlock.DLRR;
  uint32_t uiRtt = 0;
  int32_t iRtt = m_uiXrNtpArrival - m_uiXrDlrr_sender - m_uiXrLrr_sender;

  if (iRtt >= 0)
  {
    uiRtt = static_cast<uint32_t>(iRtt);
  }
  else
    uiRtt = 0;

  m_dXrRtt = uiRtt/65536.0;

  VLOG(1) << "[" << this << "]"
          << " XR Reporter SSRC: " << hex(uiReporterSSRC)
          << " Reportee: "  << hex(dlrrBlock.SSRC)
          << " Arrival: " << hex(m_uiNtpArrival)
          << " LRR: " << hex(m_uiXrLrr_sender)
          << " DLRR: " << hex(m_uiXrDlrr_sender)
          << " (" << RtpTime::convertDlsrToSeconds(dlrrBlock.DLRR) << "s)"
          << " RTT: " << hex(iRtt) << " unsigned: " << hex(uiRtt) << " ( " << m_dXrRtt << "s )";
}

}
}
