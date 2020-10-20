# source files
SET(MPRTODE_SRCS
main.cpp
)

SET(MPRTODE_HEADERS
DataStructures.h
MultipathRTODataExtractorPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(MultipathRTODataExtractor ${MPRTODE_SRCS} ${MPRTODE_HEADERS})

TARGET_LINK_LIBRARIES (
MultipathRTODataExtractor
${rtp++Libs}
)

install(TARGETS MultipathRTODataExtractor
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

