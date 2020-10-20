#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>

/**
 * @brief RTCP REMB extension
 */

namespace rtp_plus_plus
{
namespace experimental
{

// a=rtcp-fb:<payload type> goog-remb
/// SDP attributes
extern const std::string GOOG_REMB;

extern bool supportsReceiverEstimatedMaximumBitrate(const RtpSessionParameters& rtpParameters);

} // experimental
} // rtp_plus_plus
