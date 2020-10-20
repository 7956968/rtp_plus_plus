#include "RtcClientPch.h"
#include "RtcClient.h"
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>

using grpc::CompletionQueue;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

RtcClient::RtcClient(std::shared_ptr<ChannelInterface> channel)
  :stub_(peer_connection::PeerConnectionApi::NewStub(channel))
{

}

boost::optional<std::string> RtcClient::createOffer(const std::string& sContentType)
{
  ClientContext context;

  peer_connection::OfferDescriptor desc;
  desc.set_content_type(sContentType);

  peer_connection::CreateSessionDescriptionResponse response;

  Status status = stub_->createOffer(&context, desc, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC createOffer success";
    if (response.response_code() == 200)
    {
      return boost::optional<std::string>(response.session_description().session_description());
    }
    else
    {
      LOG(WARNING) << "Server returned error: " << response.response_code() << " " << response.response_description();
      return boost::optional<std::string>();
    }
  }
  else
  {
    LOG(WARNING) << "RPC createOffer failed";
    return boost::optional<std::string>();
  }
}

boost::optional<std::string> RtcClient::createAnswer(const std::string& sSessionDescription, const std::string& sContentType)
{
  ClientContext context;

  peer_connection::AnswerDescriptor desc;
  peer_connection::SessionDescription* pOffer = new peer_connection::SessionDescription();
  pOffer->set_content_type(sContentType);
  pOffer->set_session_description(sSessionDescription);
  desc.set_allocated_session_description(pOffer);

  peer_connection::CreateSessionDescriptionResponse response;

  Status status = stub_->createAnswer(&context, desc, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC createAnswer success";
    if (response.response_code() == 200)
    {
      return boost::optional<std::string>(response.session_description().session_description());
    }
    else
    {
      LOG(WARNING) << "Server returned error: " << response.response_code() << " " << response.response_description();
      return boost::optional<std::string>();
    }
  }
  else
  {
    LOG(WARNING) << "RPC createAnswer failed";
    return boost::optional<std::string>();
  }
}

bool RtcClient::setRemoteDescription(const std::string& sSessionDescription, const std::string& sContentType)
{
  ClientContext context;

  peer_connection::SessionDescription remoteSdp;
  remoteSdp.set_content_type(sContentType);
  remoteSdp.set_session_description(sSessionDescription);

  peer_connection::SetSessionDescriptionResponse response;

  Status status = stub_->setRemoteDescription (&context, remoteSdp, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC setRemoteDescription success";
    if (response.response_code() == 200)
    {
      return true;
    }
    else
    {
      LOG(WARNING) << "Server returned error: " << response.response_code() << " " << response.response_description();
      return false;
    }
  }
  else
  {
    LOG(WARNING) << "RPC setRemoteDescription failed";
    return false;
  }
}

bool RtcClient::startStreaming()
{
  ClientContext context;
  peer_connection::StartStreamingRequest request;
  peer_connection::StartStreamingResponse response;

  Status status = stub_->startStreaming(&context, request, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC startStreaming success";
    if (response.response_code() == 200)
    {
      return true;
    }
    else
    {
      LOG(WARNING) << "startStreaming: Server returned error: " << response.response_code() << " " << response.response_description();
      return false;
    }
  }
  else
  {
    LOG(WARNING) << "RPC startStreaming failed";
    return false;
  }
}

bool RtcClient::stopStreaming()
{
  ClientContext context;
  peer_connection::StopStreamingRequest request;
  peer_connection::StopStreamingResponse response;

  Status status = stub_->stopStreaming(&context, request, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC stopStreaming success";
    if (response.response_code() == 200)
    {
      return true;
    }
    else
    {
      LOG(WARNING) << "stopStreaming: Server returned error: " << response.response_code() << " " << response.response_description();
      return false;
    }
  }
  else
  {
    LOG(WARNING) << "RPC stopStreaming failed";
    return false;
  }
}

bool RtcClient::shutdownRemoteRtcServer()
{
  ClientContext context;
  peer_connection::ShutdownRequest request;
  peer_connection::ShutdownResponse response;

  Status status = stub_->shutdown(&context, request, &response);
  if (status.IsOk())
  {
    LOG(INFO) << "RPC shutdown success";
    if (response.response_code() == 200)
    {
      return true;
    }
    else
    {
      LOG(WARNING) << "shutdown: Server returned error: " << response.response_code() << " " << response.response_description();
      return false;
    }
  }
  else
  {
    LOG(WARNING) << "RPC shutdown failed";
    return false;
  }
}

void RtcClient::reset()
{ 
  stub_.reset(); 
}
 
