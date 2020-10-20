# source files for RtpAnalyser
SET(RTP_ANALYSER_SRCS
main.cpp
)

SET(RTP_ANALYSER_HEADERS
RtpAnalyserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(RtpAnalyser ${RTP_ANALYSER_SRCS} ${RTP_ANALYSER_HEADERS})

TARGET_LINK_LIBRARIES (
RtpAnalyser
${rtp++Libs}
)

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include/
${rtp++_SOURCE_DIR}/../include/rtp++/
)

install(TARGETS RtpAnalyser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

