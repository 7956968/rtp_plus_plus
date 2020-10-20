# source files for SctpTestClient
SET(SCTP_TEST_CLIENT_SRCS
main.cpp
../SctpTestServer/SctpAssociation.cpp
)

SET(SCTP_TEST_CLIENT_HEADERS
SctpTestClientPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SctpTestClient ${SCTP_TEST_CLIENT_SRCS} ${SCTP_TEST_CLIENT_HEADERS})

TARGET_LINK_LIBRARIES (
SctpTestClient
${rtp++Libs}
)

install(TARGETS SctpTestClient
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

