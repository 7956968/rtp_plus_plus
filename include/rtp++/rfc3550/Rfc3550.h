#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/rfc3550/SdesInformation.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

extern const unsigned RTP_VERSION_NUMBER;
extern const unsigned MAX_CSRC;
extern const unsigned ONE_BYTE_EXTENSION_HEADER_TERMINAL;
extern const unsigned MAX_SOURCES_PER_RTCP_RR;
extern const unsigned MIN_RTP_HEADER_SIZE;
extern const unsigned MIN_EXTENSION_HEADER_SIZE;
extern const unsigned RTP_SEQ_MOD;
/// maximum text length for SDES
extern const unsigned RTP_MAX_SDES;
extern const unsigned MAX_DROPOUT;
extern const unsigned MAX_MISORDER;
/// for now two sequential RTP packets required for validation
extern const unsigned MIN_SEQUENTIAL;

/// length measured in 32-bit words
extern const unsigned BASIC_RR_LENGTH;
/// length measured in 32-bit words
extern const unsigned BASIC_RR_BLOCK_LENGTH;
/// length measured in 32-bit words
extern const unsigned BASIC_SR_LENGTH;
/// measured in octets
extern const unsigned SDES_ITEM_HEADER_LENGTH;

const uint8_t RTCP_SDES_TYPE_CNAME = 1;
const uint8_t RTCP_SDES_TYPE_NAME  = 2;
const uint8_t RTCP_SDES_TYPE_EMAIL = 3;
const uint8_t RTCP_SDES_TYPE_PHONE = 4;
const uint8_t RTCP_SDES_TYPE_LOC   = 5;
const uint8_t RTCP_SDES_TYPE_TOOL  = 6;
const uint8_t RTCP_SDES_TYPE_NOTE  = 7;
const uint8_t RTCP_SDES_TYPE_PRIV  = 8;

const uint8_t PT_RTCP_SR = 200;
const uint8_t PT_RTCP_RR = 201;
const uint8_t PT_RTCP_SDES = 202;
const uint8_t PT_RTCP_BYE = 203;
const uint8_t PT_RTCP_APP = 204;
const uint8_t PT_RTCP_RESERVED = 255;

/// Recommended in RFC3550
extern const uint32_t MINIMUM_RTCP_INTERVAL_SECONDS;
extern const uint32_t RECOMMENDED_RTCP_BANDWIDTH_PERCENTAGE;

/**
 * @summary The rfc 3550 recommended number of rtcp intervals elapsed in which no RTP or RTCP packets
 * 					have been received. Participants are marked as inactive or (deleted if non valid)
 */
extern const uint32_t TIMEOUT_MULTIPLIER;

/// RTP profile
extern const std::string UDP;
extern const std::string RTP;
extern const std::string AVP;
extern const std::string RTP_AVP;
extern const std::string RTP_AVP_UDP;
extern const std::string RTP_AVP_TCP;

extern bool isAudioVisualProfile(const RtpSessionParameters& rtpParameters);

extern SdesInformation getLocalSdesInformation();

} // rfc3550
} // rtp_plus_plus
