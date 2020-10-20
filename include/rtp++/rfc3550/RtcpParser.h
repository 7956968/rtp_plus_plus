#pragma once
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/rfc3550/Rtcp.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

class RtcpParser : public RtcpParserInterface
{
public:

  virtual bool canHandleRtcpType(uint32_t uiType) const
  {
    switch (uiType)
    {
    case PT_RTCP_SR:
    case PT_RTCP_RR:
    case PT_RTCP_SDES:
    case PT_RTCP_BYE:
    case PT_RTCP_APP:
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
    case PT_RTCP_SR:
    {
        VLOG(10) << "Parsing Rtcp SR";
        pPacket = RtcpSr::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        VLOG(10) << "Parsing Rtcp SR - done";
        break;
    }
    case PT_RTCP_RR:
    {
        VLOG(10) << "Parsing Rtcp RR";
        pPacket = RtcpRr::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        VLOG(10) << "Parsing Rtcp RR - done";
        break;
    }
    case PT_RTCP_SDES:
    {
        VLOG(10) << "Parsing Rtcp SDES";
        pPacket = RtcpSdes::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        VLOG(10) << "Parsing Rtcp SDES - done";
        break;
    }
    case PT_RTCP_BYE:
    {
        VLOG(10) << "Parsing Rtcp BYE";
        pPacket = RtcpBye::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        VLOG(10) << "Parsing Rtcp BYE - done";
        break;
    }
    case PT_RTCP_APP:
    {
        VLOG(10) << "Parsing Rtcp APP";
        pPacket = RtcpApp::create(uiVersion, uiPadding != 0, uiTypeSpecific, uiLengthInWords);
        pPacket->read(ib);
        VLOG(10) << "Parsing Rtcp APP - done";
        break;
    }
    }
    return pPacket;
  }
};

}
}

