# CMakeLists.txt in VppH264Codec dir

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
${rtp++Includes}
${rtp++_SOURCE_DIR}/Libs/H264v2
${rtp++_SOURCE_DIR}/Libs/H264v2Codec
${rtp++_SOURCE_DIR}/Libs/CodecUtils
#${rtp++_SOURCE_DIR}/Libs/GeneralUtils
#${rtp++_SOURCE_DIR}/Libs/ImageUtils
#${rtp++_SOURCE_DIR}/Libs/StreamUtils
)

# Lib directories
LINK_DIRECTORIES(
${rtp++Link}
)

SET(H264v2_LIB_HDRS
VppH264Codec.h
)	

SET(H264v2_LIB_SRCS 
VppH264Codec.cpp
)

ADD_LIBRARY( VppH264Codec SHARED ${H264v2_LIB_SRCS} ${H264v2_LIB_HDRS})

TARGET_LINK_LIBRARIES(
VppH264Codec
H264v2
#RtvcCodecUtils
#RtvcGeneralUtils
#RtvcImageUtils
#RtvcStreamUtils
glog
) 
