#include "RemoteCloudControlClientPch.h"
#include <iostream>
#include <grpc/grpc.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>

#include <RemoteCloudControl/remote_cloud_control.grpc.pb.h>

using grpc::ChannelArguments;
using grpc::CompletionQueue;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class RemoteCloudControlClient
{
public:

  RemoteCloudControlClient(std::shared_ptr<ChannelInterface> channel)
    :stub_(rtp_plus_plus::RemoteCallControlService::NewStub(channel))
  {

  }

  rtp_plus_plus::Response call()
  {
    rtp_plus_plus::CalleeInfo calleeInfo;
    calleeInfo.set_sip_uri("sip:alice@10.0.0.1");
    rtp_plus_plus::Response response;
    ClientContext context;
    CompletionQueue cq;
#if 0

    Status status;
    std::unique_ptr<ClientAsyncResponseReader<rtp_plus_plusResponse> > rpc(
        stub_->call(&context, callerInfo, &cq, (void*)1));
    void* got_tag;
    bool ok;
    cq.Next(&got_tag, &ok);
    GPR_ASSERT(ok);
    GPR_ASSERT(got_tag == (void*)1);

    rpc->Finish(&response, &status, (void*)2);
    cq.Next(&got_tag, &ok);
    GPR_ASSERT(ok);
    GPR_ASSERT(got_tag == (void*)2);

    if (status.IsOk()) {
      return response;
    } else {
      return response;
    }
#else
    Status status = stub_->call(&context, calleeInfo, &response);
    if (status.IsOk())
    {
      LOG(INFO) << "RPC success";       
    }
    else
    {
      LOG(INFO) << "RPC failed";       
    }


#endif
    return response;
  }


  void Shutdown() { stub_.reset(); } 

private:

  std::unique_ptr<rtp_plus_plus::RemoteCallControlService::Stub> stub_;
};

int main()
{
  grpc_init();

  RemoteCloudControlClient rccClient( grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials(), ChannelArguments()));

  rccClient.call();
  
  rccClient.Shutdown();
  grpc_shutdown();
  
  return 0;
}

