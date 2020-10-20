#include "CorePch.h"
#include <rtp++/RtpTime.h>
#include <cstdlib>
#include <cmath>
#include <cpputil/Utility.h>

// -DDEBUG_RTP_TIMESTAMP_CONVERSION
// #define DEBUG_RTP_TIMESTAMP_CONVERSION

// TODO: Refactor! Check that all conversion functions match with unit tests
namespace rtp_plus_plus
{

uint32_t RtpTime::convertToRtpTimestamp(uint32_t uiSeconds, uint32_t uiMicroseconds, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase)
{
  uint32_t uiRtpTimestamp = uiTimestampFrequency * uiSeconds;
#ifdef DEBUG_RTP_TIMESTAMP_CONVERSION
  // NB: cast to double before mult: fixed overflow
  double dVal = ((double)uiMicroseconds * uiTimestampFrequency)/1000000.0;
  uint32_t uiRtpMicroSecs = static_cast<uint32_t>(dVal + 0.5);
  uiRtpTimestamp += uiRtpMicroSecs;

  DLOG(INFO) << "Seconds: " << uiSeconds << "." << uiMicroseconds << " RTP TS: " << uiRtpTimestamp << " Contribution microsecs: " << uiRtpMicroSecs << " TSF: " << uiTimestampFrequency << " double: " << dVal;
#else
  // NB: cast to double before mult: fixed overflow
  uiRtpTimestamp += (uint32_t)((((double)uiMicroseconds * uiTimestampFrequency)/1000000.0) + 0.5);
#endif

  return uiTimestampBase + uiRtpTimestamp;
}

uint32_t RtpTime::convertToRtpTimestamp(double dPresentationTime, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase)
{
  uint32_t uiRtpTimestamp = (uint32_t)((uiTimestampFrequency * dPresentationTime) + 0.5);
  return uiTimestampBase + uiRtpTimestamp;
}

uint32_t RtpTime::convertTimeToRtpTimestamp( const boost::posix_time::ptime& tArrival, uint32_t uiTimestampFrequency, uint32_t uiTimestampBase)
{
  boost::posix_time::ptime tEpoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::time_duration diff = tArrival - tEpoch;
  uint32_t uiSeconds = static_cast<uint32_t>(diff.total_seconds());
  uint32_t uiMicroseconds = static_cast<uint32_t>(diff.fractional_seconds());

  uint32_t uiRtpTimestamp = uiTimestampFrequency * uiSeconds;
  uiRtpTimestamp += (uint32_t)((((double)uiMicroseconds * uiTimestampFrequency)/1000000.0)+0.5);

#ifdef DEBUG_RTP_TIMESTAMP_CONVERSION
  DLOG(INFO) << "Seconds: " << uiSeconds << " Microseconds: " << uiMicroseconds << "RTP TS: " << uiRtpTimestamp;
#endif

    return uiTimestampBase + uiRtpTimestamp;
}

uint32_t RtpTime::currentTimeToRtpTimestamp(uint32_t uiTimestampFrequency, uint32_t uiTimestampBase)
{
  boost::posix_time::ptime tEpoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::time_duration diff = tNow - tEpoch;
  uint32_t uiSeconds = static_cast<uint32_t>(diff.total_seconds());
  uint32_t uiMicroseconds = static_cast<uint32_t>(diff.fractional_seconds());

  uint32_t uiRtpTimestamp = uiTimestampFrequency * uiSeconds;
  uiRtpTimestamp += (uint32_t)((((double)uiMicroseconds * uiTimestampFrequency)/1000000.0)+0.5);

#ifdef DEBUG_RTP_TIMESTAMP_CONVERSION
  DLOG(INFO) << "Seconds: " << uiSeconds << " Microseconds: " << uiMicroseconds << "RTP TS: " << uiRtpTimestamp;
#endif

    return uiTimestampBase + uiRtpTimestamp;
}

void RtpTime::convertRtpToSystemTimestamp(uint32_t& uiSeconds, uint32_t& uiMicroseconds, uint32_t uiRtpTimestamp, uint32_t uiTimestampFrequency)
{
  uiSeconds = uiRtpTimestamp/uiTimestampFrequency;
  uiMicroseconds = uiRtpTimestamp%uiTimestampFrequency;
}

void RtpTime::convertRtpToSystemTimestamp(double& dTime, uint32_t uiRtpTimestamp, uint32_t uiTimestampFrequency)
{
  dTime = uiRtpTimestamp/static_cast<double>(uiTimestampFrequency);
  dTime += (uiRtpTimestamp%uiTimestampFrequency)/1000000.0;
}

void RtpTime::getTimeDifference(boost::posix_time::ptime t1, boost::posix_time::ptime t2, uint32_t& uiSeconds, uint32_t& uiMicroseconds)
{
  boost::posix_time::time_duration diff = t2 - t1;
  uiSeconds = static_cast<uint32_t>(diff.total_seconds());
  uiMicroseconds = static_cast<uint32_t>(diff.fractional_seconds());
#if 0
  DLOG(INFO) << "Time: " << t2
             << " Seconds: " << uiSeconds
             << " Microseconds: " << uiMicroseconds;
#endif
}

void RtpTime::getTimeElapsedSince(boost::gregorian::date epoch, uint32_t& uiSeconds, uint32_t& uiMicroseconds)
{
  getTimeDifference(boost::posix_time::ptime(epoch), boost::posix_time::microsec_clock::universal_time(), uiSeconds, uiMicroseconds);
}

boost::posix_time::ptime RtpTime::convertCurrentTimeToNtpTimestamp()
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  RtpTime::convertToNtpTimestamp(tNow, uiNtpMsw, uiNtpLsw);
  return convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);
}

