#pragma once
#include <algorithm>
#include <vector>
#include <boost/make_shared.hpp>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus{
namespace rfc4585 {
//namespace experimental {

class RtcpGenericAck : public rfc4585::RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpGenericAck> ptr;

  // default named constructor
  static ptr create()
  {
    return boost::make_shared<RtcpGenericAck>();
  }

  static ptr create(uint16_t uiSN, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericAck>(uiSN, uiSenderSSRC, uiSourceSSRC);
  }

  static ptr create(const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericAck>(vSNs, uiSenderSSRC, uiSourceSSRC);
  }

  static ptr create(const std::vector<uint32_t>& vSNs, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericAck>(vSNs, uiSenderSSRC, uiSourceSSRC);
  }

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
  {
    return boost::make_shared<RtcpGenericAck>(uiVersion, uiPadding, uiLengthInWords);
  }

  RtcpGenericAck()
    :RtcpFb(TL_FB_GENERIC_ACK, PT_RTCP_GENERIC_FEEDBACK)
  {
    setLength(BASIC_FB_LENGTH);
  }

  RtcpGenericAck(uint16_t uiSN, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_ACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addAck(uiSN);
  }

  RtcpGenericAck(const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_ACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addAcks(vSNs);
  }

  RtcpGenericAck(const std::vector<uint32_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_ACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addAcks(vSNs);
  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpGenericAck(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
    :RtcpFb(uiVersion, uiPadding, TL_FB_GENERIC_ACK, PT_RTCP_GENERIC_FEEDBACK, uiLengthInWords)
  {

  }

  const std::vector<uint16_t>& getAcks() const { return m_vAcks; }
  const std::vector<uint16_t>& getBases() const { return m_vBase; }
  const std::vector<uint16_t>& getMasks() const { return m_vMasks; }

  uint32_t calculateLength()
  {
    setMasks();
    assert(m_vBase.size() == m_vMasks.size());
    return m_vBase.size();
  }

  void addAcks(const std::vector<uint16_t>& vAcks)
  {
    std::for_each(vAcks.begin(), vAcks.end(), [this](uint16_t uiSN)
    {
      addAck(uiSN);
    });
  }

  void addAcks(const std::vector<uint32_t>& vAcks)
  {
    std::for_each(vAcks.begin(), vAcks.end(), [this](uint32_t uiSN)
    {
      addAck(static_cast<uint16_t>(uiSN));
    });
  }

  void addAck(uint16_t uiSN)
  {
    m_vAcks.push_back(uiSN);
    std::sort(m_vAcks.begin(), m_vAcks.end());
    setMasks();
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

protected:
  void setMasks()
  {
    // check how many sets we need to cover the range
    auto rit = m_vAcks.rbegin();
    uint16_t uiLast = *rit;
    m_vBase.clear();
    m_vMasks.clear();
    m_vBase.push_back(uiLast);
    m_vMasks.push_back(0);
    for (; rit != m_vAcks.rend(); ++rit)
    {
      uint16_t uiDiff = uiLast - *rit;
      if (uiDiff > 16)
      {
        // new set
        m_vBase.push_back(*rit);
        m_vMasks.push_back(0);
        uiLast = *rit;
      }
      else
      {
        uint16_t uiBit = 0x01 << (uiDiff - 1);
        uint16_t& uiMask = m_vMasks[m_vMasks.size() - 1];
        uiMask |= uiBit;
      }
    }
  }

  virtual void writeFeedbackControlInformation(OBitStream& ob) const
  {
    // now calculate
    assert(m_vBase.size() == m_vMasks.size());
    for (size_t i = 0; i < m_vMasks.size(); ++i)
    {
      ob.write(m_vBase[i], 16);
      ob.write(m_vMasks[i], 16);
    }
  }

  virtual void readFeedbackControlInformation(IBitStream& ib)
  {
    // NB: if the length is less than 2, the following code
    // will cause the application to consume all CPU
    // before running out of memory and crashing
    assert(getLength() >= 2);
    uint32_t uiCount = getLength() - 2;
    uint16_t uiBase = 0, uiMask = 0;
    for (size_t i = 0; i < uiCount; ++i)
    {
      ib.read(uiBase, 16);
      ib.read(uiMask, 16);
      m_vBase.push_back(uiBase);
      m_vMasks.push_back(uiMask);

      // process mask
      for (int j = 15; j >= 0; --j)
      {
        if ((uiMask >> j) & 0x1)
          m_vAcks.push_back(uiBase - (j + 1));
      }
      m_vAcks.push_back(uiBase);
    }
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

private:


  // store raw sequence numbers
  std::vector<uint16_t> m_vAcks;
  // store base sequence numbers
  std::vector<uint16_t> m_vBase;
  // store masks
  std::vector<uint16_t> m_vMasks;
};

//} // experimental
} // rfc4585
} // rtp_plus_plus
