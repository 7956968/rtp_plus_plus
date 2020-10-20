#include "CorePch.h"
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

RtcpExtendedNack::ptr RtcpExtendedNack::create()
{
  return boost::make_shared<RtcpExtendedNack>();
}

RtcpExtendedNack::ptr RtcpExtendedNack::create(const uint16_t uiFlowId, uint16_t uiSN, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
{
  return boost::make_shared<RtcpExtendedNack>(uiFlowId, uiSN, uiSenderSSRC, uiSourceSSRC);
}

RtcpExtendedNack::ptr RtcpExtendedNack::create(const uint16_t uiFlowId, const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
{
  return boost::make_shared<RtcpExtendedNack>(uiFlowId, vSNs, uiSenderSSRC, uiSourceSSRC);
}

// raw data constructor: this can be used when the report will parse the body itself
RtcpExtendedNack::ptr RtcpExtendedNack::createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
{
  return boost::make_shared<RtcpExtendedNack>(uiVersion, uiPadding, uiLengthInWords);
}

RtcpExtendedNack::RtcpExtendedNack()
  :RtcpFb(TL_FB_EXTENDED_NACK, rfc4585::PT_RTCP_GENERIC_FEEDBACK),
    m_uiFlowId(0)
{
//  setPadding(true);
  setLength(rfc4585::BASIC_FB_LENGTH);
}

RtcpExtendedNack::RtcpExtendedNack(const uint16_t uiFlowId, uint16_t uiSN, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
  :RtcpFb(TL_FB_EXTENDED_NACK, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC),
    m_uiFlowId(uiFlowId)
{
//  setPadding(true);
  addNack(uiSN);
}

RtcpExtendedNack::RtcpExtendedNack(const uint16_t uiFlowId, const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
  :RtcpFb(TL_FB_EXTENDED_NACK, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC),
    m_uiFlowId(uiFlowId)
{
//  setPadding(true);
  addNacks(vSNs);
}

RtcpExtendedNack::RtcpExtendedNack(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
  :RtcpFb(uiVersion, uiPadding, TL_FB_EXTENDED_NACK, rfc4585::PT_RTCP_GENERIC_FEEDBACK, uiLengthInWords),
    m_uiFlowId(0)
{

}

uint32_t RtcpExtendedNack::calculateLength()
{
  assert(m_vBase.size() == m_vMasks.size());
  return m_vBase.size() + 1; // + 1 for flow id row
}

void RtcpExtendedNack::addNacks(const std::vector<uint16_t>& vNacks)
{
  std::for_each(vNacks.begin(), vNacks.end(), [this](uint16_t uiSN)
  {
    addNack(uiSN);
  });
}

void RtcpExtendedNack::addNack(uint16_t uiSN)
{
  m_vNacks.push_back(uiSN);
  int index = getNackIndex(uiSN);
  if (index != -1)
  {
    uint32_t uiOffset = uiSN - m_vBase[index];
    uint16_t uiMask = m_vMasks[index];
    uiMask = uiMask | (0x1 << (16 - uiOffset));
    m_vMasks[index] = uiMask;
  }
  else
  {
    m_vBase.push_back(uiSN);
    m_vMasks.push_back(0);
  }
  setLength(rfc4585::BASIC_FB_LENGTH + calculateLength());
}

void RtcpExtendedNack::writeFeedbackControlInformation(OBitStream& ob) const
{
  ob.write(m_uiFlowId, 16);
  assert(m_vBase.size() == m_vMasks.size());
  for (size_t i = 0; i < m_vMasks.size(); ++i)
  {
    ob.write(m_vBase[i], 16);
    ob.write(m_vMasks[i], 16);
  }
  // padding
  ob.write(0, 16);
}

void RtcpExtendedNack::readFeedbackControlInformation(IBitStream& ib)
{
  // NB: if the length is less than 2, the following code
  // will cause the application to consume all CPU
  // before running out of memory and crashing
  assert(getLength() >= 2);

  ib.read(m_uiFlowId, 16);
  uint32_t uiCount = getLength() - 2 - 1; // -1 for flowid
  uint16_t uiBase = 0, uiMask = 0;
  for (size_t i = 0; i < uiCount; ++i)
  {
    ib.read(uiBase, 16);
    ib.read(uiMask, 16);
    m_vBase.push_back(uiBase);
    m_vMasks.push_back(uiMask);
    m_vNacks.push_back(uiBase);

    // process mask
    for (int j = 15; j >= 0; --j)
    {
      if ((uiMask >> j) & 0x1)
        m_vNacks.push_back(uiBase + (16 - j));
    }
  }
  // padding at end
  uint16_t dummy;
  ib.read(dummy, 16);
  assert(dummy == 0);
  setLength(rfc4585::BASIC_FB_LENGTH + calculateLength());
}

int RtcpExtendedNack::getNackIndex(uint16_t uiSN)
{
  for (size_t i = 0; i < m_vBase.size(); ++i)
  {
    uint16_t uiBase = m_vBase[i];
    if (uiSN >= uiBase && uiSN <= (uiBase + 16))
    {
      return i;
    }
  }
  return -1;
}

}
}
