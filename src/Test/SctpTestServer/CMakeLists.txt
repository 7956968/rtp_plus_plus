# source files for SctpTestServer
SET(SCTP_TEST_SERVER_SRCS
SctpAssociation.cpp
main.cpp
)

SET(SCTP_TEST_SERVER_HEADERS
SctpTestServerPch.h
SctpAssociation.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SctpTestServer ${SCTP_TEST_SERVER_SRCS} ${SCTP_TEST_SERVER_HEADERS})

TARGET_LINK_LIBRARIES (
SctpTestServer
${rtp++Libs}
)

install(TARGETS SctpTestServer
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

