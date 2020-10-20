#pragma once

namespace rtp_plus_plus
{
namespace rfc4566
{

/**
 * @brief enum to store direction attribute in SDP.
 */
enum Direction
{
  DIR_UNSPECIFIED, // unspecified in SDP (may default to something)
  DIR_SENDONLY,
  DIR_RECVONLY,
  DIR_SENDRECV,
  DIR_INACTIVE,
  DIR_UNKNOWN // unknown value
};

/**
 * @brief generates an origin line
 */
extern std::string origin(const std::string& sUser, const std::string& sIp);

extern const std::string APPLICATION_SDP;

/// rtpmap attribute string in SDP
extern const std::string RTPMAP;
/// fmtp attribute string in SDP
extern const std::string FMTP;
/// control attribute string in SDP
extern const std::string CONTROL;

/// video string in SDP
extern const std::string VIDEO;
/// audio string in SDP
extern const std::string AUDIO;
/// text string in SDP
extern const std::string TEXT;
/// application string in SDP
extern const std::string APPLICATION;
/// message string in SDP
extern const std::string MESSAGE;

/// sendonly attribute string in SDP
extern const std::string SENDONLY;
/// recvonly attribute string in SDP
extern const std::string RECVONLY;
/// sendrecv attribute string in SDP
extern const std::string SENDRECV;
/// inactive attribute string in SDP
extern const std::string INACTIVE;

/**
 * @brief returns Direction enum based on string
 * @return Enum for direction, and returns DIR_UNKNOWN if string is not known
 */
static Direction getDirectionFromString(const std::string& sDirection)
{
  if (sDirection == SENDONLY)
    return DIR_SENDONLY;
  else if (sDirection == RECVONLY)
    return DIR_RECVONLY;
  else if (sDirection == SENDRECV)
    return DIR_SENDRECV;
  else if (sDirection == INACTIVE)
    return DIR_INACTIVE;
  return DIR_UNKNOWN;
}

} // rfc4566
} // rtp_plus_plus
