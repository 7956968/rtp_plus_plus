#pragma once

#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpSessionState.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(RtpTimeTest)
BOOST_AUTO_TEST_CASE(test_RttCalculation)
{
  // Example taken from Perkins, pg 105
  uint32_t arrivaltime = 0xb7108000;
  uint32_t dlsr = 0x00054000;
  uint32_t lsr = 0xb7052000;
  uint32_t rtt = arrivaltime - dlsr -lsr;
  BOOST_CHECK_EQUAL(rtt, 0x00062000);

  arrivaltime = 0xee520005;
  dlsr = 0x0001d617;
  lsr = 0xee502570;
  rtt = arrivaltime - dlsr -lsr;
  BOOST_CHECK_EQUAL(rtt, 0x0000047e);
}

BOOST_AUTO_TEST_CASE(test_RtpTimestamps)
{
  // test RTP timestamp increments
  uint32_t uiSeconds = 0, uiMicroseconds = 0, uiTimestampFrequency = 8000, uiTimestampBase = 0;
  uint32_t uiRtpTimestamp = RtpTime::convertToRtpTimestamp(uiSeconds, uiMicroseconds, uiTimestampFrequency, uiTimestampBase);
  BOOST_CHECK_EQUAL(uiRtpTimestamp, 0);

  uiMicroseconds += 20000;
  uiRtpTimestamp = RtpTime::convertToRtpTimestamp(uiSeconds, uiMicroseconds, uiTimestampFrequency, uiTimestampBase);
  BOOST_CHECK_EQUAL(uiRtpTimestamp, 160);

  uiSeconds += 1;
  uiRtpTimestamp = RtpTime::convertToRtpTimestamp(uiSeconds, uiMicroseconds, uiTimestampFrequency, uiTimestampBase);
  BOOST_CHECK_EQUAL(uiRtpTimestamp, 8160);
}

/**
 * @brief test_testTimerGranularity2 This method looks for approximate matches
 */
BOOST_AUTO_TEST_CASE(test_testTimerGranularity)
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiRtp = RtpTime::convertTimeToRtpTimestamp(tNow, 90000, 0);
  uint32_t uiMsw = 0, uiLsw = 0;
  RtpTime::convertToNtpTimestamp(tNow, uiMsw, uiLsw);

#ifdef _WIN32
Sleep(1000);
#else
  sleep(1);
#endif

  boost::posix_time::ptime tLater = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiRtpLater = RtpTime::convertTimeToRtpTimestamp(tLater, 90000, 0);
  uint32_t uiMsw2 = 0, uiLsw2 = 0;
  RtpTime::convertToNtpTimestamp(tLater, uiMsw2, uiLsw2);

  uint64_t uiTotalMicroseconds = (tLater - tNow).total_microseconds();

  // check precision lost by NTP
  boost::posix_time::ptime tFirst = RtpTime::convertNtpTimestampToPosixTime(uiMsw, uiLsw);
  boost::posix_time::ptime tSecond = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);
  uint64_t uiTotalMicrosecondsNtp = (tLater - tNow).total_microseconds();


  int timestampDiff = uiRtpLater - uiRtp;

  double timeDiff = timestampDiff/(double)90000;

  unsigned const million = 1000000;

  if (timeDiff >= 0.0)
  {
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million);
    uint64_t uiTotal = seconds * million + uSeconds;
    int iDiff = std::abs<int64_t>(((int64_t)uiTotalMicroseconds - uiTotal));
    BOOST_CHECK_EQUAL( iDiff < million/90000.0, true);
  }
  else
  {
    // This could be the case if samples are not sent in presentation order
    timeDiff = -timeDiff;
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million);
    uint64_t uiTotal = seconds * million + uSeconds;
    int iDiff = std::abs<int64_t>(((int64_t)uiTotalMicroseconds - uiTotal));
    BOOST_CHECK_EQUAL( iDiff < million/90000.0, true);
  }

}

/**
 * @brief test_testTimerGranularity2 This method looks for exact matches
 */
