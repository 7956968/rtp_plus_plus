# source files for SctpTest
SET(SCTP_TEST_SRCS
main.cpp
)

SET(SCTP_TEST_HEADERS
SctpTestPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SctpTest ${SCTP_TEST_SRCS} ${SCTP_TEST_HEADERS})

TARGET_LINK_LIBRARIES (
SctpTest
${rtp++Libs}
)

install(TARGETS SctpTest
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

