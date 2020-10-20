# CMakeLists.txt in OpenH264Codec dir

IF(WIN32)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BUILD_FOR_SHORT")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
# get rid of some compiler warnings
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
ELSE(WIN32)
add_definitions("-Wwrite-strings")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
ENDIF(WIN32)

# on windows we use a custom compile of OpenH264
IF(WIN32)
add_library( openh264 UNKNOWN IMPORTED )
set_target_properties( openh264 PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{OPEN_H264_DIR}/bin/Win32/Debug/welsenc.lib )
set_target_properties( openh264 PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{OPEN_H264_DIR}/bin/Win32/Release/welsenc.lib )
ENDIF(WIN32)

INCLUDE_DIRECTORIES(
${rtp++Includes}
$ENV{OPEN_H264_DIR}/codec/api/svc
)

# Lib directories
LINK_DIRECTORIES(
${rtp++Link}
${OPEN_H264_DIR}/bin/Win32/Release/
)

SET(H264v2_LIB_HDRS
OpenH264Codec.h
)	

SET(H264v2_LIB_SRCS 
OpenH264Codec.cpp
)

ADD_LIBRARY( OpenH264Codec SHARED ${H264v2_LIB_SRCS} ${H264v2_LIB_HDRS})

TARGET_LINK_LIBRARIES(
OpenH264Codec
openh264
glog
) 

