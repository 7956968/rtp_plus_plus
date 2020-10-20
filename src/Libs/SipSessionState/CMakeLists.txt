# source files for RtpW
SET(SSS_SRCS
SipSessionState.cpp
)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 64bit
  message("Detected 64-bit system - fPIC")
  SET(CMAKE_CXX_FLAGS "-fPIC")
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 32 bit
  message("Detected 32-bit system")
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

SET(SSS_HEADERS
DllSetup.h
SipSessionState.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_LIBRARY( SipSessionState SHARED ${SSS_SRCS} ${SSS_HEADERS})

TARGET_LINK_LIBRARIES (
SipSessionState
${rtp++Libs}
)

install(TARGETS SipSessionState
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

