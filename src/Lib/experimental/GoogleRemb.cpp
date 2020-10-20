#include "CorePch.h"
#include <rtp++/experimental/GoogleRemb.h>

namespace rtp_plus_plus
{
namespace experimental
{

const std::string GOOG_REMB = "goog-remb";

bool supportsReceiverEstimatedMaximumBitrate(const RtpSessionParameters& rtpParameters)
{
  return rtpParameters.supportsFeedbackMessage(GOOG_REMB);
}

} // experimental
} // rtp_plus_plus
