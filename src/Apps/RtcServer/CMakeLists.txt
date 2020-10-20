# source files for RtcServer
SET(RTC_SRCS
RtcConfig.cpp
RtcServiceImpl.cpp
RtcServiceThread.cpp
main.cpp
)

SET(RTC_HEADERS
RtcConfig.h
RtcServerPch.h
RtcServiceImpl.h
RtcServiceThread.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtcServer ${RTC_SRCS} ${RTC_HEADERS})

IF (WIN32)
TARGET_LINK_LIBRARIES (
RtcServer
${rtp++Libs}
PeerConnectionApi
protobuf
grpc++_unsecure
grpc
gpr
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RtcServer
${rtp++Libs}
scream
PeerConnectionApi
protobuf
grpc++_unsecure
grpc
gpr
dl
)
ENDIF(WIN32)

install(TARGETS RtcServer
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

