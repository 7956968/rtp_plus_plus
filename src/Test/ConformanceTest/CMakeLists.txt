# source files for RtpSender
SET(CONFORMANCE_TEST_SRCS
main.cpp
)

SET(CONFORMANCE_TEST_HEADERS
ConformanceTestPch.h
)

#IF(WIN32)
## Assuming the Foo-root dir is defined in FOO_DIR
#add_library( glog UNKNOWN IMPORTED )
## You can define two import-locations: one for debug and one for release.
#set_target_properties( glog PROPERTIES IMPORTED_LOCATION_DEBUG $ENV{GLOG_DIR}/Debug/libglog.lib )
#set_target_properties( glog PROPERTIES IMPORTED_LOCATION_RELEASE $ENV{GLOG_DIR}/Release/libglog.lib )
##set_target_properties( libglog PROPERTIES IMPORTED_LOCATION "C:/svn_projects/google-glog/Debug/libglog.lib" )
##set_target_properties( libglog PROPERTIES IMPORTED_LOCATION_DEBUG "C:/svn_projects/google-glog/Debug/libglog.lib" )
##set_target_properties( libglog PROPERTIES IMPORTED_LOCATION_RELEASE "C:/svn_projects/google-glog/Release/libglog.lib" )
#ENDIF(WIN32)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(ConformanceTest ${CONFORMANCE_TEST_SRCS} ${CONFORMANCE_TEST_HEADERS})

TARGET_LINK_LIBRARIES (
ConformanceTest
${rtp++Libs}
)

install(TARGETS ConformanceTest
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

