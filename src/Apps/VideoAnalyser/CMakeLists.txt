# source files for H.264 Analyser
SET(VIDEO_ANALYSER_SRCS
main.cpp
)

SET(VIDEO_ANALYSER_HEADERS
VideoAnalyserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(VideoAnalyser ${VIDEO_ANALYSER_SRCS} ${VIDEO_ANALYSER_HEADERS})

TARGET_LINK_LIBRARIES (
VideoAnalyser
${rtp++Libs}
)

install(TARGETS VideoAnalyser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

