#pragma once
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtp.h>

namespace rtp_plus_plus
{
namespace mprtp
{

class RtcpParser : public RtcpParserInterface
{
public:
  typedef std::unique_ptr<RtcpParser> ptr;

  static ptr create()
  {
    return std::unique_ptr<RtcpParser>( new RtcpParser() );
  }

  // allow subclasses to handle new RTCP report types
  virtual bool canHandleRtcpType(uint32_t uiType) const
  {
    return uiType == PT_MPRTCP;
  }

  virtual RtcpPacketBase::ptr parseRtcpPacket(IBitStream& ib, uint32_t uiVersion, uint32_t uiPadding, uint32_t uiTypeSpecific, uint32_t uiPayload, uint32_t uiLengthInWords)
  {
    switch( uiPayload )
    {
      case PT_MPRTCP:
      {
        RtcpPacketBase::ptr pPacket = MpRtcp::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        return pPacket;
        break;
      }
      default:
      {
        return RtcpPacketBase::ptr();
      }
    }
  }

private:

};

}
}
