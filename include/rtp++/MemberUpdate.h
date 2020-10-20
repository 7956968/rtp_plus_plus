#pragma once
#include <cstdint>
#include <boost/function.hpp>

namespace rtp_plus_plus {

// forward
class MemberUpdate;
typedef boost::function<void (const MemberUpdate&)> MemberUpdateCB_t;


class PathInfo
{
public:
  PathInfo()
    :m_uiSSRC(0),
      m_uiFlowId(-1),
      m_dRtt(0),
      m_uiJitter(0),
      m_uiLost(0),
      m_uiLostFraction(0),
      m_uiExtendedHighestSNReceived(0)
  {

  }

  PathInfo(uint32_t uiSSRC)
    :m_uiSSRC(uiSSRC),
      m_uiFlowId(-1),
      m_dRtt(0),
      m_uiJitter(0),
      m_uiLost(0),
      m_uiLostFraction(0),
      m_uiExtendedHighestSNReceived(0)
  {

  }

  PathInfo(uint32_t uiSSRC, double dRtt, uint32_t uiJitter, uint32_t uiLost, uint32_t uiLostFraction, uint32_t uiExtendedHighestSNReceived)
    :m_uiSSRC(uiSSRC),
      m_uiFlowId(-1),
      m_dRtt(dRtt),
      m_uiJitter(uiJitter),
      m_uiLost(uiLost),
      m_uiLostFraction(uiLostFraction),
      m_uiExtendedHighestSNReceived(uiExtendedHighestSNReceived)
  {

  }

  PathInfo(uint32_t uiSSRC, uint16_t uiFlowId, double dRtt, uint32_t uiJitter, uint32_t uiLost, uint32_t uiLostFraction, uint32_t uiExtendedHighestSNReceived)
    :m_uiSSRC(uiSSRC),
      m_uiFlowId(uiFlowId),
      m_dRtt(dRtt),
      m_uiJitter(uiJitter),
      m_uiLost(uiLost),
      m_uiLostFraction(uiLostFraction),
      m_uiExtendedHighestSNReceived(uiExtendedHighestSNReceived)
  {

  }

  // SSRC of source
  uint32_t getSSRC() const { return m_uiSSRC; }
  // Flow ID
  int getFlowId() const { return m_uiFlowId; }
  // RTT
  double getRoundTripTime() const { return m_dRtt; }
  // RTCP jitter value
  uint32_t getJitter() const { return m_uiJitter; }
  // Total number of lost packets;
  uint32_t getLost() const { return m_uiLost; }
  // Fraction of lost packets;
  uint32_t getLostFraction() const { return m_uiLostFraction; }
  // Highest SN received
  uint32_t getExtendedHighestSNReceived() const { return m_uiExtendedHighestSNReceived;  }
  // jiffy
  void setJiffy(uint32_t uiJiffy) { m_uiJiffy = uiJiffy;  }
  // jiffy
  uint32_t getJiffy() const { return m_uiJiffy;  }

private:

  // SSRC to be used for an ID
  uint32_t m_uiSSRC;
  // Flow ID for multiple paths
  int m_uiFlowId;
  // round trip time in seconds
  double m_dRtt;
  // RTCP jitter value
  uint32_t m_uiJitter;
  // Total number of lost packets;
  uint32_t m_uiLost;
  // lost fraction
  uint32_t m_uiLostFraction;
  // highest SN received
  uint32_t m_uiExtendedHighestSNReceived;
  // jiffy for scream
  uint32_t m_uiJiffy;
};

class MemberUpdate
{
  enum EventType
  {
    EVENT_NOT_SET,
    EVENT_JOIN,
    EVENT_UPDATE,
    EVENT_LEAVE
  };

public:

  MemberUpdate()
    :m_eType(EVENT_NOT_SET)
  {

  }

  MemberUpdate(uint32_t uiSSRC, bool bIsJoin)
    :m_eType(bIsJoin ? EVENT_JOIN : EVENT_LEAVE),
      m_pathInfo(uiSSRC)
  {

  }

  MemberUpdate(uint32_t uiSSRC, double dRtt, uint32_t uiJitter, uint32_t uiLost, uint32_t uiLostFraction, uint32_t uiExtendedHighestSNReceived)
    :m_eType(EVENT_UPDATE),
      m_pathInfo(uiSSRC, dRtt, uiJitter, uiLost, uiLostFraction, uiExtendedHighestSNReceived)
  {

  }

  MemberUpdate(uint32_t uiSSRC, uint16_t uiFlowId, double dRtt, uint32_t uiJitter, uint32_t uiLost, uint32_t uiLostFraction, uint32_t uiExtendedHighestSNReceived)
    :m_eType(EVENT_UPDATE),
      m_pathInfo(uiSSRC, uiFlowId, dRtt, uiJitter, uiLost, uiLostFraction, uiExtendedHighestSNReceived)
  {

  }

  bool isMemberJoin() const { return m_eType == EVENT_JOIN; }
  bool isMemberUpdate() const { return m_eType == EVENT_UPDATE; }
  bool isMemberLeave() const { return m_eType == EVENT_LEAVE; }

  const PathInfo& getPathInfo() const { return m_pathInfo; }

  // SSRC of source
  uint32_t getSSRC() const { return m_pathInfo.getSSRC(); }
  // Flow ID
  int getFlowId() const { return m_pathInfo.getFlowId(); }
  // RTT
  double getRoundTripTime() const { return m_pathInfo.getRoundTripTime(); }
  // RTCP jitter value
  uint32_t getJitter() const { return m_pathInfo.getJitter(); }
  // Total number of lost packets;
  uint32_t getLost() const { return m_pathInfo.getLost(); }
  // Fraction of lost packets;
  uint32_t getLostFraction() const { return m_pathInfo.getLostFraction(); }
  // Highest SN received
  uint32_t getExtendedHighestSNReceived() const { return m_pathInfo.getExtendedHighestSNReceived();  }

private:

  EventType m_eType;
  PathInfo m_pathInfo;
};

static std::ostream& operator<<( std::ostream& ostr, const MemberUpdate& update )
{
  ostr << " Flow: " << update.getFlowId();
  ostr << " SSRC: " << update.getSSRC();
  ostr << " RTT: " << update.getRoundTripTime();
  ostr << " Jitter: " << update.getJitter();
  ostr << " Loss: " << update.getPathInfo().getLost();
  ostr << " LossFraction: " << update.getLostFraction();
  return ostr;
}

} // rtp_plus_plus