void RtpTime::convertCurrentTimeToNtpTimestamp(uint32_t& uiNtpMsw, uint32_t& uiNtpLsw)
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  RtpTime::convertToNtpTimestamp(tNow, uiNtpMsw, uiNtpLsw);
}

void RtpTime::convertToNtpTimestamp(boost::posix_time::ptime t1, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw)
{
  uint32_t uiMicroseconds = 0;
  getTimeDifference(boost::posix_time::ptime(boost::gregorian::date(1900,1,1)), t1, uiNtpMsw, uiMicroseconds);
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uiNtpLsw = static_cast<uint32_t>(dFractionalPart + 0.5);
}

void RtpTime::convertToNtpTimestamp(boost::posix_time::ptime t1, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw, uint32_t uiClockFrequency)
{
  uint32_t uiMicroseconds = 0;
  getTimeDifference(boost::posix_time::ptime(boost::gregorian::date(1900,1,1)), t1, uiNtpMsw, uiMicroseconds);
  double dFactor = 1000000.0/uiClockFrequency;
  uint32_t uiDiv = (uint32_t)(uiMicroseconds/dFactor + 0.5);
  uiMicroseconds = static_cast<uint32_t>(uiDiv * dFactor);
  //uiMicroseconds = static_cast<uint32_t>((uiMicroseconds/(double)uiClockFrequency) + 0.5);
  //uiMicroseconds *= uiClockFrequency;
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uiNtpLsw = static_cast<uint32_t>(dFractionalPart + 0.5);
}

boost::posix_time::ptime RtpTime::convertNtpTimestampToPosixTime(uint64_t uiNtp)
{
  uint32_t uiNtpLsw = static_cast<uint32_t>(uiNtp); //& 0xFFFFFFFF;
  uint32_t uiNtpMsw = static_cast<uint32_t>(uiNtp >> 32);
  return convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);
}

boost::posix_time::ptime RtpTime::convertNtpTimestampToPosixTime(uint32_t uiNtpMsw, uint32_t uiNtpLsw)
{
  boost::posix_time::ptime tTime = boost::posix_time::ptime(boost::gregorian::date(1900,1,1));
  long hours = uiNtpMsw/3600;
  uiNtpMsw %= 3600;
  long minutes = uiNtpMsw/60;
  uiNtpMsw %= 60;
  long seconds = uiNtpMsw;
  // now convert from the 2^32 range: 10^6/2^32 = 2^26/(10^6/2^6)
  double dMicroseconds = uiNtpLsw * 15625.0/0x04000000;
  long microseconds = static_cast<long>(dMicroseconds + 0.5);
  boost::posix_time::time_duration td(hours, minutes, seconds, 0);
  td += boost::posix_time::microseconds(microseconds);
  tTime += td;
  return tTime;
}

void RtpTime::split(uint64_t uiNtp, uint32_t& uiNtpMsw, uint32_t& uiNtpLsw)
{
  uiNtpLsw = static_cast<uint32_t>(uiNtp); //& 0xFFFFFFFF;
  uiNtpMsw = static_cast<uint32_t>(uiNtp >> 32);
}

uint64_t RtpTime::join(const uint32_t uiNtpMsw, const uint32_t uiNtpLsw)
{
  return (static_cast<uint64_t>(uiNtpMsw) << 32) | (uiNtpLsw);
}

void RtpTime::getTimeElapsedSinceUnixEpoch(uint32_t& uiSeconds, uint32_t& uiMicroseconds)
{
  getTimeElapsedSince(boost::gregorian::date(1970,1,1), uiSeconds, uiMicroseconds);
}

void RtpTime::getTimeElapsedSinceNTPEpoch(uint32_t& uiSeconds, uint32_t& uiMicroseconds)
{
  getTimeElapsedSince(boost::gregorian::date(1900,1,1), uiSeconds, uiMicroseconds);
}

