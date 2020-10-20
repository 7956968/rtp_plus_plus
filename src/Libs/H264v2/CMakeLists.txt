# CmakeLists.txt in H264v2 dir

IF(WIN32)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BUILD_FOR_SHORT")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
# get rid of some compiler warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
ELSE(WIN32)
add_definitions("-Wwrite-strings")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
ENDIF(WIN32)

INCLUDE_DIRECTORIES(
${artistIncludes}
../H264v2Codec
${artist_SOURCE_DIR}/libs/CodecUtils
${artist_SOURCE_DIR}/libs/GeneralUtils
${artist_SOURCE_DIR}/libs/ImageUtils
${artist_SOURCE_DIR}/libs/StreamUtils
)

# Lib directories
LINK_DIRECTORIES(
${artistLink}
)

SET(H264v2_LIB_HDRS
H264v2.h
../H264v2Codec/H264v2Codec.h
../H264v2Codec/H264v2CodecHeader.h
stdafx.h
)

SET(H264v2_LIB_SRCS 
H264v2.cpp
../H264v2Codec/H264v2Codec.cpp
../H264v2Codec/H264v2CodecHeader.cpp
stdafx.cpp
)

ADD_LIBRARY( H264v2 SHARED ${H264v2_LIB_SRCS} ${H264v2_LIB_HDRS})

TARGET_LINK_LIBRARIES(
H264v2
RtvcCodecUtils
RtvcGeneralUtils
RtvcImageUtils
RtvcStreamUtils
) 
