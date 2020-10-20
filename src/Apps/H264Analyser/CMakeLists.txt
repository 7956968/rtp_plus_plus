# source files for H.264 Analyser
SET(H264_ANALYSER_SRCS
main.cpp
)

SET(H264_ANALYSER_HEADERS
H264AnalyserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(H264Analyser ${H264_ANALYSER_SRCS} ${H264_ANALYSER_HEADERS})

TARGET_LINK_LIBRARIES (
H264Analyser
${rtp++Libs}
)

install(TARGETS H264Analyser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

