#pragma once
#include <functional>
#include <thread>
#include <grpc++/server.h>
#include "RtcConfig.h"

namespace rtp_plus_plus {

/**
 * @brief The RtcServiceThread class manages an instance of the RTC server running in a separate thread.
 *
 */
class RtcServiceThread
{
public:
  /**
   * @brief RtcServiceThread
   */
  RtcServiceThread(const RtcConfig& rtcConfig);
  /**
   * @brief run
   */
  void run();
  /**
   * @brief shutdown
   */
  void shutdown();

private:
  RtcServiceThread(const RtcServiceThread&) = delete;
  RtcServiceThread& operator=(const RtcServiceThread&) = delete;
  void runServer();

private:

  RtcConfig m_rtcConfig;
  std::unique_ptr<std::thread> m_pThread;
  std::unique_ptr<grpc::Server> m_pRtcServer;
};

} // rtp_plus_plus
