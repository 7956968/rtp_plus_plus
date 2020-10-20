# source files
SET(PSNR_SRCS
main.cpp
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

ADD_EXECUTABLE(GeneratePSNR ${PSNR_SRCS} ${PSNR_HEADERS})

TARGET_LINK_LIBRARIES (
GeneratePSNR
)

install(TARGETS GeneratePSNR
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

