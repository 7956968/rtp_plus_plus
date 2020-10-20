#pragma once
#include <cstdint>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief enum of SIP response codes
 */
enum ResponseCode
{
  NOT_SET = 0,
  // PROVISIONAL
  TRYING = 100,
  RINGING = 180,
  CALL_IS_BEING_FORWARDED = 181,
  QUEUED = 182,
  SESSION_PROGRESS = 183,
  // SUCCESS
  OK = 200,
  //REDIRECTION
  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  MOVED_TEMPORARILY = 302,
  USE_PROXY = 305,
  ALTERNATIVE_SERVICE = 380,
  // CLIENT ERROR
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  PAYMENT_REQUIRED = 402,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  PROXY_AUTHENTICATION_REQUIRED = 407,
  REQUEST_TIMEOUT = 408,
  GONE = 410,
  REQUEST_ENTITY_TOO_LARGE = 413,
  REQUEST_URI_TOO_LONG = 414,
  UNSUPPORTED_MEDIA_TYPE = 415,
  UNSUPPORTED_URI_SCHEME = 416,
  BAD_EXTENSION = 420,
  EXTENSION_REQUIRED = 421,
  INTERVAL_TOO_BRIEF = 423,
  TEMPORARILY_UNAVAILABLE = 480,
  CALL_TRANSACTION_DOES_NOT_EXIST = 481,
  LOOP_DETECTED = 482,
  TOO_MAY_HOPS = 483,
  ADDRESS_INCOMPLETE = 484,
  AMBIGUOUS = 485,
  BUSY_HERE = 486,
  REQUEST_TERMINATED = 487,
  NOT_ACCEPTABLE_HERE = 488,
  REQUEST_PENDING = 491,
  UNDECIPHERABLE = 493,
  // SERVER ERROR
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  SERVER_TIMEOUT = 504,
  VERSION_NOT_SUPPORTED = 505,
  MESSAGE_TOO_LARGE = 513,
  // GLOBAL FAILURES
  BUSY_EVERYWHERE = 600,
  DECLINE = 603,
  DOES_NOT_EXIST_ANWHERE = 604,
  NOT_ACCEPTABLE2 = 606,
  RESERVED_FINAL_GLOBAL = 699
};
/**
 * @brief Determines if the response is final
 */
static bool isResponseFinal(ResponseCode eCode)
{
  return eCode >= OK;
}
/**
 * @brief Determines if the response is provisional
 */
static bool isResponseProvisional(ResponseCode eCode)
{
  return eCode >= TRYING && eCode < OK;
}
/**
 * @brief Determines if a response is a redirection
 */
static bool isResponseRedirectional(ResponseCode eCode)
{
  return eCode >= MULTIPLE_CHOICES && eCode < BAD_REQUEST;
}
/**
 * @brief Determines if the response indicates success
 */
static bool isResponseSuccessfull(ResponseCode eCode)
{
  return eCode >= OK && eCode < MULTIPLE_CHOICES;
}
/**
 * @brief Determines if the response indicates success
 */
static bool isFinalNonSuccessfulResponse(ResponseCode eCode)
{
  return eCode >= MULTIPLE_CHOICES && eCode < RESERVED_FINAL_GLOBAL;
}
/**
 * @brief Converts response code to int value.
 *
 * Provisional and successful codes are converted to 0, errors to the integer representation the error value
 */
static int responseCodeToErrorCode(ResponseCode eCode)
{
  switch (eCode)
  {
  case TRYING:
  case RINGING:
  case CALL_IS_BEING_FORWARDED:
  case QUEUED:
  case SESSION_PROGRESS:
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
static std::string responseCodeString(ResponseCode eCode)
{
  switch (eCode)
  {
  case TRYING:
    return "Trying";
  case RINGING:
    return "Ringing";
  case CALL_IS_BEING_FORWARDED:
    return "Call is being forwarded";
  case QUEUED:
    return "Queued";
  case SESSION_PROGRESS:
    return "Session Progress";
  case OK:
    return "OK";
  case MULTIPLE_CHOICES:
    return "Multiple Choices";
  case MOVED_PERMANENTLY:
    return "Moved Permanently";
  case MOVED_TEMPORARILY:
    return "Moved Temporarily";
  case USE_PROXY:
    return "Use Proxy";
  case ALTERNATIVE_SERVICE:
    return "Alternative Service";
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
  case REQUEST_ENTITY_TOO_LARGE:
    return "Request Entity Too Large";
  case REQUEST_URI_TOO_LONG:
    return "Request Uri Too Long";
  case UNSUPPORTED_MEDIA_TYPE:
    return "Unsupported Media Type";
  case UNSUPPORTED_URI_SCHEME:
    return "Unsupported Uri Scheme";
  case BAD_EXTENSION:
    return "Bad Extension";
  case EXTENSION_REQUIRED:
    return "Extension Required";
  case INTERVAL_TOO_BRIEF:
    return "Interval Too Brief";
  case TEMPORARILY_UNAVAILABLE:
    return "Temporarily Unavailable";
  case CALL_TRANSACTION_DOES_NOT_EXIST:
    return "Call Transaction Does Not Exist";
  case LOOP_DETECTED:
    return "Loop Detected";
  case TOO_MAY_HOPS:
    return "Too May Hops";
  case ADDRESS_INCOMPLETE:
    return "Address Incomplete";
  case AMBIGUOUS:
    return "Ambiguous";
  case BUSY_HERE:
    return "Busy Here";
  case REQUEST_TERMINATED:
    return "Request Terminated";
  case NOT_ACCEPTABLE_HERE:
    return "Not Acceptable Here";
  case REQUEST_PENDING:
    return "Request Pending";
  case UNDECIPHERABLE:
    return "Undecipherable";
  case INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case NOT_IMPLEMENTED:
    return "Not Implemented";
  case BAD_GATEWAY:
    return "Bad Gateway";
  case SERVICE_UNAVAILABLE:
    return "Service Unavailable";
  case SERVER_TIMEOUT:
    return "Server Timeout";
  case VERSION_NOT_SUPPORTED:
    return "Version Not Supported";
  case MESSAGE_TOO_LARGE:
    return "Message Too Large";
  case BUSY_EVERYWHERE:
    return "Busy Everywhere";
  case DECLINE:
    return "Decline";
  case DOES_NOT_EXIST_ANWHERE:
    return "Does Not Exist Anwhere";
  case NOT_ACCEPTABLE2:
    return "Not Acceptable";
  default:
    return "Unknown response code";
  }
}

} // rfc3261
} // rtp_plus_plus
