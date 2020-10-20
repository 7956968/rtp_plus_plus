# source files for RtpProxy
SET(RTP_PROXY_SRCS
main.cpp
)

SET(RTP_PROXY_HEADERS
RtpProxyPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(RtpProxy ${RTP_PROXY_SRCS} ${RTP_PROXY_HEADERS})

TARGET_LINK_LIBRARIES (
RtpProxy
${rtp++Libs}
)

install(TARGETS RtpProxy 
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

