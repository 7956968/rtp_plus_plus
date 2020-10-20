# source files for RemoteCloudControlServer
SET(RCC_SRCS
main.cpp
)

SET(RCC_HEADERS
RemoteCloudControlServerPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RemoteCloudControlServer ${RCC_SRCS} ${RCC_HEADERS})

IF (WIN32)
TARGET_LINK_LIBRARIES (
RemoteCloudControlServer
${rtp++Libs}
RemoteCloudControl
protobuf
grpc++_unsecure
grpc
gpr
)
ELSEIF(UNIX)
TARGET_LINK_LIBRARIES (
RemoteCloudControlServer
${rtp++Libs}
RemoteCloudControl
protobuf
grpc++_unsecure
grpc
gpr
dl
)
ENDIF(WIN32)

install(TARGETS RemoteCloudControlServer
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

