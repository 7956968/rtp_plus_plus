#include "CorePch.h"
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

//const uint8_t PT_RTCP_GENERIC_FEEDBACK = 205;
//const uint8_t PT_RTCP_PAYLOAD_SPECIFIC = 206;
//const uint8_t TL_FB_UNASSIGNED         = 0;
//const uint8_t TL_FB_GENERIC_NACK       = 1;

const unsigned BASIC_FB_LENGTH = 2;

const std::string RTCP_FB = "rtcp-fb";
const std::string ACK = "ack";
const std::string NACK = "nack";
const std::string AVPF = "AVPF";
const std::string RTP_AVPF = "RTP/AVPF";

bool isAudioVisualProfileWithFeedback(const RtpSessionParameters& rtpParameters)
{
 return rtpParameters.getProfile() == rfc4585::AVPF;
}

} // rfc4585
} // rtp_plus_plus
