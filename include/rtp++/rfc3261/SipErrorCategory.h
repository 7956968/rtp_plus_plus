#pragma once
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/ResponseCodes.h>

#ifndef BOOST_SYSTEM_NOEXCEPT
#define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT
#endif

namespace rtp_plus_plus
{
namespace rfc3261
{

  // TODO: define SIP specific errors
enum SipErrorCodes
{
   INVALID_STATE                       = 10000,
   FAILED_TO_GET_SESSION_DESCRIPTION   = 10001,
   FAILED_TO_PARSE_SESSION_DESCRIPTION = 10002,
   FAILED_TO_CREATE_MEDIA_SESSION      = 10003,
};

static std::string errorCodeToString( SipErrorCodes eCode )
{
  switch(eCode)
  {
  case INVALID_STATE:
    return "Invalid state";
  default:
    return "Unknown error";
  }
}

class SipErrorCategory : public boost::system::error_category
{
public:
#if 1
  const char *name() const BOOST_SYSTEM_NOEXCEPT { return "SipCategory"; }
#else
  const char *name() const { return "SipCategory"; }
#endif
  std::string message(int ev) const
  {
    if (ev >= NOT_SET && ev <= RESERVED_FINAL_GLOBAL)
    {
      ResponseCode eCode = (ResponseCode)ev;
      return responseCodeString(eCode);
    }
    else
    {
      return errorCodeToString((SipErrorCodes)ev);
    }
  }
};

} // rfc3261
} // rtp_plus_plus
