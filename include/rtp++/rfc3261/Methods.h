#pragma once

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief enum of supported SIP methods
 *
 * INVITE BYE REGISTER CANCEL OPTIONS
 */
enum Method
{
  UNKNOWN,
  INVITE,
  BYE,
  REGISTER,
  CANCEL,
  OPTIONS,
  ACK
};
/**
 * @brief Returns a string representation of the SIP method
 */
static std::string methodToString(Method eMethod)
{
  switch (eMethod)
  {
  case INVITE:
    return "INVITE";
  case BYE:
    return "BYE";
  case REGISTER:
    return "REGISTER";
  case CANCEL:
    return "CANCEL";
  case OPTIONS:
    return "OPTIONS";
  case ACK:
    return "ACK";
  default:
    return "UNKNOWN";
  }
}
/**
 * @brief Returns a enum for the passed in SIP method string.
 */
static Method stringToMethod(const std::string& sMethod)
{
  if (sMethod == "INVITE")
  {
    return INVITE;
  }
  else if (sMethod == "BYE")
  {
    return BYE;
  }
  else if (sMethod == "REGISTER")
  {
    return REGISTER;
  }
  else if (sMethod == "CANCEL")
  {
    return CANCEL;
  }
  else if (sMethod == "OPTIONS")
  {
    return OPTIONS;
  }
  else if (sMethod == "ACK")
  {
    return ACK;
  }
  else
  {
    return UNKNOWN;
  }
}

} // rfc3261
} // rtp_plus_plus
