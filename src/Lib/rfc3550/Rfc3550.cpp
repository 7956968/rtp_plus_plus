#include "CorePch.h"
#include <rtp++/rfc3550/Rfc3550.h>
#include <boost/asio/ip/host_name.hpp>

namespace rtp_plus_plus
{
namespace rfc3550
{

const unsigned RTP_VERSION_NUMBER = 2;
const unsigned MAX_CSRC = 15;
const unsigned ONE_BYTE_EXTENSION_HEADER_TERMINAL = 0xf;
const unsigned MAX_SOURCES_PER_RTCP_RR = 31;
const unsigned MIN_RTP_HEADER_SIZE = 12;
const unsigned MIN_EXTENSION_HEADER_SIZE = 4;
const unsigned RTP_SEQ_MOD = (1<<16);
const unsigned RTP_MAX_SDES = 255;
const unsigned MAX_DROPOUT = 3000;
const unsigned MAX_MISORDER = 100;
const unsigned MIN_SEQUENTIAL = 2;

const unsigned BASIC_RR_LENGTH = 1;
const unsigned BASIC_RR_BLOCK_LENGTH = 6;
const unsigned BASIC_SR_LENGTH = 6;
const unsigned SDES_ITEM_HEADER_LENGTH = 2;

const uint32_t MINIMUM_RTCP_INTERVAL_SECONDS = 5;
const uint32_t RECOMMENDED_RTCP_BANDWIDTH_PERCENTAGE = 5;
const uint32_t TIMEOUT_MULTIPLIER = 5;

const std::string UDP = "UDP";
const std::string RTP = "RTP";
const std::string AVP = "AVP";
const std::string RTP_AVP = "RTP/AVP";
const std::string RTP_AVP_UDP = "RTP/AVP/UDP";
const std::string RTP_AVP_TCP = "RTP/AVP/TCP";

bool isAudioVisualProfile(const RtpSessionParameters& rtpParameters)
{
 return rtpParameters.getProfile() == AVP;
}

SdesInformation getLocalSdesInformation()
{
  return SdesInformation(boost::asio::ip::host_name());
}

} // rfc3550
} // rtp_plus_plus
