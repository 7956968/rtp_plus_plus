#include "CorePch.h"
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc4566
{

std::string origin(const std::string& sUser, const std::string& sIp)
{
  std::ostringstream ostr;
  ostr << sUser << " " << RtpTime::getNTPTimeStamp() << " " << RtpTime::getNTPTimeStamp() << " IN IP4 " << sIp;
  return ostr.str();
}

const std::string APPLICATION_SDP = "application/sdp";

const std::string RTPMAP = "rtpmap";
const std::string FMTP = "fmtp";
const std::string CONTROL = "control";
const std::string VIDEO = "video";
const std::string AUDIO = "audio";
const std::string TEXT = "text";
const std::string APPLICATION = "application";
const std::string MESSAGE = "message";
const std::string SENDONLY = "sendonly";
const std::string RECVONLY = "recvonly";
const std::string SENDRECV = "sendrecv";
const std::string INACTIVE = "inactive";

} // rfc4566
} // rtp_plus_plus