BOOST_AUTO_TEST_CASE(test_testTimerGranularity2)
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiRtp = RtpTime::convertTimeToRtpTimestamp(tNow, 90000, 0);
  uint32_t uiMsw = 0, uiLsw = 0;
  RtpTime::convertToNtpTimestamp(tNow, uiMsw, uiLsw, 90000);
  boost::posix_time::ptime tNowConverted = RtpTime::convertNtpTimestampToPosixTime(uiMsw, uiLsw);
  // Note: NTP TS has less resolution than boost::posix_time::microsec_clock::universal_time()
  // Hence check will always fail
  // BOOST_CHECK_EQUAL( tNow, tNowConverted);

#ifdef _WIN32
Sleep(1000);
#else
  sleep(1);
#endif

  boost::posix_time::ptime tLater = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiRtpLater = RtpTime::convertTimeToRtpTimestamp(tLater, 90000, 0);
  uint32_t uiMsw2 = 0, uiLsw2 = 0;
  RtpTime::convertToNtpTimestamp(tLater, uiMsw2, uiLsw2, 90000);
  boost::posix_time::ptime tLaterConverted = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);
  // Note: NTP TS has less resolution than boost::posix_time::microsec_clock::universal_time()
  // Hence check will always fail
  // BOOST_CHECK_EQUAL( tLater, tLaterConverted);

  uint64_t uiTotalMicroseconds = (tLaterConverted - tNowConverted).total_microseconds();

  // check precision lost by NTP
  boost::posix_time::ptime tFirst = RtpTime::convertNtpTimestampToPosixTime(uiMsw, uiLsw);
  boost::posix_time::ptime tSecond = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);
  uint64_t uiTotalMicrosecondsNtp = (tSecond - tFirst).total_microseconds();

  int timestampDiff = uiRtpLater - uiRtp;
  double timeDiff = timestampDiff/(double)90000;
  unsigned const million = 1000000;

  // No rounding necessary
  uint32_t uiRtpSeconds = uiTotalMicroseconds/1000000;

  double dRtpMicroseconds = (double)uiTotalMicroseconds/million - uiRtpSeconds;
  uint32_t uiRtpMicroseconds = (uint32_t)(90000.0*dRtpMicroseconds + 0.5);
  uint32_t uiRtpTotal = (90000 * uiRtpSeconds) + uiRtpMicroseconds;

  if (timeDiff >= 0.0)
  {
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million + 0.5);
    uint64_t uiTotal = seconds * million + uSeconds;
//      BOOST_CHECK_EQUAL( uiTotalMicrosecondsNtp, uiTotal);
    int iDiff = std::abs<int64_t>(((int64_t)uiTotalMicrosecondsNtp - uiTotal));
    BOOST_CHECK_EQUAL( iDiff < million/90000.0, true);
    BOOST_CHECK_EQUAL( uiRtpTotal, timestampDiff);
  }
  else
  {
    // This could be the case if samples are not sent in presentation order
    timeDiff = -timeDiff;
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million + 0.5);
    uint64_t uiTotal = seconds * million + uSeconds;
//      BOOST_CHECK_EQUAL( uiTotalMicrosecondsNtp, uiTotal);
    int iDiff = std::abs<int64_t>(((int64_t)uiTotalMicrosecondsNtp - uiTotal));
    BOOST_CHECK_EQUAL( iDiff < million/90000.0, true);
    BOOST_CHECK_EQUAL( uiRtpTotal, timestampDiff);
  }
}

BOOST_AUTO_TEST_CASE(test_TimeConversion)
{
  uint32_t uiSecondsUnix = 0;
  uint32_t uiMicrosecondsUnix = 0;
  RtpTime::getTimeElapsedSinceUnixEpoch(uiSecondsUnix, uiMicrosecondsUnix);

  uint32_t uiSecondsNtp = 0;
  uint32_t uiMicrosecondsNtp = 0;
  RtpTime::getTimeElapsedSinceNTPEpoch(uiSecondsNtp, uiMicrosecondsNtp);

  uint32_t uiSecondsUnixToNTP = uiSecondsUnix + NTP_SECONDS_OFFSET;

  BOOST_CHECK_EQUAL(uiSecondsUnixToNTP, uiSecondsNtp);
}

