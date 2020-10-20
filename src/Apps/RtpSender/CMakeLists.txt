# source files for RtpSender
# TODO: SCTP integration is hard-coded for now
SET(RTP_SENDER_SRCS
main.cpp
)

SET(RTP_SENDER_HEADERS
RtpSenderPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtpSender ${RTP_SENDER_SRCS} ${RTP_SENDER_HEADERS})

IF (BUILD_SCREAM)
TARGET_LINK_LIBRARIES (
RtpSender
${rtp++Libs}
#VppH264Codec
#X265Codec
scream
)
ELSE(BUILD_SCREAM)
TARGET_LINK_LIBRARIES (
RtpSender
${rtp++Libs}
#VppH264Codec
#X265Codec
)
ENDIF (BUILD_SCREAM)
# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# RtpSender
# Core
# glog
# #boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# RtpSender
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# ) 
# ENDIF(WIN32)

install(TARGETS RtpSender
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

