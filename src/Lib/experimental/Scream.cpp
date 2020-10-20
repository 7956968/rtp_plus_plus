#include "CorePch.h"
#include <rtp++/experimental/Scream.h>

namespace rtp_plus_plus
{
namespace experimental
{

const std::string SCREAM = "scream";

bool supportsScreamFb(const RtpSessionParameters& rtpParameters)
{
  return rtpParameters.supportsFeedbackMessage(SCREAM);
}

} // experimental
} // rtp_plus_plus
