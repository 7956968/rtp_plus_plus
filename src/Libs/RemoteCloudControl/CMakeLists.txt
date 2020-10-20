# source files for RemoteCloudControl
SET(RCC_SRCS
remote_cloud_control.pb.cc
remote_cloud_control.grpc.pb.cc
)

SET(RCC_HEADERS
remote_cloud_control.proto
remote_cloud_control.pb.h
remote_cloud_control.grpc.pb.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_LIBRARY( RemoteCloudControl STATIC ${RCC_SRCS} ${RCC_HEADERS})

