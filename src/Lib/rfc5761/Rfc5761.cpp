#include "CorePch.h"
#include <rtp++/rfc5761/Rfc5761.h>

namespace rtp_plus_plus
{
namespace rfc5761
{

const std::string RTCP_MUX = "rtcp-mux";

bool muxRtpRtcp(const RtpSessionParameters& rtpParameters)
{
  return rtpParameters.hasAttribute(rfc5761::RTCP_MUX);
}

} // rfc5761
} // rtp_plus_plus
