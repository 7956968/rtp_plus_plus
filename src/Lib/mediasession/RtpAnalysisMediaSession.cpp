#include "CorePch.h"
#include <rtp++/mediasession/RtpAnalysisMediaSession.h>
#include <cpputil/Utility.h>
#include <rtp++/RtpPacket.h>

namespace rtp_plus_plus
{

RtpSessionAnalysis::RtpSessionAnalysis()
  :m_uiClockFrequency(0),
    m_uiPrevRtpTs(0)
{

}

void RtpSessionAnalysis::addInfo(const RtpPacket& rtpPacket, const boost::posix_time::ptime& tPresentation)
{
  if (m_tFirst.is_not_a_date_time())
  {
    m_tFirst = rtpPacket.getArrivalTime();
    m_tLast = m_tFirst;
    m_tPreviousArrival = m_tFirst;
    m_tLastPresentation = tPresentation;
    m_uiPrevRtpTs = rtpPacket.getRtpTimestamp();
  }
  else
  {
    // calc difference to previous arrival time
    // only use RTP packets with new timestamps
    if (rtpPacket.getRtpTimestamp() != m_uiPrevRtpTs)
    {
      uint64_t uiMsElapsedSincePrevious = (rtpPacket.getArrivalTime() - m_tLast).total_milliseconds();
      uint64_t uiMsElapsedSincePreviousPts = (tPresentation - m_tLastPresentation).total_milliseconds();
      uint32_t uiRtpElapsed = rtpPacket.getRtpTimestamp() - m_uiPrevRtpTs;

      // convert to time
      double dElapsed = (double)uiRtpElapsed/m_uiClockFrequency;
      uint32_t uiElapsedMs = (uint32_t)dElapsed;
      const int million = 1000000;
      uiElapsedMs += static_cast<uint32_t>(((dElapsed - uiElapsedMs)*million) + 0.5);
      VLOG(2) << "RTP info - SSRC: " << hex(rtpPacket.getSSRC())
              << " SN: " << rtpPacket.getSequenceNumber()
              << " TS: " << rtpPacket.getRtpTimestamp()
              << " PTS: " << tPresentation
              << " ARRIVAL diff: " << uiMsElapsedSincePrevious
              << " ms PTS Diff: " << uiMsElapsedSincePreviousPts
              << " ms RTP diff: " << uiElapsedMs
              << " Size: " << rtpPacket.getSize();

      m_tLast = rtpPacket.getArrivalTime();
      m_tLastPresentation = tPresentation;
      m_uiPrevRtpTs = rtpPacket.getRtpTimestamp();
    }
    else
    {
      uint64_t uiMsElapsedSincePrevious = (rtpPacket.getArrivalTime() - m_tPreviousArrival).total_milliseconds();
      VLOG(2) << "RTP info - SSRC: " << hex(rtpPacket.getSSRC())
              << " SN: " << rtpPacket.getSequenceNumber()
              << " TS: " << rtpPacket.getRtpTimestamp()
              << " PTS: " << tPresentation
              << " ARRIVAL diff: " << uiMsElapsedSincePrevious
              << " ms PTS Diff: 0 ms RTP diff: 0 Size: " << rtpPacket.getSize();
    }
  }
  m_tPreviousArrival = rtpPacket.getArrivalTime();
}

RtpAnalysisMediaSession::RtpAnalysisMediaSession()
{

}

void RtpAnalysisMediaSession::handleIncomingAudio(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                         bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation)
{
  if (!bRtcpSynchronised)
  {
    LOG(INFO) << "Unable to analyse incoming audio until RTCP sync occurs.";
    return;
  }

  m_audioAnalysis.addInfo(rtpPacket, tPresentation);
}

void RtpAnalysisMediaSession::handleIncomingVideo(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                         bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation)
{
  if (!bRtcpSynchronised)
  {
    LOG(INFO) << "Unable to analyse incoming video until RTCP sync occurs.";
    return;
  }

  m_videoAnalysis.addInfo(rtpPacket, tPresentation);
}

}
