#include "CorePch.h"
#include <rtp++/rfc3611/Rfc3611.h>

namespace rtp_plus_plus
{
namespace rfc3611
{

// const uint8_t PT_RTCP_XR = 207;

const unsigned BASIC_XR_LENGTH = 1;
const unsigned BASIC_XR_BLOCK_LENGTH = 1;

const std::string RTCP_XR = "rtcp-xr";
const std::string RCVR_RTT = "rcvr-rtt";
const std::string SENDER = "sender";
const std::string ALL = "all";

} // rfc3611
} // rtp_plus_plus
