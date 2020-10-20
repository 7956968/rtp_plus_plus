#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#endif

/// Dynamic linking of boost test
#define BOOST_TEST_DYN_LINK
/// generate main entry points
#define BOOST_TEST_MODULE ts_shared master_test_suite

#ifdef _WIN32
// https://connect.microsoft.com/VisualStudio/feedback/details/737019
// http://social.msdn.microsoft.com/Forums/vstudio/en-US/878f09ff-408f-47a8-913e-15f3a983e237/stdmaketuple-only-allows-5-parameters-with-vc11
#define  _VARIADIC_MAX  6
#endif

#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/chrono.hpp>
#include <boost/test/unit_test.hpp>

#ifdef _WIN32
#pragma warning(push)     // disable for this header only
#pragma warning(disable:4251) 
// To get around compile error on windows: ERROR macro is defined
#define GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include <glog/logging.h>
#ifdef _WIN32
#pragma warning(pop)     // restore original warning level
#endif

#include <rtp++/CorePch.h>
#include "CppUtilTest.h"
#include "ExperimentalTest.h"
#include "LossEstimatorTest.h"
#include "MediaTest.h"
#include "MemberEntryTest.h"
#include "MpRtpTest.h"
#include "NetworkTest.h"
#include "Rfc2326Test.h"
#include "Rfc3261Test.h"
#include "Rfc4571Test.h"
#include "Rfc4585Test.h"
#include "Rfc5285Test.h"
#include "Rfc6184Test.h"
#include "RtcpTest.h"
#include "RtoTest.h"
#include "RtpJitterBufferV2Test.h"
#include "RtpPacketGroupTest.h"
#include "RtpPlayoutBufferTest.h"
#include "RtpTimeTest.h"
#include "SctpTest.h"
#include "TransmissionManagerTest.h"

using namespace std;
using namespace boost::chrono;
using namespace rtp_plus_plus::test;

BOOST_AUTO_TEST_CASE( tc_test_MemberEntry )
{
  MemberEntryTest::test();
}

BOOST_AUTO_TEST_CASE( tc_test_RTO )
{
  RtoTest::test();
}

#if 0
#if 0
BOOST_AUTO_TEST_CASE (tc_RtpPacketGroup)
{
  RtpPacketGroupTest::test();
}

// This unit test tests the correct insertion points of RTP packets into the playout buffer
BOOST_AUTO_TEST_CASE( tc_test_RtpJitterBuffer )
{
  RtpJitterBufferTest::test();
}

// This unit test tests the correct insertion points of RTP packets into the playout buffer
BOOST_AUTO_TEST_CASE( tc_test_RtpJitterBufferV2)
{
  RtpJitterBufferV2Test::test();
}

#endif
#endif

#if 0
BOOST_AUTO_TEST_CASE( tc_test_TcpRtpSockets )
{
//  VLOG(5) << "Testing TCP RTP sockets";
  Rfc4571Test::test();
//  VLOG(5) << "Testing TCP RTP sockets - Done";
}
#endif
