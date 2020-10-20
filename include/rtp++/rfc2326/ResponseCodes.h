#pragma once
#include <string>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief enum of RTSP response codes
 */
enum ResponseCode
{
  NOT_SET                                 = 0,
  // SUCCESS: CONTINUE
  CONTINUE                                = 100,
  // SUCCESS
  OK                                      = 200,
  //REDIRECTION
  MOVED_PERMANENTLY                       = 301,
  FOUND                                   = 302,
  SEE_OTHER                               = 303,
  NOT_MODIFIED                            = 304,
  USE_PROXY                               = 305,
  // CLIENT ERROR
  BAD_REQUEST                             = 400,
  UNAUTHORIZED                            = 401,
  PAYMENT_REQUIRED                        = 402,
  FORBIDDEN                               = 403,
  NOT_FOUND                               = 404,
  METHOD_NOT_ALLOWED                      = 405,
  NOT_ACCEPTABLE                          = 406,
  PROXY_AUTHENTICATION_REQUIRED           = 407,
  REQUEST_TIMEOUT                         = 408,
  GONE                                    = 410,
  LENGTH_REQUIRED                         = 411,
  PRECONDITION_FAILED                     = 412,
  REQUEST_MESSAGE_BODY_TOO_LARGE          = 413,
  REQUEST_URI_TOO_LONG                    = 414,
  UNSUPPORTED_MEDIA_TYPE                  = 415,
  PARAMETER_NOT_UNDERSTOOD                = 451,
  RESERVED                                = 452,
  NOT_ENOUGH_BANDWIDTH                    = 453,
  SESSION_NOT_FOUND                       = 454,
  METHOD_NOT_VALID_IN_THIS_STATE          = 455,
  HEADER_FIELD_NOT_VALID_FOR_RESOURCE     = 456,
  INVALID_RANGE                           = 457,
  PARAMETER_IS_READ_ONLY                  = 458,
  AGGREGATE_OPERATION_NOT_ALLOWED         = 459,
  ONLY_AGGREGATE_OPERATION_ALLOWED        = 460,
  UNSUPPORTED_TRANSPORT                   = 461,
  DESTINATION_UNREACHABLE                 = 462,
  DESTINATION_PROHIBITED                  = 463,
  DATA_TRANSPORT_NOT_READY_YET            = 464,
  NOTIFICATION_REASON_UNKNOWN             = 465,
  CONNECTION_AUTHORIZATION_REQUIRED       = 470,
  CONNECTION_CREDENTIALS_NOT_ACCEPTED     = 471,
  FAILURE_TO_ESTABLISH_SECURE_CONNECTION  = 472,
  // SERVER ERROR
  INTERNAL_SERVER_ERROR                   = 500,
  NOT_IMPLEMENTED                         = 501,
  BAD_GATEWAY                             = 502,
  SERVICE_UNAVAILABLE                     = 503,
  GATEWAY_TIMEOUT                         = 504,
  RTSP_VERSION_NOT_SUPPORTED              = 505,
  OPTION_NOT_SUPPORTED                    = 551
};
/**
 * @brief Converts response code to int value.
 *
 * Continue and OK are converted to 0, errors to the integer representation the error value
 */
static int responseCodeToErrorCode(ResponseCode eCode)
{
  switch(eCode)
  {
  case CONTINUE:
  case OK:
    return 0;
  default:
    return static_cast<int>(eCode);
  }
}
/**
 * @brief Returns the response code from the string representation
 */
static ResponseCode stringToResponseCode(const std::string& sCode)
{
  bool bRes(false);
  uint32_t uiCode = convert<uint32_t>(sCode, bRes);
  if (bRes)
  {
    return static_cast<ResponseCode>(uiCode);
  }
  else
  {
    return NOT_SET;
  }
}
/**
 * @brief returns the string representation of the response code.
 */
static std::string responseCodeString( ResponseCode eCode )
{
  switch(eCode)
  {
  case CONTINUE:
    return "Continue";
  case OK:
    return "OK";
  case MOVED_PERMANENTLY:
    return "Moved Permanently";
  case FOUND:
    return "Found";
  case SEE_OTHER:
    return "See Other";
  case NOT_MODIFIED:
    return "Not Modified";
  case USE_PROXY:
    return "Use Proxy";
  case BAD_REQUEST:
    return "Bad Request";
  case UNAUTHORIZED:
    return "Unauthorized";
  case PAYMENT_REQUIRED:
    return "Payment Required";
  case FORBIDDEN:
    return "Forbidden";
  case NOT_FOUND:
    return "Not Found";
  case METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case NOT_ACCEPTABLE:
    return "Not Acceptable";
  case PROXY_AUTHENTICATION_REQUIRED:
    return "Proxy Authentication Required";
  case REQUEST_TIMEOUT:
    return "Request Timeout";
  case GONE:
    return "Gone";
  case LENGTH_REQUIRED:
    return "Length Required";
  case PRECONDITION_FAILED:
    return "Precondition Failed";
  case REQUEST_MESSAGE_BODY_TOO_LARGE:
    return "Request Message Body Too Large";
  case REQUEST_URI_TOO_LONG:
    return "Request-URI Too Long";
  case UNSUPPORTED_MEDIA_TYPE:
    return "Unsupported Media Type";
  case PARAMETER_NOT_UNDERSTOOD:
    return "Parameter Not Understood";
  case RESERVED:
    return "Reserved";
  case NOT_ENOUGH_BANDWIDTH:
    return "Not Enough Bandwidth";
  case SESSION_NOT_FOUND:
    return "Session Not Found";
  case METHOD_NOT_VALID_IN_THIS_STATE:
    return "Method Not Valid in This State";
  case HEADER_FIELD_NOT_VALID_FOR_RESOURCE:
    return "Header Field Not Valid for Resource";
  case INVALID_RANGE:
    return "Invalid Range";
  case PARAMETER_IS_READ_ONLY:
    return "Parameter Is Read-Only";
  case AGGREGATE_OPERATION_NOT_ALLOWED:
    return "Aggregate Operation Not Allowed";
  case ONLY_AGGREGATE_OPERATION_ALLOWED:
    return "Only Aggregate Operation Allowed";
  case UNSUPPORTED_TRANSPORT:
    return "Unsupported Transport";
  case DESTINATION_UNREACHABLE:
    return "Destination Unreachable";
  case DESTINATION_PROHIBITED:
    return "Destination Prohibited";
  case DATA_TRANSPORT_NOT_READY_YET:
    return "Data Transport Not Ready Yet";
  case NOTIFICATION_REASON_UNKNOWN:
    return "Notification Reason Unknown";
  case CONNECTION_AUTHORIZATION_REQUIRED:
    return "Connection Authorization Required";
  case CONNECTION_CREDENTIALS_NOT_ACCEPTED:
    return "Connection Credentials not accepted";
  case FAILURE_TO_ESTABLISH_SECURE_CONNECTION:
    return "Failure to establish secure connection";
  case INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case NOT_IMPLEMENTED:
    return "Not Implemented";
  case BAD_GATEWAY:
    return "Bad Gateway";
  case SERVICE_UNAVAILABLE:
    return "Service Unavailable";
  case GATEWAY_TIMEOUT:
    return "Gateway Timeout";
  case RTSP_VERSION_NOT_SUPPORTED:
    return "RTSP Version Not Supported";
  case OPTION_NOT_SUPPORTED:
    return "Option not supported";
  default:
    return "Unknown response code";
  }
}

} // rfc2326
} // rtp_plus_plus
