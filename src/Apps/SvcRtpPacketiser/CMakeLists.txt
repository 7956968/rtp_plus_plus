# source files for RtpPacketiser
# TODO: SCTP integration is hard-coded for now
SET(SVC_RTP_PACKETISER_SRCS
main.cpp
)

SET(SVC_RTP_PACKETISER_HEADERS
SvcRtpPacketiserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SvcRtpPacketiser ${SVC_RTP_PACKETISER_SRCS} ${SVC_RTP_PACKETISER_HEADERS})

TARGET_LINK_LIBRARIES (
SvcRtpPacketiser
${rtp++Libs}
)

install(TARGETS SvcRtpPacketiser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

