#include "CorePch.h"
#include <rtp++/rfc3711/Rfc3711.h>

namespace rtp_plus_plus
{
namespace rfc3711
{

const std::string SAVP = "SAVP";
const std::string SAVPF = "SAVPF";

bool isSecureProfile(const RtpSessionParameters& rtpParameters)
{
  return (rtpParameters.getProfile() == rfc3711::SAVP || rtpParameters.getProfile() == rfc3711::SAVPF);
}

} // rfc3711
} // rtp_plus_plus

