#pragma once
#include <cstdint>
#include <boost/date_time/posix_time/posix_time.hpp>

// -DDEBUG_RTP_TIMESTAMP_CONVERSION
// #define DEBUG_RTP_TIMESTAMP_CONVERSION

// TODO: Refactor! Check that all conversion functions match with unit tests

namespace rtp_plus_plus
{

class RtpTime
{

public:
/**
 * @fn  uint32_t :::convertToRtpTimestamp(uint32_t uiSeconds, uint32_t uiMicroseconds,
 *      uint32_t uiTimestampFrequency, uint32_t uiTimestampBase)
 *
 * @brief Converts this object to a rtp timestamp.
 *
 * @param uiSeconds             The seconds.
 * @param uiMicroseconds        The microseconds.
 * @param uiTimestampFrequency  The timestamp frequency.
 * @param uiTimestampBase       The timestamp base.
 *
 * @return  .
 */
static uint32_t convertToRtpTimestamp(uint32_t uiSeconds, uint32_t uiMicroseconds, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase);

/**
 * Converts this object to a rtp timestamp.
 *
 * \param dPresentationTime     Time of the presentation.
 * \param uiTimestampFrequency  The timestamp frequency.
 * \param uiTimestampBase       The timestamp base.
 *
 * \return  .
 */
static uint32_t convertToRtpTimestamp(double dPresentationTime, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase);
static uint32_t convertTimeToRtpTimestamp( const boost::posix_time::ptime& tArrival, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase);

static uint32_t currentTimeToRtpTimestamp(uint32_t uiTimestampFrequency, uint32_t uiTimestampBase);
/**
 * Convert rtp to system timestamp.
 *
 * \param [in,out]  uiSeconds       The seconds.
 * \param [in,out]  uiMicroseconds  The microseconds.
 * \param uiRtpTimestamp            The rtp timestamp.
 * \param uiTimestampFrequency      The timestamp frequency.
*/
static void convertRtpToSystemTimestamp(uint32_t& uiSeconds, uint32_t& uiMicroseconds, uint32_t uiRtpTimestamp, uint32_t uiTimestampFrequency);
/**
 * Convert rtp to system timestamp.
 *
 * \param [in,out]  dTime       The time.
 * \param uiRtpTimestamp        The rtp timestamp.
 * \param uiTimestampFrequency  The timestamp frequency.
 */
static void convertRtpToSystemTimestamp(double& dTime, uint32_t uiRtpTimestamp, uint32_t uiTimestampFrequency);

static void getTimeDifference(boost::posix_time::ptime t1, boost::posix_time::ptime t2, uint32_t& uiSeconds, uint32_t& uiMicroseconds);

static void getTimeElapsedSince(boost::gregorian::date epoch, uint32_t& uiSeconds, uint32_t& uiMicroseconds);

static boost::posix_time::ptime convertCurrentTimeToNtpTimestamp();
static void convertCurrentTimeToNtpTimestamp(uint32_t& uiNtpMsw, uint32_t& uiNtpLsw);

static void convertToNtpTimestamp(boost::posix_time::ptime t1, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw);

/**
 * @brief convertToNtpTimestamp This version rounds up to the frequency of the used clock
 * @param t1
 * @param uiNtpMsw
 * @param uiNtpLsw
 * @param uiClockFrequency
 */
static void convertToNtpTimestamp(boost::posix_time::ptime t1, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw, uint32_t uiClockFrequency);

static boost::posix_time::ptime convertNtpTimestampToPosixTime(uint64_t uiNtp);
static boost::posix_time::ptime convertNtpTimestampToPosixTime(uint32_t uiNtpMsw, uint32_t uiNtpLsw);
static void getNTPTimeStamp(uint64_t& uiNtpTime);
static void getNTPTimeStamp(uint32_t& uiNtpMsw, uint32_t& uiNtpLsw);

static void split(uint64_t uiNtp, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw);
static uint64_t join(const uint32_t uiNtpMsw, const uint32_t uiNtpLsw);

static void getTimeElapsedSinceUnixEpoch(uint32_t& uiSeconds, uint32_t& uiMicroseconds);
static void getTimeElapsedSinceNTPEpoch(uint32_t& uiSeconds, uint32_t& uiMicroseconds);


static uint64_t getNTPTimeStamp();
static uint32_t getMiddle32bitsOfNTPTimeStamp();

static uint32_t convertDelaySinceLastSenderReportToDlsr(double dDelaySeconds);
static double convertDlsrToSeconds(uint32_t uiDlsr);

static boost::posix_time::ptime getPresentationTime(uint32_t uiRtpSyncRef, const boost::posix_time::ptime& tSyncRef,
                                                    uint32_t uiRtpTs, uint32_t uiRtpTimestampFrequency);

/**
 * @brief presentationTimeMatch This method checks if two presentation times are close enough to be the same
 * @param t1 First presentation time
 * @param t2 Second presentation time
 * @return True if the difference between two presentation times is less than 12 microseconds, false otherwise
 */
static bool presentationTimeMatch(const boost::posix_time::ptime& t1, const boost::posix_time::ptime& t2);
};

} // rtp_plus_plus