void RtpTime::getNTPTimeStamp(uint64_t& uiNtpTime)
{
  uint32_t uiSeconds = 0, uiMicroseconds = 0;
  getTimeElapsedSinceNTPEpoch(uiSeconds, uiMicroseconds);
  // now convert into the 2^32 range: 2^32/10^6 = 2^26/(10^6/2^6)
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uint64_t uiLsw = static_cast<uint64_t>(dFractionalPart + 0.5);
  uiNtpTime = (static_cast<uint64_t>(uiSeconds) << 32) | (uiLsw);
}

void RtpTime::getNTPTimeStamp(uint32_t& uiNtpMsw, uint32_t& uiNtpLsw)
{
  uint32_t uiSeconds = 0, uiMicroseconds = 0;
  getTimeElapsedSinceNTPEpoch(uiSeconds, uiMicroseconds);
  // now convert into the 2^32 range: 2^32/10^6 = 2^26/(10^6/2^6)
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uiNtpMsw = uiSeconds;
  uiNtpLsw = static_cast<uint32_t>(dFractionalPart + 0.5);
}

uint64_t RtpTime::getNTPTimeStamp()
{
  uint64_t uiNtpTime = 0;
  uint32_t uiSeconds = 0, uiMicroseconds = 0;
  getTimeElapsedSince(boost::gregorian::date(1900,1,1), uiSeconds, uiMicroseconds);
  // now convert into the 2^32 range: 2^32/10^6 = 2^26/(10^6/2^6)
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uint32_t uiLsw = static_cast<uint32_t>(dFractionalPart + 0.5);
  uiNtpTime = (static_cast<uint64_t>(uiSeconds) << 32) | (uiLsw);
  return uiNtpTime;
}

uint32_t RtpTime::getMiddle32bitsOfNTPTimeStamp()
{
  uint32_t uiNtpTime = 0;
  uint32_t uiSeconds = 0, uiMicroseconds = 0;
  getTimeElapsedSince(boost::gregorian::date(1900,1,1), uiSeconds, uiMicroseconds);
  // now convert into the 2^32 range: 2^32/10^6 = 2^26/(10^6/2^6)
  double dFractionalPart = (uiMicroseconds/15625.0) * 0x04000000;
  uint32_t uiLsw = static_cast<uint32_t>(dFractionalPart + 0.5);
  uiNtpTime = ((uiSeconds << 16)&0xFFFF0000) | ((uiLsw >> 16) & 0xFFFF);
  return uiNtpTime;
}

uint32_t RtpTime::convertDelaySinceLastSenderReportToDlsr(double dDelaySeconds)
{
  double dInt = 0.0, dFrac = 0.0;
  uint32_t uiDelay = static_cast<uint32_t>(dDelaySeconds) << 16;
  dFrac = modf(dDelaySeconds, &dInt) * 65536;
  uiDelay |= static_cast<uint32_t>(dFrac);
  return uiDelay;
}

double RtpTime::convertDlsrToSeconds(uint32_t uiDlsr)
{
  double dFractionalPart = ((uiDlsr >> 16) & 0xFFFF) + ((uiDlsr & 0xFFFF)/65536.0);
  return dFractionalPart;
}

bool RtpTime::presentationTimeMatch(const boost::posix_time::ptime& t1, const boost::posix_time::ptime& t2)
{
  return (t1-t2).total_microseconds() < 12;
}

boost::posix_time::ptime RtpTime::getPresentationTime(uint32_t uiRtpSyncRef, const boost::posix_time::ptime& tSyncRef,
                                                             uint32_t uiRtpTs, uint32_t uiRtpTimestampFrequency)
{
  boost::posix_time::ptime tPresentation;
  // simulate what MemberEntry does
  int timestampDiff = uiRtpTs - uiRtpSyncRef;

  // Divide this by the timestamp frequency to get real time:
  double timeDiff = timestampDiff/(double)uiRtpTimestampFrequency;

  unsigned const million = 1000000;

  if (timeDiff >= 0.0)
  {
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million + 0.5); // is there rounding missing here?
    tPresentation = tSyncRef + boost::posix_time::seconds(seconds) + boost::posix_time::microseconds(uSeconds);
  }
  else
  {
    // This could be the case if samples are not sent in presentation order
    // This also happens when sending a packet over SCTP or over MST SVC:
    // An earlier timestamped RTP packet arrives later than one timestamped with a later RTP TS.
    timeDiff = -timeDiff;
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million  + 0.5); // is there rounding missing here?
    tPresentation = tSyncRef - boost::posix_time::seconds(seconds) - boost::posix_time::microseconds(uSeconds);
  }
  return tPresentation;
}

} // rtp_plus_plus
