#pragma once
#include <rtp++/rfc3550/SessionDatabase.h>

namespace rtp_plus_plus {
namespace mprtp {

class MpRtpSessionDatabase : public rfc3550::SessionDatabase
{
public:
  typedef std::unique_ptr<MpRtpSessionDatabase> ptr;

  static ptr create(RtpSessionState& rtpSessionState,
                    const RtpSessionParameters& rtpParameters)
  {
    return std::unique_ptr<MpRtpSessionDatabase>( new MpRtpSessionDatabase(rtpSessionState,
                                                                           rtpParameters) );
  }

  MpRtpSessionDatabase( RtpSessionState& rtpSessionState,
                        const RtpSessionParameters& rtpParameters );

private:
  /**
    * Override base class method create MpRtpMemberEntries
    */
  virtual rfc3550::MemberEntry* createMemberEntry(uint32_t uiSSRC);

  /**
    * Overriding base class method to handle MPRTCP reports
    */
  virtual void handleOtherRtcpPacketTypes(const RtcpPacketBase& rtcpPacket,
                                          const EndPoint& ep);
};

} // mprtp
} // rtp_plus_plus
