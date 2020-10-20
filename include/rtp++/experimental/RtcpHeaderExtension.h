#pragma once
#include <boost/optional.hpp>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/rfc5285/HeaderExtensionElement.h>

namespace rtp_plus_plus {
namespace experimental {

extern const std::string RTCP_RTP_HEADER_EXT;
extern const std::string EXTENSION_NAME_RTCP_HEADER_EXT;

/**
 * @brief The RtcpHeaderExtension class is an experimental class that allows us to package an RTCP report
 * as an RTP header extension class.
 */
class RtcpHeaderExtension
{
public:
  /**
   * @brief create
   * @param uiRtcpExtMapId
   * @param vCompoundRtcp
   * @return
   */
  static boost::optional<rfc5285::HeaderExtensionElement> create(uint32_t uiRtcpExtMapId, const CompoundRtcpPacket& vCompoundRtcp );
  /**
   * @brief parse
   * @param headerExtension
   * @return
   */
  static CompoundRtcpPacket parse(const rfc5285::HeaderExtensionElement& headerExtension, RtpPacketiser& rtpPacketiser);

};

} // experimental
} // rtp_plus_plus
