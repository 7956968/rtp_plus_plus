cmake_minimum_required(VERSION 2.6)

PROJECT( rtp++ )

message("Source Dir:" ${rtp++_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${rtp++_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${rtp++_SOURCE_DIR}/../lib)
SET(RUNTIME_OUTPUT_DIRECTORY ${rtp++_SOURCE_DIR}/../bin)

SET(rtp++_BIN ${rtp++_SOURCE_DIR}/../bin)

# for profiling on linux
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

find_package (glog 0.4.0 REQUIRED)

message("rtp++_SOURCE_DIR directories:" ${rtp++_SOURCE_DIR})
message("GLOG_DIR directories:" $ENV{GLOG_DIR})
message("CPP_UTIL_DIR:" $ENV{CPP_UTIL_DIR})
message("BOOST_INCLUDE_DIR:" $ENV{BOOST_INCLUDE_DIR})

IF(WIN32)
IF(MSVC12)
set(ARTIST_CONTRIB_DIR_LIBS $ENV{ARTIST_CONTRIB_DIR}/lib/VC12)
ELSEIF(MSVC11)
set(ARTIST_CONTRIB_DIR_LIBS $ENV{ARTIST_CONTRIB_DIR}/lib/VC11)
ELSEIF(MSVC10)
set(ARTIST_CONTRIB_DIR_LIBS $ENV{ARTIST_CONTRIB_DIR}/lib/VC10)
ELSE(MSVC12)
set(ARTIST_CONTRIB_DIR_LIBS $ENV{ARTIST_CONTRIB_DIR}/lib)
ENDIF(MSVC12)
ENDIF(WIN32)

IF(WIN32)
IF(MSVC12)
message("Detected MSVC12 compiler")
set(MSVC_VER VC12)
ELSEIF(MSVC11)
message("Detected MSVC11 compiler")
set(MSVC_VER VC11)
ELSEIF(MSVC10)
message("Detected MSVC10 compiler")
set(MSVC_VER VC10)
ELSE(MSVC12)
message("WARNING: Unknown/unsupported MSVC version")
ENDIF(MSVC12)
ENDIF(WIN32)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 64bit
  message("Detected 64-bit build - compiling with -fPIC")
  SET(CMAKE_CXX_FLAGS "-fPIC")
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 32 bit
message("Detected 32-bit build")
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

IF(DEFINED ENV{USRSCTP_DIR})
message("USRSCTP_DIR:" $ENV{USRSCTP_DIR})
add_definitions(-DENABLE_SCTP_USERLAND)
ELSE()
message("No SCTP support")
ENDIF()

IF(DEFINED ENV{LIB_SRTP_DIR})
message("LIB_SRTP_DIR:" $ENV{LIB_SRTP_DIR})
add_definitions(-DENABLE_LIB_SRTP)
ELSE()
message("No SRTP support")
ENDIF()

SET(BUILD_VPP false)
IF(BUILD_VPP)
add_definitions(-DENABLE_VPP)
ENDIF(BUILD_VPP)

IF(DEFINED ENV{OPEN_H264_DIR})
message("OPEN_H264_DIR:" $ENV{OPEN_H264_DIR})
add_definitions(-DENABLE_OPEN_H264)
SET(BUILD_OPEN_H264 true)
ELSE()
message("No OpenH264 support")
SET(BUILD_OPEN_H264 false)
ENDIF()

IF(DEFINED ENV{X264_DIR})
message("X264_DIR:" $ENV{X264_DIR})
add_definitions(-DENABLE_X264)
SET(BUILD_X264 true)
ELSE()
message("No x264 support")
SET(BUILD_X264 false)
ENDIF()

IF(DEFINED ENV{X265_DIR})
message("X265_DIR:" $ENV{X265_DIR})
add_definitions(-DENABLE_X265)
SET(BUILD_X265 true)
ELSE()
message("No x265 support")
SET(BUILD_X265 false)
ENDIF()

# set this to true to build the RemoteCallControl application
SET(BUILD_CLOUD_CONTROL false)

# set this to true to build scream
SET(BUILD_SCREAM true)
IF (BUILD_SCREAM)
add_definitions(-DENABLE_SCREAM)
ENDIF (BUILD_SCREAM)

IF (UNIX)
SET(PROFILE_RTP false)
ENDIF (UNIX)

IF (UNIX)
IF (PROFILE_RTP)
message("Adding symbols for profiling")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
ENDIF (PROFILE_RTP)
ENDIF (UNIX)

#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#  # using Clang
#  message("Clang detected: setting warning level")
#  set_property( TARGET main APPEND_STRING PROPERTY COMPILE_FLAGS -Wall )
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#  # using GCC
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
#  # using Intel C++
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
#  # using Visual Studio C++
#endif()

IF(WIN32)
# Assuming the Foo-root dir is defined in FOO_DIR
add_library( glog UNKNOWN IMPORTED )
# You can define two import-locations: one for debug and one for release.
set_target_properties( glog PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{GLOG_DIR}/lib/Debug/libglog.lib )
set_target_properties( glog PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{GLOG_DIR}/lib/Release/libglog.lib )
ENDIF(WIN32)

# DCCP definitions: dccp code only works on linux
IF(UNIX)
IF(APPLE)
ELSE(APPLE)
IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  # No DCCP on FreeBSD
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
#add_definitions(-DENABLE_ASIO_DCCP)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(APPLE)
ENDIF(UNIX)

# SCTP definitions
IF (UNIX)
message("Unix variant")
add_definitions(-std=c++0x)
add_definitions(-DSCTP_DEBUG)
add_definitions(-D__Userspace__)
add_definitions(-D__Userspace_os_Linux)
add_definitions(-D_GNU_SOURCE)
add_definitions(-DSCTP_PROCESS_LEVEL_LOCKS)
add_definitions(-DSCTP_SIMPLE_ALLOCATOR)
add_definitions( -DSTDC_HEADERS=1 )
add_definitions( -DHAVE_SYS_TYPES_H=1 )
add_definitions( -DHAVE_SYS_STAT_H=1 )
add_definitions( -DHAVE_STDLIB_H=1 )
add_definitions( -DHAVE_STRING_H=1 )
add_definitions( -DHAVE_MEMORY_H=1 )
add_definitions( -DHAVE_STRINGS_H=1 )
add_definitions( -DHAVE_INTTYPES_H=1 )
add_definitions( -DHAVE_STDINT_H=1 )
add_definitions( -DHAVE_UNISTD_H=1 )
add_definitions( -DHAVE_DLFCN_H=1 )
add_definitions( -DLT_OBJDIR=\".libs/\" )
add_definitions( -DSCTP_DEBUG=1 )
add_definitions( -DINET=1 )
add_definitions( -DINET6=1 )
add_definitions( -DHAVE_SOCKET=1 )
add_definitions( -DHAVE_INET_ADDR=1 )
ELSEIF(WIN32)
message("Windows")
add_definitions(-DSCTP_DEBUG)
add_definitions(-D__Userspace_os_Windows)
add_definitions(-D_VARIADIC_MAX=10)
ELSE(UNIX)
message("Unknown OS")
ENDIF (UNIX)

# Assuming the Foo-root dir is defined in FOO_DIR
#add_library( glog SHARED IMPORTED )
# You can define two import-locations: one for debug and one for release.
#set_target_properties( glog PROPERTIES IMPORTED_LOCATION_DEBUG ${GLOG_DIR}/Debug/libglog.lib )
#set_target_properties( glog PROPERTIES IMPORTED_LOCATION_RELEASE ${GLOG_DIR}/Release/libglog.lib )

IF(WIN32)
SET(rtp++Includes
${rtp++_SOURCE_DIR}
${rtp++_SOURCE_DIR}/Lib
${rtp++_SOURCE_DIR}/Libs
$ENV{GLOG_DIR}/src/windows
$ENV{CPP_UTIL_DIR}/include
$ENV{BOOST_INCLUDE_DIR}
$ENV{USRSCTP_DIR}
$ENV{USRSCTP_DIR}/usrsctplib
$ENV{LIB_SRTP_DIR}/include
)
ELSEIF(UNIX)
# Include directories
SET(rtp++Includes
${rtp++_SOURCE_DIR}
${rtp++_SOURCE_DIR}/Libs
${rtp++_SOURCE_DIR}/../include/rtp++/
$ENV{CPP_UTIL_DIR}/include/
$ENV{BOOST_INCLUDE_DIR}
$ENV{USRSCTP_DIR}
$ENV{USRSCTP_DIR}/usrsctplib
$ENV{LIB_SRTP_DIR}/include
/usr/local/include
/usr/include
)
ENDIF(WIN32)

IF(DEFINED ENV{LIB_SRTP_DIR})
SET(rtp++Includes
${rtp++Includes}
$ENV{LIB_SRTP_DIR}/include
$ENV{LIB_SRTP_DIR}/crypto/include
)
ENDIF()

message("rtp++Includes directories:" ${rtp++Includes})

# lib directories
IF(WIN32)
SET(rtp++Link
${rtp++_SOURCE_DIR}/../lib
$ENV{BOOST_LIB_DIR}
)
ELSEIF(UNIX)
SET(rtp++Link
${rtp++_SOURCE_DIR}/../lib
${BOOST_LIB_DIR}
/usr/local/lib
/usr/lib
$ENV{USRSCTP_DIR}/usrsctplib/.libs
)
ENDIF(WIN32)

IF(DEFINED ENV{LIB_SRTP_DIR})
SET(rtp++Link
${rtp++Link}
$ENV{LIB_SRTP_DIR}
$ENV{LIB_SRTP_DIR}/crypto
)
ENDIF()

message("rtp++Link directories:" ${rtp++Link})

# libs
IF(WIN32)
# Do windows specific includes
SET(rtp++Libs
Core
glog
)
ELSEIF(APPLE)
# Do apple specific includes
SET(rtp++Libs
Core
glog
boost_date_time boost_filesystem boost_system boost_thread boost_program_options boost_regex
pthread
c++
)
ELSEIF(UNIX)
# Do linux specific includes
SET(rtp++Libs
Core
#OpenH264Codec
#VppH264Codec
#X264Codec
#X265Codec
glog::glog
#glog
boost_date_time boost_filesystem boost_system boost_thread boost_program_options boost_regex
pthread
)
ENDIF(WIN32) 

IF (UNIX)
IF (BUILD_X264)
SET(rtp++Libs
${rtp++Libs}
X264Codec
)
ENDIF (BUILD_X264)
IF (BUILD_X265)
SET(rtp++Libs
${rtp++Libs}
X265Codec
)
ENDIF (BUILD_X265)
IF (BUILD_OPEN_H264)
SET(rtp++Libs
${rtp++Libs}
OpenH264Codec
)
ENDIF (BUILD_OPEN_H264)

ENDIF(UNIX)

IF(DEFINED ENV{USRSCTP_DIR})
SET(rtp++Libs
${rtp++Libs}
usrsctp
)
ENDIF()

IF(DEFINED ENV{LIB_SRTP_DIR})
SET(rtp++Libs
${rtp++Libs}
srtp
cryptomath
cryptomodule
)
ENDIF()


SET(LibDir
${rtp++_SOURCE_DIR}/../lib
$ENV{BOOST_LIB_DIR}
)

IF(DEFINED ENV{LIB_SRTP_DIR})
SET(LibDir
${LibDir}
$ENV{LIB_SRTP_DIR}
)
ENDIF()

IF (BUILD_SCREAM)
SET(rtp++Libs
${rtp++Libs}
scream
)
ENDIF (BUILD_SCREAM)


message("LibDir directories:" ${LibDir})

# Include directories
INCLUDE_DIRECTORIES(
${rtp++_SOURCE_DIR}/../include
$ENV{CPP_UTIL_DIR}/include
)

ADD_SUBDIRECTORY( Apps) 
ADD_SUBDIRECTORY( Lib) 
ADD_SUBDIRECTORY( Libs)
ADD_SUBDIRECTORY( Test )
ADD_SUBDIRECTORY( Tools )
