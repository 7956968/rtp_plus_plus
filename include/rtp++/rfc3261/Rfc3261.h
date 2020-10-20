#pragma once
#include <cstdint>

namespace rtp_plus_plus
{
namespace rfc3261
{

extern const uint16_t DEFAULT_SIP_PORT;
extern const uint16_t DEFAULT_SIPS_PORT;

extern const std::string SIP;
extern const std::string SIPS;

extern const std::string ACCEPT;
extern const std::string ACCEPT_ENCODING;
extern const std::string ACCEPT_LANGUAGE;
extern const std::string ALERT_INFO;
extern const std::string ALLOW;
extern const std::string AUTHENTICATION_INFO;
extern const std::string AUTHORIZATION;
extern const std::string CALL_ID;
extern const std::string CALL_INFO;
extern const std::string CONTACT;
extern const std::string CONTENT_DISPOSITION;
extern const std::string CONTENT_ENCODING;
extern const std::string CONTENT_LANGUAGE;
extern const std::string CONTENT_LENGTH;
extern const std::string CONTENT_TYPE;
extern const std::string CSEQ;
extern const std::string DATE;
extern const std::string ERROR_INFO;
extern const std::string EXPIRES;
extern const std::string FROM;
extern const std::string IN_REPLY_TO;
extern const std::string MAX_FORWARDS;
extern const std::string MIME_VERSION;
extern const std::string MIN_EXPIRES;
extern const std::string ORGANIZATION;
extern const std::string PRIORITY;
extern const std::string PROXY_AUTHENTICATE;
extern const std::string PROXY_AUTHORIZATION;
extern const std::string PROXY_REQUIRE;
extern const std::string RECORD_ROUTE;
extern const std::string REPLY_TO;
extern const std::string REQUIRE;
extern const std::string RETRY_AFTER;
extern const std::string ROUTE;
extern const std::string SERVER;
extern const std::string SUBJECT;
extern const std::string SUPPORTED;
extern const std::string TIMESTAMP;
extern const std::string TO;
extern const std::string UNSUPPORTED;
extern const std::string USER_AGENT;
extern const std::string VIA;
extern const std::string WARNING;
extern const std::string WWW_AUTHENTICATE;

extern const std::string BRANCH;
extern const std::string SENT_BY;
extern const std::string TAG;
extern const std::string MAGIC_COOKIE;

#define DEFAULT_MAX_FORWARDS 70
#define INTERVAL_T1_DEFAULT_MS 500
#define INTERVAL_T2_DEFAULT_MS 4000
#define INTERVAL_T4_DEFAULT_MS 5000
#define INTERVAL_UNRELIABLE_PROTOCOL_TIMEOUT_S 32

} // rfc3261
} // rtp_plus_plus
