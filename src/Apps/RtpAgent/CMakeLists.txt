# source files for RtpAgent
SET(RTP_AGENT_SRCS
main.cpp
)

SET(RTP_AGENT_HEADERS
RtpAgentPch.h
)

#IF(WIN32)
# Assuming the Foo-root dir is defined in FOO_DIR
#add_library( GLOG SHARED IMPORTED )
# You can define two import-locations: one for debug and one for release.
#set_target_properties( GLOG PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{GLOG_DIR}/Debug/libglog.lib )
#set_target_properties( GLOG PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{GLOG_DIR}/Release/libglog.lib )
#ENDIF(WIN32)


INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(RtpAgent ${RTP_AGENT_SRCS} ${RTP_AGENT_HEADERS})

TARGET_LINK_LIBRARIES (
RtpAgent
${rtp++Libs}
)

# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# RtpAgent
# Core
# glog
# #boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# RtpAgent
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# )
# ENDIF(WIN32)

install(TARGETS RtpAgent
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

