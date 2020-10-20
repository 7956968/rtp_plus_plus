# source files for StatelessProxy
SET(SIP_PROXY_SRCS
main.cpp
)

SET(SIP_PROXY_HEADERS
StatelessProxyPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(StatelessProxy ${SIP_PROXY_SRCS} ${SIP_PROXY_HEADERS})

IF(WIN32)
# Do windows specific includes
TARGET_LINK_LIBRARIES (
StatelessProxy
${rtp++Libs}
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
StatelessProxy
${rtp++Libs}
boost_regex
)
ENDIF(WIN32) 

install(TARGETS StatelessProxy
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

