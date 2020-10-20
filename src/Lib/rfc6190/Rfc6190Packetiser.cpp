#include "CorePch.h"
#include <rtp++/rfc6190/Rfc6190Packetiser.h>

namespace rtp_plus_plus
{
namespace rfc6190
{

Rfc6190PayloadPacketiser::ptr Rfc6190PayloadPacketiser::create()
{
  return std::unique_ptr<Rfc6190PayloadPacketiser>( new Rfc6190PayloadPacketiser() );
}

Rfc6190PayloadPacketiser::ptr Rfc6190PayloadPacketiser::create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
{
  return std::unique_ptr<Rfc6190PayloadPacketiser>( new Rfc6190PayloadPacketiser(uiMtu, uiSessionBandwidthKbps) );
}

Rfc6190PayloadPacketiser::Rfc6190PayloadPacketiser()
  :rfc6184::Rfc6184Packetiser()
{

}

Rfc6190PayloadPacketiser::Rfc6190PayloadPacketiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps)
  : rfc6184::Rfc6184Packetiser(uiMtu, uiSessionBandwidthKbps)
{

}

}
}
