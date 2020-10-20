#pragma once
#include <cstdint>
#include <string>

namespace rtp_plus_plus
{
class RtpSessionParameters;

namespace rfc4585
{

const uint8_t PT_RTCP_GENERIC_FEEDBACK = 205;
const uint8_t PT_RTCP_PAYLOAD_SPECIFIC = 206;

const uint8_t TL_FB_UNASSIGNED         = 0;
const uint8_t TL_FB_GENERIC_NACK       = 1;
const uint8_t TL_FB_EXTENDED_NACK      = 10; // experimental value
const uint8_t TL_FB_GENERIC_ACK        = 11; // experimental value

const uint8_t FMT_APPLICATION_LAYER_FEEDBACK = 15;

/// length measured in 32-bit words
extern const unsigned BASIC_FB_LENGTH;

/// SDP attributes
extern const std::string RTCP_FB;
extern const std::string ACK;
extern const std::string NACK;

/// RTP profile
extern const std::string AVPF;
extern const std::string RTP_AVPF;

extern bool isAudioVisualProfileWithFeedback(const RtpSessionParameters& rtpParameters);

} // rfc4585
} // rtp_plus_plus
