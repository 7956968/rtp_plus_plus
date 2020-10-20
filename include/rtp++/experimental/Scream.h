#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>

/**
 * @brief Extension to signal scream FB message
 */

namespace rtp_plus_plus {
namespace experimental {

// continued from Rfc4585.h
const uint8_t TL_FB_SCREAM_JIFFY        = 12; // experimental value

// a=rtcp-fb:<payload type> scream
/// SDP attributes
extern const std::string SCREAM;

extern bool supportsScreamFb(const RtpSessionParameters& rtpParameters);

} // experimental
} // rtp_plus_plus
