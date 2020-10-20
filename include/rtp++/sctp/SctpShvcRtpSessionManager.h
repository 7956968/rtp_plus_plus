#pragma once
#include <rtp++/sctp/SctpH265RtpSessionManager.h>

namespace rtp_plus_plus
{
namespace sctp
{

class SctpShvcRtpSessionManager : public SctpH265RtpSessionManager
{
public:

  static std::unique_ptr<SctpShvcRtpSessionManager> create(boost::asio::io_service& ioService,
                                                          const GenericParameters& applicationParameters);

  SctpShvcRtpSessionManager(boost::asio::io_service& ioService,
                           const GenericParameters& applicationParameters);

  void send(const media::MediaSample& mediaSample);
  void send(const std::vector<media::MediaSample>& vMediaSamples);
};

}
}
