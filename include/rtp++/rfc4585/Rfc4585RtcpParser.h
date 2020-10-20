#pragma once
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/experimental/RtcpGenericAck.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

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
    case PT_RTCP_GENERIC_FEEDBACK:
    case PT_RTCP_PAYLOAD_SPECIFIC:
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
      case PT_RTCP_GENERIC_FEEDBACK:
      {
        switch (uiTypeSpecific)
        {
          case TL_FB_GENERIC_NACK:
          {
            pPacket = RtcpGenericNack::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          case TL_FB_EXTENDED_NACK:
          {
            pPacket = RtcpExtendedNack::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          case TL_FB_GENERIC_ACK:
          {
            pPacket = RtcpGenericAck::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          default:
          {
            LOG_FIRST_N(WARNING, 1) << "Unhandled Generic Feedback message: " << uiTypeSpecific;
            break;
          }
        }
        break;
      }
      case PT_RTCP_PAYLOAD_SPECIFIC:
      {
        switch (uiTypeSpecific)
        {
          case FMT_APPLICATION_LAYER_FEEDBACK:
          {
            LOG(INFO) << "Application layer feedback received";
            pPacket = RtcpApplicationLayerFeedback::createRaw(uiVersion, uiPadding != 0, uiLengthInWords);
            pPacket->read(ib);
            break;
          }
          default:
          {
            LOG(WARNING) << "Payload specific RTCP FB not implemented";
          }
        }

        break;
      }
    }
    return pPacket;
  }
};

} // rfc4585
} // rtp_plus_plus
