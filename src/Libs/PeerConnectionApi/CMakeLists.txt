# source files for RemoteCloudControl
SET(PC_SRCS
peer_connection_api.pb.cc
peer_connection_api.grpc.pb.cc
)

SET(PC_HEADERS
peer_connection_api.proto
peer_connection_api.pb.h
peer_connection_api.grpc.pb.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_LIBRARY( PeerConnectionApi STATIC ${PC_SRCS} ${PC_HEADERS})

