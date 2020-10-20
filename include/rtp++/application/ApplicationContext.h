#pragma once
#include <cstdint>
#include <boost/noncopyable.hpp>

#ifdef ENABLE_SCTP_USERLAND
#include <usrsctplib/usrsctp.h>
#endif

namespace rtp_plus_plus 
{
namespace app
{

/**
  * The purpose of this class is to add application specific initialisation
  * The user land sctp stack for example needs to be shutdown correctly if the stack
  * was used
  */
class ApplicationContext : private boost::noncopyable
{
public:
  static ApplicationContext& getInstance();
  static bool initialiseApplication(const char* szApplication);
  static bool initialiseUsrSctpPort(uint16_t uiLocalUdpEncapsPort);
  static bool cleanupApplication();

private:
  ApplicationContext();
  static void cleanupUserlandSctp();

  /**
    * This flag tracks whether SCTP has been initialised during the application
    * and is used to shutdown user land SCTP cleanly if so
    */
  bool m_bSctpInitialised;
};

}
}

