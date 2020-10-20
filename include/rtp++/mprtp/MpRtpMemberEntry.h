#pragma once
#include <utility>
#include <rtp++/mprtp/FlowSpecificMemberEntry.h>
#include <rtp++/mprtp/MpRtcp.h>
#include <rtp++/mprtp/MpRtpHeader.h>
#include <rtp++/rfc3550/MemberEntry.h>

namespace rtp_plus_plus
{
namespace mprtp
{

class MpRtpMemberEntry : public rfc3550::MemberEntry
{
public:
  typedef std::map<uint16_t, std::unique_ptr<FlowSpecificMemberEntry> > FlowSpecificMap_t;
  typedef std::pair<const uint16_t, std::unique_ptr<FlowSpecificMemberEntry> > FlowSpecificMapEntry_t;

  MpRtpMemberEntry();
  MpRtpMemberEntry(uint32_t uiSSRC);
  ~MpRtpMemberEntry();

  /**
    * @return If an entry exists for the specified flow ID
    */
  bool hasFlowSpecificMemberEntry(uint16_t uiFlowId) const
  {
    return m_mFlowSpecificMap.find(uiFlowId) != m_mFlowSpecificMap.end();
  }

  std::unique_ptr<FlowSpecificMemberEntry>& getFlowSpecificMemberEntry(uint16_t uiFlowId)
  {
    return m_mFlowSpecificMap.at(uiFlowId);
  }

  // override this to initialise flow specific sequence numbers
  virtual void init(const RtpPacket& rtpPacket);

  void onMpRtcp( const MpRtcp& mprtcp );

  virtual void finaliseRRData();

  /// A receiver should call this method so that we can update the state according to the
  /// RTCP packet received
  virtual void doReceiveRtpPacket(const RtpPacket& rtpPacket);

  void onMemberUpdate(const MemberUpdate& memberUpdate)
  {
    if (m_onUpdate) m_onUpdate(memberUpdate);
  }

private:
  std::unique_ptr<FlowSpecificMemberEntry> createFlowSpecificMemberEntry(uint16_t uiFlowId);

  FlowSpecificMap_t m_mFlowSpecificMap;
};

}
}
