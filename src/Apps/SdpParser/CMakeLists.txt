# source files for SdpParser
SET(SDP_PARSER_SRCS
main.cpp
)

SET(SDP_PARSER_HEADERS
SdpParserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SdpParser ${SDP_PARSER_SRCS} ${SDP_PARSER_HEADERS})

TARGET_LINK_LIBRARIES (
SdpParser
${rtp++Libs}
)

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include/
${rtp++_SOURCE_DIR}/../include/rtp++/
)

install(TARGETS SdpParser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

