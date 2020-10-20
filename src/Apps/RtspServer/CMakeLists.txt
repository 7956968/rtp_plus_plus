# source files for RtspServer
SET(RTSP_SERVER_SRCS
main.cpp
)

SET(RTSP_SERVER_HEADERS
RtspServerPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtspServer ${RTSP_SERVER_SRCS} ${RTSP_SERVER_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
RtspServer
${rtp++Libs}
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RtspServer
${rtp++Libs}
boost_regex
)
ENDIF(WIN32) 

install(TARGETS RtspServer
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

