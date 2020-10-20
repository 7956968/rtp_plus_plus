#pragma once

namespace rtp_plus_plus
{
namespace rfc2326
{

extern std::string CONTROL;

// namespace bis is RTSP2 draft-specific 
namespace bis
{ 

extern std::string SRC_ADDR;
extern std::string DST_ADDR;
extern std::string SETUP_RTP_RTCP_MUX;
extern std::string RTCP_MUX;

} // bis

} // rfc2326
} // rtp_plus_plus

