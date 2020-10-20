# source files for H.265 Analyser
SET(H265_ANALYSER_SRCS
main.cpp
)

SET(H265_ANALYSER_HEADERS
H265AnalyserPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(H265Analyser ${H265_ANALYSER_SRCS} ${H265_ANALYSER_HEADERS})

TARGET_LINK_LIBRARIES (
H265Analyser
${rtp++Libs}
)

install(TARGETS H265Analyser
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

