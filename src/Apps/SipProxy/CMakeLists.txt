# source files for SipProxy
SET(SIP_PROXY_SRCS
main.cpp
)

SET(SIP_PROXY_HEADERS
SipProxyPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SipProxy ${SIP_PROXY_SRCS} ${SIP_PROXY_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
SipProxy
${rtp++Libs}
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
SipProxy
${rtp++Libs}
boost_regex
)
ENDIF(WIN32) 

install(TARGETS SipProxy
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

