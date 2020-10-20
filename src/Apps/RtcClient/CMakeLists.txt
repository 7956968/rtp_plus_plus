# source files for RtcClient
SET(RTC_SRCS
main.cpp
RtcClient.cpp
RtcClientConfig.cpp
)

SET(RTC_HEADERS
RtcClient.h
RtcClientConfig.h
RtcClientPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtcClient ${RTC_SRCS} ${RTC_HEADERS})

IF (WIN32)
TARGET_LINK_LIBRARIES (
RtcClient
${rtp++Libs}
PeerConnectionApi
protobuf
grpc++_unsecure
grpc
gpr
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RtcClient
${rtp++Libs}
PeerConnectionApi
protobuf
grpc++_unsecure
grpc
gpr
dl
)
ENDIF(WIN32)

install(TARGETS RtcClient
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

