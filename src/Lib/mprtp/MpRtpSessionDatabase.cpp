#include "CorePch.h"
#include <rtp++/mprtp/MpRtpSessionDatabase.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/mprtp/MpRtpMemberEntry.h>

namespace rtp_plus_plus {
namespace mprtp {

MpRtpSessionDatabase::MpRtpSessionDatabase( RtpSessionState& rtpSessionState,
                                            const RtpSessionParameters& rtpParameters )
  :rfc3550::SessionDatabase( rtpSessionState, rtpParameters )
{
}

rfc3550::MemberEntry* MpRtpSessionDatabase::createMemberEntry(uint32_t uiSSRC)
{
  rfc3550::MemberEntry* pEntry = new MpRtpMemberEntry(uiSSRC);
  return pEntry;
}

void MpRtpSessionDatabase::handleOtherRtcpPacketTypes(const RtcpPacketBase& rtcpPacket,
                                                      const EndPoint& ep)
{
#if 0
  VLOG(2) << "MpRtpSessionDatabase::handleOtherRtcpPacketTypes";
#endif
  switch (rtcpPacket.getPacketType())
  {
    case PT_MPRTCP:
    {
#if 0
      VLOG(2) << "MpRtpSessionDatabase::handleOtherRtcpPacketTypes MPRTCP";
#endif
      // need to cast packet to correct type
      const MpRtcp& mprtcp = static_cast<const MpRtcp&>(rtcpPacket);
#if 0
      VLOG(2) << "Received MPRTCP packet from " << hex(mprtcp.getSSRC());
#endif
      rfc3550::MemberEntry* pEntry = insertSSRCIfNotInSessionDb(mprtcp.getSSRC());
      // update the entry
      MpRtpMemberEntry* pMpRtpMember = static_cast<MpRtpMemberEntry*>(pEntry);

      pMpRtpMember->onMpRtcp(mprtcp);
      break;
    }
    default:
    {
      VLOG(10) << "Unknown packet type: " << rtcpPacket;
    }
  }
}

} // mprtp
} // rtp_plus_plus
