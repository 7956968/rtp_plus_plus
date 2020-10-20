# source files for SipClient
SET(SIP_CLIENT_SRCS
main.cpp
)

SET(SIP_CLIENT_HEADERS
SipClientPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SipClient ${SIP_CLIENT_SRCS} ${SIP_CLIENT_HEADERS})

TARGET_LINK_LIBRARIES (
SipClient
${rtp++Libs}
)

install(TARGETS SipClient
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

