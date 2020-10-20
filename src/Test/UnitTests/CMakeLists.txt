# source files for Test

SET(TEST_CORE_HEADERS
CppUtilTest.h
ExperimentalTest.h
LossEstimatorTest.h
MediaTest.h
MemberEntryTest.h
MpRtpTest.h
NetworkTest.h
Rfc2326Test.h
Rfc3261Test.h
Rfc4571Test.h
Rfc4585Test.h
Rfc5285Test.h
Rfc6184Test.h
RtcpTest.h
RtoTest.h
RtpJitterBufferV2Test.h
RtpPacketGroupTest.h
RtpPlayoutBufferTest.h
RtpTimeTest.h
SctpTest.h
TransmissionManagerTest.h
)

SET(TEST_CORE_SRCS
main.cpp
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(UnitTests ${TEST_CORE_SRCS} ${TEST_CORE_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
UnitTests
Core
glog
# Windows auto links
#boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_unit_test_framework
)
ELSEIF(UNIX)
# Do linux specific includes
TARGET_LINK_LIBRARIES (
UnitTests
Core
glog
# Do linux specific includes
boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_unit_test_framework
${rtp++Libs}
)
ENDIF(WIN32)

#TARGET_LINK_LIBRARIES (
#UnitTests
#${rtp++Libs}
#)
 
install(TARGETS UnitTests DESTINATION rtp++_SOURCE_DIR}/../test})
