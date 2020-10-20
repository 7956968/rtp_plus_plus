# source files for RtspClient
SET(RTSP_CLIENT_SRCS
main.cpp
)

SET(RTSP_CLIENT_HEADERS
RtspClientPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtspClient ${RTSP_CLIENT_SRCS} ${RTSP_CLIENT_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
RtspClient
${rtp++Libs}
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RtspClient
${rtp++Libs}
)
ENDIF(WIN32) 

install(TARGETS RtspClient
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

