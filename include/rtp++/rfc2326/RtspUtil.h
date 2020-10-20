#pragma once
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/Transport.h>

namespace rtp_plus_plus
{
namespace rfc2326 {

/**
 * @brief extractRtspInfo
 * @param sRtspMessage
 * @param sRequest
 * @param sRtspUri
 * @param sRtspVersion
 * @return
 */
extern bool extractRtspInfo(const std::string& sRtspMessage, std::string& sRequest,
                            std::string& sRtspUri, std::string& sRtspVersion);

extern bool getCSeq(const std::string& sRtspMessage, uint32_t& uiCSeq);

extern bool getCSeqString(const std::string& sRtspMessage, std::string& sCSeq);

extern bool getContentLength(const std::string& sRtspMessage, uint32_t& uiContentLength);

extern bool getContentLengthString(const std::string& sRtspMessage, std::string& sContentLength);

extern std::string getCurrentDateString();

extern bool getTransport(const std::string& sRtspMessage, Transport& transport);

extern std::string createErrorResponse(ResponseCode eCode, uint32_t CSeq, const std::vector<std::string>& vSupported = std::vector<std::string>(), const std::vector<std::string>& vUnsupported = std::vector<std::string>());

extern std::string createErrorResponse(ResponseCode eCode, const std::string& CSeq, const std::vector<std::string>& vSupported = std::vector<std::string>(), const std::vector<std::string>& vUnsupported = std::vector<std::string>());

} // rfc2326
} // rtp_plus_plus
