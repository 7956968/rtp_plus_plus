# source files for SvcRtpSender
SET(SVC_RTP_SENDER_SRCS
main.cpp
)

SET(SVC_RTP_SENDER_HEADERS
SvcRtpSenderPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SvcRtpSender ${SVC_RTP_SENDER_SRCS} ${SVC_RTP_SENDER_HEADERS})

TARGET_LINK_LIBRARIES (
SvcRtpSender
${rtp++Libs}
)


# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# SvcRtpSender
# Core
# glog
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# SvcRtpSender
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# )
# ENDIF(WIN32)

install(TARGETS SvcRtpSender
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