static void testRtpReferenceClockImpl(uint32_t uiTsFrequency, uint32_t uiRtpTsBase1, uint32_t uiRtpTsBase2)
{
  RtpReferenceClock clock1(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);

  // intialise first reference clock
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  uint32_t uiRtcpTs1 = clock1.getRtpAndNtpTimestamps(uiTsFrequency, uiRtpTsBase1, uiNtpMsw, uiNtpLsw);
  // this timestamp serves as the reference time
  boost::posix_time::ptime tSyncRef = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);

  // let some time elapse before initialising the second reference clock
  // generate vector of start times
  std::vector<double> vStartTimes;
  double dStartTime = 0.0;
  for (size_t i = 0; i < 300; ++i)
  {
    vStartTimes.push_back(dStartTime);
    dStartTime += 1.0/25.0;
  }

  // now calculate timestamps using reference clocks
  for (double dStartTime : vStartTimes)
  {
    uint32_t uiDummyNtpMsw = 0;
    uint32_t uiDummyNtpLsw = 0;
    uint32_t uiRtpTs1 = clock1.getRtpTimestamp(uiTsFrequency, uiRtpTsBase1, dStartTime, uiDummyNtpMsw, uiDummyNtpLsw);
    uint32_t uiRtpTs2 = clock1.getRtpTimestamp(uiTsFrequency, uiRtpTsBase2, dStartTime, uiDummyNtpMsw, uiDummyNtpLsw);
    // at this point the offset has been set
    uint32_t uiRtcpOffset = clock1.getOffset();

    uint32_t uiTs2 = uiRtpTs2 - uiRtpTsBase2;
    uint32_t uiTs1 = uiRtpTs1 - uiRtpTsBase1;
    BOOST_CHECK_EQUAL(uiTs1, uiTs2);
  }
}

/**
 * @brief testRtpReferenceClockImpl3 In this unit test we simulate the scenario where a video frame is fragmented over multiple
 * RTP packets with the same RTP timestamp and an RTCP SR is received in the middle causing the reference points to be updated.
 * The test should check that the RTP packets before and after the SR have the same presentation time.
 * @param uiTsFrequency
 * @param uiRtpTsBase1
 * @param uiRtpTsBase2
 */
BOOST_AUTO_TEST_CASE(testRtpReferenceClockImpl3)
{
  RtpReferenceClock clock1(RtpReferenceClock::RC_WALL_CLOCK_TIME);
  uint32_t uiTsFrequency = 90000;
  RtpSessionState sessionState1(96);

  // intialise first reference clock
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  uint32_t uiRtpSyncRef = clock1.getRtpAndNtpTimestamps(uiTsFrequency, sessionState1.getRtpTimestampBase(), uiNtpMsw, uiNtpLsw);
  // this timestamp serves as the reference time
  boost::posix_time::ptime tSyncRef = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);

#ifdef _WIN32
Sleep(1000);
#else
  sleep(1);
#endif

  // simulate what ref clock class does
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  uint32_t uiRtpTs = RtpTime::convertTimeToRtpTimestamp(tNow, uiTsFrequency, sessionState1.getRtpTimestampBase());

  // let some time elapse before initialising the second reference clock
#ifdef _WIN32
Sleep(1000);
#else
  sleep(1);
#endif
  // intialise second reference clock
  uint32_t uiNtpMsw2 = 0, uiNtpLsw2 = 0;
  uint32_t uiRtpSyncRef2 = clock1.getRtpAndNtpTimestamps(uiTsFrequency, sessionState1.getRtpTimestampBase(), uiNtpMsw2, uiNtpLsw2);
  // this timestamp serves as the reference time
  boost::posix_time::ptime tSyncRef2 = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw2, uiNtpLsw2);

  // calculate presentation timestamps before second SR is received
  boost::posix_time::ptime t1 = RtpTime::getPresentationTime(uiRtpSyncRef, tSyncRef, uiRtpTs, 90000);

  boost::posix_time::ptime t2 = RtpTime::getPresentationTime(uiRtpSyncRef2, tSyncRef2, uiRtpTs, 90000);

  BOOST_CHECK_EQUAL(RtpTime::presentationTimeMatch(t1, t2), true);
}

