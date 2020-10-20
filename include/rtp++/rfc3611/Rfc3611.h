#pragma once
#include <cstdint>
#include <string>

namespace rtp_plus_plus
{
namespace rfc3611
{

const uint8_t PT_RTCP_XR = 207;

// length measured in 32-bit words
extern const unsigned BASIC_XR_LENGTH;
// length measured in 32-bit words
extern const unsigned BASIC_XR_BLOCK_LENGTH;

/// SDP attributes
extern const std::string RTCP_XR;
extern const std::string RCVR_RTT;
extern const std::string SENDER;
extern const std::string ALL;

} // rfc3611
} // rtp_plus_plus
