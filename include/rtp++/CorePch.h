#pragma once

// Included before CoreDecl.h
// To prevent double inclusion of winsock on windows

#define NOMINMAX
#ifdef _WIN32
#include <WinSock2.h>
#endif

#define BOOST_TEST_DYN_LINK

#ifdef _WIN32
  // To get around compile error on windows: ERROR macro is defined
  #define GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include <glog/logging.h>

#include <cpputil/Utility.h>

// Define this directive in both CorePch.h AND the application to be debugged
// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING

/**
 * @def LOG_MODIFY_WITH_CARE Use this sentinel for any logging that needs to be extracted
 * from the log files and should be changed carefully, since the scripts might break.
 */

#define LOG_MODIFY_WITH_CARE "_LogSentinel_"


