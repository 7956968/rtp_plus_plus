#pragma once
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/rfc3611/RtcpXr.h>

namespace rtp_plus_plus
{
namespace rfc3611
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
    case PT_RTCP_XR:
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
    case PT_RTCP_XR:
    {
        pPacket = RtcpXr::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        break;
    }
    }
    return pPacket;
  }
};

}
}
