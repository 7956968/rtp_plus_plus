#pragma once
#include <string>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus
{
namespace rfc3711
{

extern const std::string SAVP;
extern const std::string SAVPF;

extern bool isSecureProfile(const RtpSessionParameters& rtpParameters);

} // rfc3711
} // rtp_plus_plus

