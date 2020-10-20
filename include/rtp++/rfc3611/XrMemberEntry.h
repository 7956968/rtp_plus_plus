#pragma once
#include <rtp++/rfc3550/MemberEntry.h>
#include <rtp++/rfc3611/RtcpXr.h>

namespace rtp_plus_plus
{
namespace rfc3611
{

/**
  * RTCP XR Member Entry
  */
class XrMemberEntry : public rfc3550::MemberEntry
{
public:

  XrMemberEntry();
  XrMemberEntry(uint32_t uiSSRC);

  uint32_t getLastRr() const { return m_uiXrLrr; }
  uint32_t getDelaySinceLastReceiverReport() const { return m_uiXrDlrr; }

  void onXrReceived( const rfc3611::RtcpXr& rXr );
  void finaliseData();
  void processDLRRBlock(const uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const rfc3611::DlrrBlock& dlrrBlock);

private:

  /// Receiver stores the last RR extracted from the XR
  uint32_t m_uiXrLrr;
  /// Receiver stores the last DLRR that was extracted from the RR
  uint32_t m_uiXrDlrr;
  /// Sender stores middle 32 bits of last RRT received
  uint32_t m_uiXrLrr_sender;
  /// Sender stores the last DLRR calculated
  uint32_t m_uiXrDlrr_sender;
  /// Receiver: stores the middle 32 bits of the NTP timestamp of when the last DLRR block was received
  uint32_t m_uiXrNtpArrival;
  /// Receiver: stores the last RTT calculated
  double m_dXrRtt;
  /// This member stores the time at which the LRR was received
  boost::posix_time::ptime m_tXrLrr;
  boost::posix_time::time_duration m_tXrDlrr;
};

}
}
