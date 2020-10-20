#include "RtpW.h"

// To prevent double inclusion of winsock on windows
#ifdef _WIN32
// To be able to use std::max
#define NOMINMAX
#include <WinSock2.h>
#endif

#ifdef _WIN32
  // To get around compile error on windows: ERROR macro is defined
  #define GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include <glog/logging.h>

#include "RtpWrapper.h"

void* createInstance()
{
  google::InitGoogleLogging("RtpW");

  RtpWrapper* pWrapper = new RtpWrapper();

  int res = pWrapper->start();
  if (res != RTPW_EC_NO_ERROR)
  {
      LOG(ERROR) << "Failed to start RtpWrapper EC: " << res;
      delete pWrapper;
      return NULL;
  }
  else
  {
      return pWrapper;
  }
}

void deleteInstance(void* instance)
{
  if (instance != 0)
  {
    RtpWrapper* pInstance = static_cast<RtpWrapper*>(instance);
    int res = pInstance->stop();
    if (res != RTPW_EC_NO_ERROR)
    {
        LOG(ERROR) << "Failed to stop RtpWrapper cleanly- EC: " << res;
    }
    delete pInstance;
  }
}

int sendRtpPacket(void* instance, uint8_t uiPayloadType, uint8_t* pBuffer, uint32_t uiBufferSize, uint64_t uiPts, uint64_t uiDts)
{
  if (instance != 0)
  {
      RtpWrapper* pInstance = static_cast<RtpWrapper*>(instance);
      return pInstance->sendRtpPacket(uiPayloadType, pBuffer, uiBufferSize, uiPts, uiDts);
  }
  return RTPW_EC_NULL_POINTER;
}
