#include "CorePch.h"
#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

RtpReferenceClock::RtpReferenceClock(Mode eMode)
  :m_eMode(eMode),
  m_uiLastRtpTimestamp(0),
  m_uiRtcpOffset(0),
  m_bOffsetRtp(false)
{

}

uint32_t RtpReferenceClock::getRtpTimestamp(uint32_t uiRtpTimestampFrequency,
                                            uint32_t uiRtpTimestampBase,
                                            double dMediaSampleTime,
                                            uint32_t& uiNtpTimestampMsw,
                                            uint32_t& uiNtpTimestampLsw
                                            )
{
  switch (m_eMode)
  {
  case RC_MEDIA_SAMPLE_TIME:
    {
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
      boost::mutex::scoped_lock l( m_lock );
      // if we received RTCP previously, we have to offset RTP
      if (m_bOffsetRtp)
      {
        // check how much time has elapsed since the RTCP
        uint64_t uiTotalMicroseconds = (tNow - m_tReferenceBase).total_microseconds();
        uint32_t uiSecondsElapsed = (uint32_t)(uiTotalMicroseconds/1000000);
        uint32_t uiMicroSecondsElapsed = (uint32_t)(uiTotalMicroseconds%1000000);
        m_uiRtcpOffset = RtpTime::convertToRtpTimestamp(
          uiSecondsElapsed,
          uiMicroSecondsElapsed,
          uiRtpTimestampFrequency,
          0 // use 0 as base since we are calculating the difference
          );
        // only calc first time
        m_bOffsetRtp = false;
        VLOG(5) << "RTCP offset set to " << m_uiRtcpOffset << " (" << uiTotalMicroseconds << "Us)";
      }

      // HACK: we only store the base RTP timestamp without the session specific offset
      // This gives us a constant reference to the media time
      // NOTE: this code won't work for B-frames
      uint32_t uiRtpTimestamp = RtpTime::convertToRtpTimestamp(dMediaSampleTime,
        uiRtpTimestampFrequency,
        0
        );

      // if it's a new timestamp, update the references
      if (uiRtpTimestamp != m_uiLastRtpTimestamp || m_tReferenceBase.is_not_a_date_time())
      {
        m_uiLastRtpTimestamp = uiRtpTimestamp;
        m_tReferenceBase = tNow;
      }

      // add RTCP offset: this can be zero if RTP was sent first
      uint32_t uiRtpTs = m_uiLastRtpTimestamp + uiRtpTimestampBase + m_uiRtcpOffset;

      // TODO: for now we won't calculate this if mode == RC_MEDIA_SAMPLE_TIME, add it if needed
      uiNtpTimestampMsw = 0;
      uiNtpTimestampLsw = 0;

      return uiRtpTs;
      break;
    }
  case RC_WALL_CLOCK_TIME:
  default:
    {
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
      // TEST: RG
      uint32_t uiMsw2 = 0, uiLsw2 = 0;
      RtpTime::convertToNtpTimestamp(tNow, uiMsw2, uiLsw2, 90000);
      boost::posix_time::ptime tNowRounded = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);

      uiNtpTimestampMsw = uiMsw2;
      uiNtpTimestampLsw = uiLsw2;
      return RtpTime::convertTimeToRtpTimestamp(tNowRounded,
        uiRtpTimestampFrequency,
        uiRtpTimestampBase);
      break;
    }
  }
}

uint32_t RtpReferenceClock::getRtpAndNtpTimestamps(uint32_t uiRtpTimestampFrequency,
                                                   uint32_t uiRtpTimestampBase,
                                                   uint32_t& uiNtpTimestampMsw,
                                                   uint32_t& uiNtpTimestampLsw
                                                   )
{
  switch (m_eMode)
  {
  case RC_MEDIA_SAMPLE_TIME:
    {
      boost::mutex::scoped_lock l( m_lock );
      if (m_tReferenceBase.is_not_a_date_time())
      {
        // first packet sent: store RTCP as reference time
        m_tReferenceBase = boost::posix_time::microsec_clock::universal_time();
        // set flag to calc offsets when RTP is sent
        m_bOffsetRtp = true;
        // use 0.0 as the TS: RTP will be offset against the difference in time later...
        m_uiLastRtpTimestamp = RtpTime::convertToRtpTimestamp(0.0,
          uiRtpTimestampFrequency,
          0
          );
        RtpTime::convertToNtpTimestamp(m_tReferenceBase, uiNtpTimestampMsw, uiNtpTimestampLsw);

        uint32_t uiRtpTs = m_uiLastRtpTimestamp + uiRtpTimestampBase + m_uiRtcpOffset;
        return uiRtpTs;
      }
      else
      {
        // RTP/RTCP has been sent previously
        boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
        uint64_t uiTotalMicroseconds = (tNow - m_tReferenceBase).total_microseconds();
        uint32_t uiSecondsElapsed = (uint32_t)(uiTotalMicroseconds/1000000);
        uint32_t uiMicroSecondsElapsed = (uint32_t)(uiTotalMicroseconds%1000000);
        uint32_t uiRtpTimeElapsed = RtpTime::convertToRtpTimestamp(
          uiSecondsElapsed,
          uiMicroSecondsElapsed,
          uiRtpTimestampFrequency,
          0 // use 0 as base since we are calculating the difference
          );

        // update references
        m_uiLastRtpTimestamp+=uiRtpTimeElapsed;
        m_tReferenceBase = tNow;
        RtpTime::convertToNtpTimestamp(m_tReferenceBase, uiNtpTimestampMsw, uiNtpTimestampLsw);
        uint32_t uiRtpTs = m_uiLastRtpTimestamp + uiRtpTimestampBase + m_uiRtcpOffset;
        return uiRtpTs;
#if 0
        VLOG(1) << "Now: " << tNow << " Elasped since last RTP: " << uiSecondsElapsed << "." << uiMicroSecondsElapsed << " = " << uiRtpTimeElapsed << "(RTP) Last RTP: " << m_uiRtpTsLastRtpPacketSent << " Updated RTP TS:" << uiRtpTimestamp;
#endif
      }
    }
  case RC_WALL_CLOCK_TIME:
  default:
    {
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
      // TEST: RG
      uint32_t uiMsw2 = 0, uiLsw2 = 0;
      RtpTime::convertToNtpTimestamp(tNow, uiMsw2, uiLsw2, 90000);
      boost::posix_time::ptime tNowRounded = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);
      uint32_t uiRtpTs = RtpTime::convertTimeToRtpTimestamp(tNowRounded, uiRtpTimestampFrequency, uiRtpTimestampBase);
      // get associated NTP timestamp
      RtpTime::convertToNtpTimestamp(tNow, uiNtpTimestampMsw, uiNtpTimestampLsw);
      return uiRtpTs;
      break;
    }
  }
}

} // rtp_plus_plus
