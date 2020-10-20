#include "CorePch.h"
#include <rtp++/rfc3261/Rfc3261.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

const uint16_t DEFAULT_SIP_PORT = 5060;
const uint16_t DEFAULT_SIPS_PORT = 5061;
const std::string SIP("sip");
const std::string SIPS("sips");

const std::string ACCEPT("Accept");
const std::string ACCEPT_ENCODING("Accept-Encoding");
const std::string ACCEPT_LANGUAGE("Accept-Language");
const std::string ALERT_INFO("Alert-Info");
const std::string ALLOW("Allow");
const std::string AUTHENTICATION_INFO("Authentication-Info");
const std::string AUTHORIZATION("Authorization");
const std::string CALL_ID("Call-ID");
const std::string CALL_INFO("Call-Info");
const std::string CONTACT("Contact");
const std::string CONTENT_DISPOSITION("Content-Disposition");
const std::string CONTENT_ENCODING("Content-Encoding");
const std::string CONTENT_LANGUAGE("Content-Language");
const std::string CONTENT_LENGTH("Content-Length");
const std::string CONTENT_TYPE("Content-Type");
const std::string CSEQ("CSeq");
const std::string DATE("Date");
const std::string ERROR_INFO("Error-Info");
const std::string EXPIRES("Expires");
const std::string FROM("From");
const std::string IN_REPLY_TO("In-Reply-To");
const std::string MAX_FORWARDS("Max-Forwards");
const std::string MIME_VERSION("MIME-Version");
const std::string MIN_EXPIRES("Min-Expires");
const std::string ORGANIZATION("Organization");
const std::string PRIORITY("Priority");
const std::string PROXY_AUTHENTICATE("Proxy-Authenticate");
const std::string PROXY_AUTHORIZATION("Proxy-Authorization");
const std::string PROXY_REQUIRE("Proxy-Require");
const std::string RECORD_ROUTE("Record-Route");
const std::string REPLY_TO("Reply-To");
const std::string REQUIRE("Require");
const std::string RETRY_AFTER("Retry-After");
const std::string ROUTE("Route");
const std::string SERVER("Server");
const std::string SUBJECT("Subject");
const std::string SUPPORTED("Supported");
const std::string TIMESTAMP("Timestamp");
const std::string TO("To");
const std::string UNSUPPORTED("Unsupported");
const std::string USER_AGENT("User-Agent");
const std::string VIA("Via");
const std::string WARNING("Warning");
const std::string WWW_AUTHENTICATE("WWW-Authenticate");
    
const std::string BRANCH("branch");
const std::string SENT_BY("sent-by");
const std::string TAG("tag");
const std::string MAGIC_COOKIE("z9hG4bK");

} // rfc3261
} // rtp_plus_plus
