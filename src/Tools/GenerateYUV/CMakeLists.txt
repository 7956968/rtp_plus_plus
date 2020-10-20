# source files
SET(GEN_YUV_SRCS
main.cpp
)

SET(GEN_YUV_HEADERS
GenerateYUVPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(GenerateYUV ${GEN_YUV_SRCS} ${GEN_YUV_HEADERS})

TARGET_LINK_LIBRARIES (
GenerateYUV
${rtp++Libs}
)

install(TARGETS GenerateYUV
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

