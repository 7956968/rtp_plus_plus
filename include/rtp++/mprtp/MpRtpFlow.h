#pragma once

#include <cstdint>
#include <cpputil/RandomUtil.h>
#include <rtp++/network/EndPoint.h>

namespace rtp_plus_plus
{
namespace mprtp
{

class MpRtpFlow
{
public:
  MpRtpFlow()
    :m_uiFlowId(0),
    m_uiFlowSpecificSN(0),
    m_uiRtxFlowSpecificSN(0)
  {
    RandomUtil& rRandom = RandomUtil::getInstance();
    m_uiFlowSpecificSN = rRandom.rand16();
    m_uiRtxFlowSpecificSN = rRandom.rand16();
  }

  MpRtpFlow(uint16_t uiFlowId, const EndPointPair_t& localEp, const EndPointPair_t& remoteEp)
    :m_uiFlowId(uiFlowId),
    m_uiFlowSpecificSN(0),
    m_uiRtxFlowSpecificSN(0),
    m_localEp(localEp),
    m_remoteEp(remoteEp)
  {
      RandomUtil& rRandom = RandomUtil::getInstance();
      m_uiFlowSpecificSN = rRandom.rand16();
      m_uiRtxFlowSpecificSN = rRandom.rand16();
      m_localEp.first.setId(uiFlowId);
      m_localEp.second.setId(uiFlowId);
      m_remoteEp.first.setId(uiFlowId);
      m_remoteEp.second.setId(uiFlowId);
  }

  int16_t getFlowId() const { return m_uiFlowId; }
  int16_t getNextSN() { return m_uiFlowSpecificSN++; }
  int16_t getNextRtxSN() { return m_uiRtxFlowSpecificSN++; }
  const EndPointPair_t& getLocalEndPoint() const { return m_localEp; }
  const EndPointPair_t& getRemoteEndPoint() const { return m_remoteEp; }

private:

  uint16_t m_uiFlowId;
  uint16_t m_uiFlowSpecificSN;
  uint16_t m_uiRtxFlowSpecificSN;
  EndPointPair_t m_localEp;
  EndPointPair_t m_remoteEp;
};

}
}
