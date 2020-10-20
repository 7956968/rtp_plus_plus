# source files
SET(SRCS
main.cpp
)

SET(HEADERS
stdafx.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)


ADD_EXECUTABLE(GenerateEc2EvalScripts ${SRCS} ${HEADERS})

TARGET_LINK_LIBRARIES (
GenerateEc2EvalScripts
glog
boost_date_time boost_filesystem boost_system boost_thread boost_program_options boost_regex
)

install(TARGETS GenerateEc2EvalScripts
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

