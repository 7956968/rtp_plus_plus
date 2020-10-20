# source files for SdpWriter
SET(SDP_WRITER_SRCS
main.cpp
)

SET(SDP_WRITER_HEADERS
SdpWriterPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(SdpWriter ${SDP_WRITER_SRCS} ${SDP_WRITER_HEADERS})

TARGET_LINK_LIBRARIES (
SdpWriter
${rtp++Libs}
)

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include/
${rtp++_SOURCE_DIR}/../include/rtp++/
)

install(TARGETS SdpWriter
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

