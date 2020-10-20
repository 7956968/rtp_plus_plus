# source files
SET(GEN_PACKET_TRACE_SRCS
main.cpp
)

SET(GEN_PACKET_TRACE_HEADERS
GeneratePacketTracePch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(GeneratePacketTrace ${GEN_PACKET_TRACE_SRCS} ${GEN_PACKET_TRACE_HEADERS})

TARGET_LINK_LIBRARIES (
GeneratePacketTrace
${rtp++Libs}
)

install(TARGETS GeneratePacketTrace
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

