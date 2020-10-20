# source files for RemoteCloudControlClient
SET(RCC_SRCS
main.cpp
)

SET(RCC_HEADERS
RemoteCloudControlClientPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RemoteCloudControlClient ${RCC_SRCS} ${RCC_HEADERS})

IF (WIN32)
TARGET_LINK_LIBRARIES (
RemoteCloudControlClient
${rtp++Libs}
RemoteCloudControl
protobuf
grpc++_unsecure
grpc
gpr
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RemoteCloudControlClient
${rtp++Libs}
RemoteCloudControl
protobuf
grpc++_unsecure
grpc
gpr
dl
)
ENDIF(WIN32)

install(TARGETS RemoteCloudControlClient
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

