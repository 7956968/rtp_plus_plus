# source files for RtpReceiver
SET(RTP_RECEIVER_SRCS
main.cpp
)

SET(RTP_RECEIVER_HEADERS
RtpReceiverPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(RtpReceiver ${RTP_RECEIVER_SRCS} ${RTP_RECEIVER_HEADERS})

TARGET_LINK_LIBRARIES (
RtpReceiver
${rtp++Libs}
)


# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# RtpReceiver
# Core
# glog
# #boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# RtpReceiver
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# )
# ENDIF(WIN32)

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include/
${rtp++_SOURCE_DIR}/../include/rtp++/
)

install(TARGETS RtpReceiver
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

