#pragma once
#include <cstdint>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

/**
 * @brief The RtpReferenceClock class Takes care of RTP timestamp generation
 * in RTP and RTCP streams. The class can be in wallclock mode or in media sample
 * timestamp mode. The recommended mode is wall clock since there is a single clock governing
 * the system. If however RTP timestamps are to be generated using media sample timestamps,
 * RC_MEDIA_SAMPLE_TIME can be used. In this case, the RTCP timestamps are approximated
 * based on the distance to the previously generated reference time. This code assumes that
 * the media timestamps are monotonically increasing.
 * The RTP timestamps may however be inaccurate with respect to the actual time elasped.
 *
 * Since different threads (RTP and RTCP) may call the timestamp generation method the mutex
 * has to be locked to avoid race conditions
 */
class RtpReferenceClock
{
public:
  enum Mode
  {
    RC_WALL_CLOCK_TIME,
    RC_MEDIA_SAMPLE_TIME
  };

  RtpReferenceClock(Mode eMode = RC_WALL_CLOCK_TIME);

  /**
   * @brief setMode Set RTP timestamping mode
   * @param eMode
   */
  void setMode(Mode eMode){ m_eMode = eMode; }

  /**
   * @brief getNextRtpTimestamp gets the next RTP timestamp according to the configured mode
   * @param uiRtpTimestampFrequency RTP timestamp frequency
   * @param uiRtpTimestampBase RTP timestamp base
   * @param dMediaSampleTime Media Sample time. Only applies in mode RC_MEDIA_SAMPLE_TIME. In
   *        RC_WALL_CLOCK_TIME mode this parameter is ignored.
   * @param uiNtpTimestampMsw The MSW of the NTP timestamp that the RTP timestamp was calculated from
   * @param uiNtpTimestampLsw The LSW of the NTP timestamp that the RTP timestamp was calculated from
   * @return The RTP timestamp
   */
  uint32_t getRtpTimestamp(uint32_t uiRtpTimestampFrequency,
                           uint32_t uiRtpTimestampBase,
                           double dMediaSampleTime,
                           uint32_t& uiNtpTimestampMsw,
                           uint32_t& uiNtpTimestampLsw
                           );

    /**
   * @brief getRtpAndNtpTimestamps gets the next RTP timestamp according to the configured mode
   * @param uiRtpTimestampFrequency RTP timestamp frequency
   * @param uiRtpTimestampBase RTP timestamp base
   * @param uiNtpTimestampMsw The most significant word of the NTP timestamp associated with the RTP timestamp
   * @param uiNtpTimestampLsw The least significant word of the NTP timestamp associated with the RTP timestamp
   * @return The RTP timestamp
   */
  uint32_t getRtpAndNtpTimestamps(uint32_t uiRtpTimestampFrequency,
                                  uint32_t uiRtpTimestampBase,
                                  uint32_t& uiNtpTimestampMsw,
                                  uint32_t& uiNtpTimestampLsw
                                  );

  /**
    * @brief returns the offset that is being used for each timestamp
    */
  uint32_t getOffset() const { return m_uiRtcpOffset; }

private:

  Mode m_eMode;

  // Member variables for RC_MEDIA_SAMPLE_TIME mode:
  // reference point used in mode RC_MEDIA_SAMPLE_TIME
  // time the last packet was sent: in mode RC_MEDIA_SAMPLE_TIME we
  // try to align the RTCP with the media sample reference clock
  boost::posix_time::ptime m_tReferenceBase;
  // To measure time elapsed since last RTP
  uint32_t m_uiLastRtpTimestamp;
  // Valid when RTCP is sent before RTP
  uint32_t m_uiRtcpOffset;
  // flag to offset RTP if necessary
  bool m_bOffsetRtp;
  // Lock for mt-threaded use
  boost::mutex m_lock;
};

} // rtp_plus_plus
