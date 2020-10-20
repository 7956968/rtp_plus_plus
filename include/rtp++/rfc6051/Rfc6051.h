#pragma once
#include <boost/optional.hpp>
#include <cpputil/BitReader.h>
#include <cpputil/BitWriter.h>
#include <cpputil/IBitStream.h>
#include <rtp++/rfc5285/HeaderExtensionElement.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc6051
{

static const int RTP_SYNC_HEADER_EXT_64_LENGTH = 8;
static const int RTP_SYNC_HEADER_EXT_56_LENGTH = 7;

// URNS for SDP
extern const std::string EXTENSION_NAME_RTP_NTP_64;
extern const std::string EXTENSION_NAME_RTP_NTP_56;

/**
 * @brief This class implements the https://tools.ietf.org/html/rfc6051 extension header.
 *
 * @TODO: we only support the 64-bit extension header for now
 */
class RtpSynchronisationExtensionHeader
{
public:
  /**
   * @brief create
   * @param uiRtpSyncExtMapId
   * @param uiNtpTimestampMsw
   * @param uiNtpTimestampLsw
   * @return
   */
  static rfc5285::HeaderExtensionElement create(uint32_t uiRtpSyncExtMapId, uint32_t uiNtpTimestampMsw, uint32_t uiNtpTimestampLsw);
  /**
   * @brief create
   * @param uiRtpSyncExtMapId
   * @param uiNtpTimestamp
   * @return
   */
  static rfc5285::HeaderExtensionElement create(uint32_t uiRtpSyncExtMapId, uint64_t uiNtpTimestamp);
  /**
   * @brief parseRtpSynchronisationExtensionHeader
   * @param headerExtension
   * @return
   */
  static boost::optional<uint64_t> parseRtpSynchronisationExtensionHeader(const rfc5285::HeaderExtensionElement& headerExtension);
};

} // rfc6051
} // rtp_plus_plus
