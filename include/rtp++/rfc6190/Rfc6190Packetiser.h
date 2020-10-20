#pragma once
#include <rtp++/rfc6184/Rfc6184Packetiser.h>

namespace rtp_plus_plus
{
namespace rfc6190
{

class Rfc6190PayloadPacketiser : public rfc6184::Rfc6184Packetiser
{
public:
  typedef std::unique_ptr<Rfc6190PayloadPacketiser> ptr;

  static ptr create();
  static ptr create(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps);

  Rfc6190PayloadPacketiser();
  Rfc6190PayloadPacketiser(const uint32_t uiMtu, const uint32_t uiSessionBandwidthKbps);

private:

};

} // rfc6190
} // rtp_plus_plus
