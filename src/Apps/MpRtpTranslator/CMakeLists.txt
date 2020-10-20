# source files for MpRtpTranslator
SET(MPRTP_TRANSLATOR_SRCS
main.cpp
)

SET(MPRTP_TRANSLATOR_HEADERS
MpRtpTranslatorPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(MpRtpTranslator ${MPRTP_TRANSLATOR_SRCS} ${MPRTP_TRANSLATOR_HEADERS})

TARGET_LINK_LIBRARIES (
MpRtpTranslator
${rtp++Libs}
)

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include/
${rtp++_SOURCE_DIR}/../include/rtp++/
)

install(TARGETS MpRtpTranslator
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

