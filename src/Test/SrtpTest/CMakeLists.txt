# source files for SrtpTest
SET(SRTP_TEST_SRCS
main.cpp
)

SET(SRTP_TEST_HEADERS
SrtpTestPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SrtpTest ${SRTP_TEST_SRCS} ${SRTP_TEST_HEADERS})

TARGET_LINK_LIBRARIES (
SrtpTest
${rtp++Libs}
)

install(TARGETS SrtpTest
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

