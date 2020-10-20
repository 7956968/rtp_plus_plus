#include "RtcServerPch.h"
#include "RtcServiceThread.h"
#include "RtcServiceImpl.h"
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/stream.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

namespace rtp_plus_plus {

RtcServiceThread::RtcServiceThread(const RtcConfig& rtcConfig)
  :m_rtcConfig(rtcConfig)
{
  LOG(INFO) << "RtcServiceThread";
}

void RtcServiceThread::run()
{
  if (!m_pThread)
  {
    m_pThread = std::unique_ptr<std::thread>(new std::thread(std::bind(&RtcServiceThread::runServer, this)));
  }
}

void RtcServiceThread::shutdown()
{
  if (m_pThread && m_pRtcServer)
  {
    LOG(INFO) << "Shutting down RtcServer";
    m_pRtcServer->Shutdown();
    m_pThread->join();
    m_pThread.reset();
  }
  else
  {
    LOG(INFO) << "Shutdown ok, no thread running";
  }
}

void RtcServiceThread::runServer()
{
  grpc_init();
  std::ostringstream serverAddress;
  serverAddress << "0.0.0.0:" << m_rtcConfig.RpcPort;
  RtcServiceImpl service(m_rtcConfig);
  ServerBuilder builder;
  builder.AddListeningPort(serverAddress.str(), grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  m_pRtcServer = std::move(std::unique_ptr<Server>(builder.BuildAndStart()));
  LOG(INFO) << "Server listening on " << serverAddress.str();
  m_pRtcServer->Wait();
  grpc_shutdown();
}

} // rtp_plus_plus
