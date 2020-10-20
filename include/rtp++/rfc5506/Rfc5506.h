#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus
{
namespace rfc5506
{

/// SDP attributes
extern const std::string RTCP_RSIZE;

extern bool supportsReducedRtcpSize(const RtpSessionParameters& rtpParameters);

}
}
