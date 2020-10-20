#pragma once
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/experimental/Experimental.h>
#include <rtp++/experimental/ExperimentalRtcp.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus {
namespace experimental {

class RtcpParser : public RtcpParserInterface
{
public:
  typedef std::unique_ptr<RtcpParser> ptr;

  static ptr create()
  {
    return std::unique_ptr<RtcpParser>( new RtcpParser() );
  }

  virtual bool canHandleRtcpType(uint32_t uiType) const
  {
    switch (uiType)
    {
    case rfc4585::PT_RTCP_GENERIC_FEEDBACK: // handles scream jiffy info
      return true;
    case rfc4585::PT_RTCP_PAYLOAD_SPECIFIC: // handles REMB
      return true;
    default:
      return false;
    }
  }

  virtual RtcpPacketBase::ptr parseRtcpPacket(IBitStream& ib, uint32_t uiVersion, uint32_t uiPadding, uint32_t uiTypeSpecific, uint32_t uiPayload, uint32_t uiLengthInWords)
  {
    RtcpPacketBase::ptr pPacket;
    switch( uiPayload )
    {
      case rfc4585::PT_RTCP_GENERIC_FEEDBACK:
      {
        switch (uiTypeSpecific)
        {
          case TL_FB_SCREAM_JIFFY:
          {
            pPacket = RtcpScreamFb::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          case TL_FB_NADA:
          {
            pPacket = RtcpNadaFb::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          default:
          {
            LOG(WARNING) << "Unhandled Generic Feedback message: " << uiTypeSpecific;
            break;
          }
        }
        break;
      }
      case rfc4585::PT_RTCP_PAYLOAD_SPECIFIC:
      {
        LOG(WARNING) << "Payload specific RTCP FB not implemented";
        break;
      }
    }
    return pPacket;
  }
};

} // experimental
} // rtp_plus_plus