static void testRtpReferenceClockImpl2(uint32_t uiTsFrequency, uint32_t uiRtpTsBase1, uint32_t uiRtpTsBase2)
{
  const int test_count = 3;

  RtpReferenceClock clock1(RtpReferenceClock::RC_WALL_CLOCK_TIME);

  // intialise first reference clock
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  uint32_t uiRtpSyncRef = clock1.getRtpAndNtpTimestamps(uiTsFrequency, uiRtpTsBase1, uiNtpMsw, uiNtpLsw);
  // this timestamp serves as the reference time
  boost::posix_time::ptime tSyncRef = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);
  // let some time elapse before initialising the second reference clock
#ifdef _WIN32
Sleep(1000);
#else
  sleep(1);
#endif
  // intialise second reference clock
  uint32_t uiNtpMsw2 = 0, uiNtpLsw2 = 0;
  uint32_t uiRtpSyncRef2 = clock1.getRtpAndNtpTimestamps(uiTsFrequency, uiRtpTsBase2, uiNtpMsw2, uiNtpLsw2);
  // this timestamp serves as the reference time
  boost::posix_time::ptime tSyncRef2 = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw2, uiNtpLsw2);

  uint64_t uiDiffRef = (tSyncRef2 - tSyncRef).total_microseconds();

  // generate vector of start times
  std::vector<uint32_t> vRtpTimestamps1;
  std::vector<uint32_t> vRtpTimestamps2;
  for (size_t i = 0; i < test_count; ++i)
  {
    // simulate what ref clock class does
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    // TEST: RG
    uint32_t uiMsw2 = 0, uiLsw2 = 0;
    RtpTime::convertToNtpTimestamp(tNow, uiMsw2, uiLsw2, 90000);
    boost::posix_time::ptime tNowRounded = RtpTime::convertNtpTimestampToPosixTime(uiMsw2, uiLsw2);
    vRtpTimestamps1.push_back(RtpTime::convertTimeToRtpTimestamp(tNowRounded, uiTsFrequency, uiRtpTsBase1));
    vRtpTimestamps2.push_back(RtpTime::convertTimeToRtpTimestamp(tNowRounded, uiTsFrequency, uiRtpTsBase2));
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
  }

  unsigned const million = 1000000;
  // now calculate timestamps using reference clocks
  for (size_t i = 0; i < test_count; ++i)
  {
    // TS 1
    int timestampDiff = vRtpTimestamps1[i] - uiRtpSyncRef;
    double timeDiff = timestampDiff/90000.0;
    // No rounding necessary
    assert (timeDiff >= 0.0);
    unsigned seconds = (uint32_t)timeDiff;
    unsigned uSeconds = (unsigned)((timeDiff - seconds) * million + 0.5);
    uint64_t uiTotal = seconds * million + uSeconds;
    // TS 1
    int timestampDiff2 = vRtpTimestamps2[i] - uiRtpSyncRef2;
    double timeDiff2 = timestampDiff2/90000.0;
    // No rounding necessary
    assert (timeDiff2 >= 0.0);
    unsigned seconds2 = (uint32_t)timeDiff2;
    unsigned uSeconds2 = (unsigned)((timeDiff2 - seconds2) * million + 0.5);
    uint64_t uiTotal2 = seconds2 * million + uSeconds2;

    int iDiff = std::abs<int64_t>(((int64_t)uiTotal2 + uiDiffRef - uiTotal));
    BOOST_CHECK_EQUAL( iDiff < million/90000.0, true);

    boost::posix_time::ptime t1 = RtpTime::getPresentationTime(uiRtpSyncRef, tSyncRef, vRtpTimestamps1[i], 90000);
    boost::posix_time::ptime t2 = RtpTime::getPresentationTime(uiRtpSyncRef2, tSyncRef2, vRtpTimestamps2[i], 90000);
    BOOST_CHECK_EQUAL((t1-t2).total_microseconds() < million/90000.0, true);
  }
}

BOOST_AUTO_TEST_CASE(test_RtpReferenceClock)
{
  uint32_t uiTsFrequency = 90000;
  RtpSessionState sessionState1(96);
  RtpSessionState sessionState2(96);

  // test with same base
  testRtpReferenceClockImpl(uiTsFrequency, 0, 0);
  // test with different bases
  testRtpReferenceClockImpl(uiTsFrequency, sessionState1.getRtpTimestampBase(), sessionState2.getRtpTimestampBase());

  // test with same base
  testRtpReferenceClockImpl2(uiTsFrequency, 0, 0);
  // test with different bases
  testRtpReferenceClockImpl2(uiTsFrequency, sessionState1.getRtpTimestampBase(), sessionState2.getRtpTimestampBase());

}

