#pragma once
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <cpputil/Conversion.h>
#include <rtp++/rfc3611/Rfc3611.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4566/SessionDescription.h>

/** 
 * @def DEBUG_SDP
 * This define logs the session and media descriptions when the method is called
 */

namespace rtp_plus_plus
{
namespace rfc4566
{

/**
 * @brief Utility class for parsing SDP.
 */
class SdpParser
{
public:
    /**
      * The parse method parses the SDP stores in sSdp and returns an optional
      * SessionDescription. If the parsing fails, it returns a null pointer.
      * @param sSdp A string containing the session description protocol
      * @return A pointer to a SessionDescription object if the parsing is successful
      * and a null pointer if it fails.
      */
  static boost::optional<SessionDescription> parse(const std::string& sSdp, bool bVerbose = false);

  static void processKnownSessionDescriptionAttributes(SessionDescription &sdp);

  static void printSessionDescriptionInfo(const SessionDescription& sdp);

};

} // rfc4566
} // rtp_plus_plus
