#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus
{
namespace rfc5761
{

/// SDP attributes
extern const std::string RTCP_MUX;

extern bool muxRtpRtcp(const RtpSessionParameters& rtpParameters);

} // rfc5761
} // rtp_plus_plus
