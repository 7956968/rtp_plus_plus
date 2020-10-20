#include "RemoteCloudControlServerPch.h"
#include <iostream>
#include <thread>
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>

#include <RemoteCloudControl/remote_cloud_control.grpc.pb.h>

#include <cpputil/ConsoleApplicationUtil.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

class RemoteCallControlServiceImpl final : public rtp_plus_plus::RemoteCallControlService::Service
{
public:

  virtual ::grpc::Status call(::grpc::ServerContext* context, const ::rtp_plus_plus::CalleeInfo* request, ::rtp_plus_plus::Response* response)
{
  // TODO: implement call control
  LOG(INFO) << "RPC call() invoked: " << request->sip_uri();  
  return Status::OK;
}


private:

};

void RunServer() 
{
  std::string server_address("0.0.0.0:50051");
  RemoteCallControlServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main()
{
  LOG(INFO) << "main";  
  grpc_init();

  LOG(INFO) << "running server";  
  RunServer();

  LOG(INFO) << "shutting down";  
  grpc_shutdown();
 
  LOG(INFO) << "app complete";  
 
  return 0;
}

