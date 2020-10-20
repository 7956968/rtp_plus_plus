#include "CorePch.h"
#include <rtp++/rfc5506/Rfc5506.h>

namespace rtp_plus_plus
{
namespace rfc5506
{

const std::string RTCP_RSIZE = "rtcp-rsize";

bool supportsReducedRtcpSize(const RtpSessionParameters& rtpParameters)
{
  return rtpParameters.hasAttribute(rfc5506::RTCP_RSIZE);
}

} // rfc5506
} // rtp_plus_plus
