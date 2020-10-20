# source files for RtpW
SET(RTPW_SRCS
RtpW.cpp
RtpWrapper.cpp
)

SET(RTPW_HEADERS
RtpW.h
RtpWrapper.cpp
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

# Lib directories
LINK_DIRECTORIES(
${rtp++_SOURCE_DIR}/../lib
$ENV{BOOST_LIB_DIR} 
/usr/local/lib
/usr/lib
)

ADD_LIBRARY( Rtp++W SHARED ${RTPW_SRCS} ${RTPW_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
Rtp++W
Core
glog
#boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
) 
ELSEIF(UNIX)
# Do linux specific includes
TARGET_LINK_LIBRARIES (
Rtp++W
Core
glog
boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
) 
ENDIF(WIN32)

install(TARGETS Rtp++W
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

