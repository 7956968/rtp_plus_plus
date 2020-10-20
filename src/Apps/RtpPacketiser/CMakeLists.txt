# source files for RtpPacketiser
# TODO: SCTP integration is hard-coded for now
SET(RTP_PACKETISER_SRCS
main.cpp
)

SET(RTP_PACKETISER_HEADERS
RtpPacketiserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtpPacketiser ${RTP_PACKETISER_SRCS} ${RTP_PACKETISER_HEADERS})

TARGET_LINK_LIBRARIES (
RtpPacketiser
${rtp++Libs}
)


# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# RtpPacketiser
# Core
# glog
# #boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# RtpPacketiser
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# ) 
# ENDIF(WIN32)

install(TARGETS RtpPacketiser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

