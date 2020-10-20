#pragma once
#include <string>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief Enum of supported RTSP methods.
 */
enum RtspMethod
{
  UNKNOWN,
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  PLAY_NOTIFY,
  PAUSE,
  TEARDOWN,
  GET_PARAMETER,
  SET_PARAMETER,
  REDIRECT,
  REGISTER
};

/**
 * @brief Returns a string representation of the RTSP method
 */
static std::string methodToString( RtspMethod eMethod)
{
  switch(eMethod)
  {
  case OPTIONS:
    return "OPTIONS";
  case DESCRIBE:
    return "DESCRIBE";
  case SETUP:
    return "SETUP";
  case PLAY:
    return "PLAY";
  case PLAY_NOTIFY:
    return "PLAY_NOTIFY";
  case PAUSE:
    return "PAUSE";
  case TEARDOWN:
    return "TEARDOWN";
  case GET_PARAMETER:
    return "GET_PARAMETER";
  case SET_PARAMETER:
    return "SET_PARAMETER";
  case REDIRECT:
    return "REDIRECT";
    case REGISTER:
      return "REGISTER";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Returns a enum for the passed in RTSP method string.
 */
static RtspMethod stringToMethod(const std::string& sMethod)
{
  if (sMethod == "OPTIONS")
  {
    return OPTIONS;
  }
  else if (sMethod == "DESCRIBE")
  {
    return DESCRIBE;
  }
  else if (sMethod == "SETUP")
  {
    return SETUP;
  }
  else if (sMethod == "PLAY")
  {
    return PLAY;
  }
  else if (sMethod == "PLAY_NOTIFY")
  {
    return PLAY_NOTIFY;
  }
  else if (sMethod == "PAUSE")
  {
    return PAUSE;
  }
  else if (sMethod == "TEARDOWN")
  {
    return TEARDOWN;
  }
  else if (sMethod == "GET_PARAMETER")
  {
    return GET_PARAMETER;
  }
  else if (sMethod == "SET_PARAMETER")
  {
    return SET_PARAMETER;
  }
  else if (sMethod == "REDIRECT")
  {
    return REDIRECT;
  }
  else if (sMethod == "REGISTER")
  {
    return REGISTER;
  }
  else
  {
    return UNKNOWN;
  }
}

} // rfc2326
} // rtp_plus_plus
