#pragma once
#include <string>
#include <boost/optional.hpp>
#include <PeerConnectionApi/peer_connection_api.grpc.pb.h>

// fwd
namespace grpc
{
  class ChannelInterface;
}

/**
 * @brief The RtcClient class
 */
class RtcClient
{
public:

  RtcClient(std::shared_ptr<grpc::ChannelInterface> channel);

  boost::optional<std::string> createOffer(const std::string& sContentType = "application/sdp");

  boost::optional<std::string> createAnswer(const std::string& sSessionDescription, const std::string& sContentType = "application/sdp");

  bool setRemoteDescription(const std::string& sSessionDescription, const std::string& sContentType = "application/sdp");

  bool startStreaming();

  bool stopStreaming();

  bool shutdownRemoteRtcServer();

  void reset();

private:

  std::unique_ptr<peer_connection::PeerConnectionApi::Stub> stub_;
};

