# source files for RtpAgent
SET(MPRTO_SRCS
main.cpp
)

SET(MPRTO_HEADERS
MultipathRTOPch.h
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
)

LINK_DIRECTORIES(
${rtp++Link}
)

ADD_EXECUTABLE(MultipathRTO ${MPRTO_SRCS} ${MPRTO_HEADERS})

TARGET_LINK_LIBRARIES (
MultipathRTO
${rtp++Libs}
)

# IF(WIN32)
#     # Do windows specific includes
# TARGET_LINK_LIBRARIES (
# MultipathRTO
# Core
# glog
# #boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread
# ) 
# ELSEIF(UNIX)
#     # Do linux specific includes
# TARGET_LINK_LIBRARIES (
# MultipathRTO
# Core
# glog
# boost_chrono boost_date_time boost_filesystem boost_regex boost_signals boost_system boost_thread boost_program_options
# pthread
# usrsctp
# )
# ENDIF(WIN32)

install(TARGETS MultipathRTO
            RUNTIME DESTINATION ${rtp++_BIN}
            LIBRARY DESTINATION ${rtp++_BIN}
            ARCHIVE DESTINATION ${rtp++_SOURCE_DIR}/../lib)

