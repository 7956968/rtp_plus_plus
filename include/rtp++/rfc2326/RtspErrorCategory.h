#pragma once
#include <boost/system/error_code.hpp>
#include <rtp++/rfc2326/ResponseCodes.h>

#ifndef BOOST_SYSTEM_NOEXCEPT
#define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT
#endif

namespace rtp_plus_plus
{
namespace rfc2326
{

enum RtspLibErrorCodes
{
   INVALID_STATE = 10000,
   INVALID_MEDIA_SESSION,
   INVALID_MEDIA_SUBSESSION,
   ERROR_PARSING_SDP
};

static std::string errorCodeToString( RtspLibErrorCodes eCode )
{
  switch(eCode)
  {
  case INVALID_STATE:
    return "Invalid state";
  case INVALID_MEDIA_SESSION:
    return "Invalid media session";
  case INVALID_MEDIA_SUBSESSION:
    return "Invalid media subsession";
  case ERROR_PARSING_SDP:
    return "Error parsing SDP";
  default:
    return "Unknown error";
  }
}

class RtspErrorCategory : public boost::system::error_category
{
public:
#if 0
  const char *name() const BOOST_SYSTEM_NOEXCEPT { return "RtspLibCategory"; }
#else
  const char *name() const BOOST_SYSTEM_NOEXCEPT { return "RtspLibCategory"; }
#endif
  std::string message(int ev) const
  {
    if (ev >= NOT_SET && ev <= OPTION_NOT_SUPPORTED)
    {
      ResponseCode eCode = (ResponseCode)ev;
      return responseCodeString(eCode);
    }
    else
    {
      return errorCodeToString((RtspLibErrorCodes)ev);
    }
  }
};

}
}
