#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>
#include <rtp++/mediasession/SimpleMediaSession.h>

namespace rtp_plus_plus
{

// fwd
class RtpPacket;
class EndPoint;

class RtpSessionAnalysis
{
public:
  RtpSessionAnalysis();
  void setClockFrequency(uint32_t uiClockFrequency) { m_uiClockFrequency = uiClockFrequency; }

  void addInfo(const RtpPacket& rtpPacket, const boost::posix_time::ptime& tPresentation);

private:
  uint32_t m_uiClockFrequency;
  // Arrival of first packet
  boost::posix_time::ptime m_tFirst;
  // Arrival of previous packet with different TS
  boost::posix_time::ptime m_tLast;
  // PTS of previous packet with different TS
  boost::posix_time::ptime m_tLastPresentation;
  // RTP TS of previous packet with different TS
  uint32_t m_uiPrevRtpTs;
  // Arrival of previous packet irrespective of RTP TS
  boost::posix_time::ptime m_tPreviousArrival;
};

class RtpAnalysisMediaSession : public SimpleMediaSession
{
public:
  RtpAnalysisMediaSession();
  void setAudioClockFrequency(uint32_t uiClockFrequency) { m_audioAnalysis.setClockFrequency(uiClockFrequency); }
  void setVideoClockFrequency(uint32_t uiClockFrequency) { m_videoAnalysis.setClockFrequency(uiClockFrequency); }

  void handleIncomingAudio(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                           bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation);

  void handleIncomingVideo(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                           bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation);

private:
  RtpSessionAnalysis m_videoAnalysis;
  RtpSessionAnalysis m_audioAnalysis;
};

}