//  static boost::posix_time::ptime convertNtpTimestampToPosixTime(uint32_t uiNtpMsw, uint32_t uiNtpLsw)
//  {
//     boost::posix_time::ptime tTime = boost::posix_time::ptime(boost::gregorian::date(1900,1,1));
//     long hours = uiNtpMsw/3600;
//     uiNtpMsw %= 3600;
//     long minutes = uiNtpMsw/60;
//     uiNtpMsw %= 60;
//     long seconds = uiNtpMsw;
//     // now convert from the 2^32 range: 10^6/2^32 = 2^26/(10^6/2^6)
//     double dMicroseconds = uiNtpLsw * 15625.0/0x04000000;
//     long microseconds = static_cast<long>(dMicroseconds + 0.5);
//     boost::posix_time::time_duration td(hours, minutes, seconds, 0);
//     td += boost::posix_time::microseconds(microseconds);
//     tTime += td;
//     return tTime;
//  }

/// test sequence number loss and reordering patterns
BOOST_AUTO_TEST_CASE(test_NtpConversion)
{
  uint32_t uiMsw1 = 0, uiLsw1 = 0;
  uint32_t uiMsw2 = 0, uiLsw2 = 0;

  boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
  RtpTime::getNTPTimeStamp(uiMsw1, uiLsw1);
  VLOG(5) << t1 << " NTP: " << hex(uiMsw1) << ":" << hex(uiLsw1);
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
  boost::posix_time::ptime t2 = boost::posix_time::microsec_clock::local_time();
  RtpTime::getNTPTimeStamp(uiMsw2, uiLsw2);
  VLOG(5) << t2 << " NTP: " <<  hex(uiMsw2) << ":" << hex(uiLsw1);

  uint32_t uiSeconds = 0;
  uint32_t uiMicroseconds = 0;
  RtpTime::getTimeDifference(t1, t2, uiSeconds, uiMicroseconds);
  double dSeconds = (double)uiSeconds + uiMicroseconds/1000000.0;

  VLOG(5) << "Difference 1:" << dSeconds << "s (" << uiSeconds << "s " << uiMicroseconds << " us)";

  uint32_t uiNtp1 = (uiMsw1 << 16) | ((uiLsw1 >> 16) & 0xFFFF);
  uint32_t uiNtp2 = (uiMsw2 << 16) | ((uiLsw2 >> 16) & 0xFFFF);

  VLOG(5) << "NTP 1:" << hex(uiNtp1) << " NTP2: " << hex(uiNtp2);

  uint32_t uiDiff = uiNtp2 -  uiNtp1;
  double dSeconds2 = (double)(uiDiff >> 16);
  dSeconds2 += (uiDiff & 0xFFFF)*15625.0/0x04000000;
  VLOG(5) << "Difference 2: " << dSeconds2 << "s";


  // to and fro NTP conversion
  uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::local_time();
  RtpTime::convertToNtpTimestamp(tNow, uiNtpMsw, uiNtpLsw);
  // convert NTP timestamp back to readable time
  boost::posix_time::ptime tAfterConversion = RtpTime::convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw);
  BOOST_CHECK_EQUAL((tAfterConversion-tNow).total_microseconds(), 0);
  BOOST_CHECK_EQUAL(tAfterConversion == tNow, true);
#if 0
  VLOG(5) << "NTP MSW:" << hex(uiNtpMsw) << " " << uiNtpMsw << " NTP LSW: " << hex(uiNtpLsw) << " " << uiNtpLsw ;
  VLOG(5) << "Testing conversion to and from NTP timestamp: " << t1 << " After: " << tAfterConversion;
#endif

  // Compare 32 bit and 64bit functions
  uint64_t uiNtp = (static_cast<uint64_t>(uiNtpMsw) << 32) | uiNtpLsw;
  // getNTPTimeStamp(uiNtp);
  boost::posix_time::ptime tNtp = RtpTime::convertNtpTimestampToPosixTime(uiNtp);
  BOOST_CHECK_EQUAL(tAfterConversion == tNtp, true);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
