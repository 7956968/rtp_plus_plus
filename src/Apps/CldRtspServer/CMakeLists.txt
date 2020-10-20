# source files for RtspServer
SET(CLD_RTSP_SERVER_SRCS
main.cpp
CldRtspServer.cpp
)

SET(CLD_RTSP_SERVER_HEADERS
CldRtspServerPch.h
CldRtspServer.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(CldRtspServer ${CLD_RTSP_SERVER_SRCS} ${CLD_RTSP_SERVER_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
CldRtspServer
${rtp++Libs}
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
CldRtspServer
${rtp++Libs}
boost_regex
)
ENDIF(WIN32) 

install(TARGETS CldRtspServer
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

