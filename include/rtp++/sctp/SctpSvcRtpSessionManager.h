#pragma once
#include <rtp++/sctp/SctpH264RtpSessionManager.h>

namespace rtp_plus_plus
{
namespace sctp
{

class SctpSvcRtpSessionManager : public SctpH264RtpSessionManager
{
public:

  static std::unique_ptr<SctpSvcRtpSessionManager> create(boost::asio::io_service& ioService,
                                                          const GenericParameters& applicationParameters);

  SctpSvcRtpSessionManager(boost::asio::io_service& ioService,
                           const GenericParameters& applicationParameters);

  void send(const media::MediaSample& mediaSample);
  void send(const std::vector<media::MediaSample>& vMediaSamples);

private:
  uint32_t m_uiAccessUnitIndex;
};

}
}
