#include "CorePch.h"
#include <rtp++/application/ApplicationContext.h>

#ifdef ENABLE_SCTP_USERLAND
#include <stdarg.h>
#include <usrsctplib/usrsctp.h>
#endif

#ifdef ENABLE_LIB_SRTP
#include <srtp.h>
#endif

#ifdef ENABLE_SCTP_USERLAND
void
debug_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
#endif

namespace rtp_plus_plus
{
namespace app
{

ApplicationContext& ApplicationContext::getInstance()
{
  static ApplicationContext context;
  return context;
}

bool ApplicationContext::initialiseApplication(const char* szApplication)
{
  // call any code here that needs to be called on application startup
  google::InitGoogleLogging(szApplication);
#ifndef WIN32
  google::InstallFailureSignalHandler();
#endif

#ifdef ENABLE_SCTP

#endif

#ifdef ENABLE_LIB_SRTP
  // initialize libSRTP
  err_status_t err = srtp_init();
  if (err)
  {
    LOG(WARNING) << "Error in init SRTP: " << err;
    return false;
  }
#endif

  return true;
}

bool ApplicationContext::initialiseUsrSctpPort(uint16_t uiLocalUdpEncapsPort)
{
#ifdef ENABLE_SCTP_USERLAND
#define USE_KERNEL_VERSION
#ifdef USE_KERNEL_VERSION
  usrsctp_init(uiLocalUdpEncapsPort, NULL, debug_printf);
#else
  usrsctp_init(uiLocalUdpEncapsPort);
#endif
#endif
  // update flag so that we can shutdown cleanly
  ApplicationContext& context = getInstance();
  context.m_bSctpInitialised = true;
  return true;
}

bool ApplicationContext::cleanupApplication()
{
#ifdef ENABLE_LIB_SRTP
  // deinitialize libSRTP
  err_status_t err = srtp_shutdown();
  if (err)
  {
    LOG(WARNING) << "Error in deinit SRTP: " << err;
  }
#endif

  // call any code here that needs to be called on application termination
  cleanupUserlandSctp();
  return true;
}

ApplicationContext::ApplicationContext()
  :m_bSctpInitialised(false)
{

}

void ApplicationContext::cleanupUserlandSctp()
{
  ApplicationContext& context = getInstance();
  if (context.m_bSctpInitialised)
  {
#ifdef ENABLE_SCTP_USERLAND
  // there seems to be a bug in SCTP shutdown
  // commenting this out for now
  /*
  LOG(INFO) << "Waiting for SCTP to shutdown";
  while (usrsctp_finish() != 0) {
#if defined (__Userspace_os_Windows)
    Sleep(1000);
#else
    sleep(1);
#endif
  }
  */
  LOG(INFO) << "SCTP shutdown complete";
#endif

  }
}

}
}

