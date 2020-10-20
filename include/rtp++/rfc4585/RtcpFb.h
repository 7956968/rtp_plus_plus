#pragma once
#include <algorithm>
#include <vector>
#include <boost/make_shared.hpp>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4585/Rfc4585.h>

namespace rtp_plus_plus
{
namespace rfc4585
{

class RtcpFb : public RtcpPacketBase
{
public:
  typedef boost::shared_ptr<RtcpFb> ptr;

  RtcpFb(uint8_t uiFmt, uint8_t uiPt, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
    :RtcpPacketBase(rfc3550::RTP_VERSION_NUMBER, false, uiFmt, uiPt, 0),
      m_uiSenderSSRC(uiSenderSSRC),
      m_uiSourceSSRC(uiSourceSSRC)
  {

  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpFb(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint8_t uiPayloadType, uint16_t uiLengthInWords)
    :RtcpPacketBase(uiVersion, uiPadding, uiTypeSpecific, uiPayloadType, uiLengthInWords)
  {

  }

  uint32_t getSenderSSRC() const { return m_uiSenderSSRC; }
  void setSenderSSRC(uint32_t val) { m_uiSenderSSRC = val; }

  uint32_t getSourceSSRC() const { return m_uiSourceSSRC; }
  void setSourceSSRC(uint32_t val) { m_uiSourceSSRC = val; }

protected:
  void writeTypeSpecific(OBitStream& ob) const
  {
    // write standard fields
    ob.write(m_uiSenderSSRC,                 32);
    ob.write(m_uiSourceSSRC,                 32);

    // write feedback control information
    writeFeedbackControlInformation(ob);
  }

  void readTypeSpecific(IBitStream& ib)
  {
    // read standard fields
    ib.read(m_uiSenderSSRC,                 32);
    ib.read(m_uiSourceSSRC,                 32);

    // read feedback control information
    readFeedbackControlInformation(ib);
  }

  virtual void writeFeedbackControlInformation(OBitStream& ob) const = 0;
  virtual void readFeedbackControlInformation(IBitStream& ib) = 0;

protected:
  uint32_t m_uiSenderSSRC;
  uint32_t m_uiSourceSSRC;
};

class RtcpApplicationLayerFeedback: public rfc4585::RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpApplicationLayerFeedback> ptr;

  // default named constructor
  static ptr create()
  {
    return boost::make_shared<RtcpApplicationLayerFeedback>();
  }

  static ptr create(const std::string& sAppData, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpApplicationLayerFeedback>(sAppData, uiSenderSSRC, uiSourceSSRC);
  }

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
  {
    return boost::make_shared<RtcpApplicationLayerFeedback>(uiVersion, uiPadding, uiLengthInWords);
  }

  explicit RtcpApplicationLayerFeedback()
    :RtcpFb(rfc4585::FMT_APPLICATION_LAYER_FEEDBACK, rfc4585::PT_RTCP_PAYLOAD_SPECIFIC)
  {
    setLength(rfc4585::BASIC_FB_LENGTH);
  }

  explicit RtcpApplicationLayerFeedback(const std::string& sAppData, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(rfc4585::FMT_APPLICATION_LAYER_FEEDBACK, rfc4585::PT_RTCP_PAYLOAD_SPECIFIC, uiSenderSSRC, uiSourceSSRC),
      m_sAppData(sAppData)
  {
    setLength(rfc4585::BASIC_FB_LENGTH + sAppData.length());
  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  explicit RtcpApplicationLayerFeedback(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
    :RtcpFb(uiVersion, uiPadding, rfc4585::FMT_APPLICATION_LAYER_FEEDBACK, rfc4585::PT_RTCP_PAYLOAD_SPECIFIC, uiLengthInWords)
  {

  }
  /**
   * @brief getAppData Getter for app data string
   * @return
   */
  std::string getAppData() const { return m_sAppData; }
protected:

  virtual void writeFeedbackControlInformation(OBitStream& ob) const
  {
    const uint8_t* pData = (const uint8_t*)m_sAppData.c_str();
    ob.writeBytes(pData, m_sAppData.length());
  }

  virtual void readFeedbackControlInformation(IBitStream& ib)
  {
    // NB: if the length is less than 2, the following code
    // will cause the application to consume all CPU
    // before running out of memory and crashing
    assert(getLength() > 2);
//    ib.read(uiExp, 32);
    char buffer[256];
    uint32_t uiAppDataLength = getLength() - 2;
    assert(uiAppDataLength > 0 && uiAppDataLength <= 256);
    uint8_t* pBuffer = (uint8_t*)buffer;
    ib.readBytes(pBuffer, uiAppDataLength);
    m_sAppData = std::string(buffer, uiAppDataLength);
  }

private:
  std::string m_sAppData;
};

class RtcpGenericNack : public RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpGenericNack> ptr;

  // default named constructor
  static ptr create()
  {
    return boost::make_shared<RtcpGenericNack>();
  }

  static ptr create(uint16_t uiSN, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericNack>(uiSN, uiSenderSSRC, uiSourceSSRC);
  }

  static ptr create(const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericNack>(vSNs, uiSenderSSRC, uiSourceSSRC);
  }
  /**
   * @brief create providing named constructor for 32-bit sequence numbers
   * for convenience when using extended RTP sequence numbers
   * @param vSNs
   * @return
   */
  static ptr create(const std::vector<uint32_t>& vSNs, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0)
  {
    return boost::make_shared<RtcpGenericNack>(vSNs, uiSenderSSRC, uiSourceSSRC);
  }

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
  {
    return boost::make_shared<RtcpGenericNack>(uiVersion, uiPadding, uiLengthInWords);
  }

  explicit RtcpGenericNack()
    :RtcpFb(TL_FB_GENERIC_NACK, PT_RTCP_GENERIC_FEEDBACK)
  {
    setLength(BASIC_FB_LENGTH);
  }

  explicit RtcpGenericNack(uint16_t uiSN, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_NACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addNack(uiSN);
  }

  explicit RtcpGenericNack(const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_NACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addNacks(vSNs);
  }

  explicit RtcpGenericNack(const std::vector<uint32_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC)
    :RtcpFb(TL_FB_GENERIC_NACK, PT_RTCP_GENERIC_FEEDBACK, uiSenderSSRC, uiSourceSSRC)
  {
    addNacks(vSNs);
  }

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  explicit RtcpGenericNack(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords)
    :RtcpFb(uiVersion, uiPadding, TL_FB_GENERIC_NACK, PT_RTCP_GENERIC_FEEDBACK, uiLengthInWords)
  {

  }

  std::vector<uint16_t> getNacks() const { return m_vNacks; }

  uint32_t calculateLength()
  {
    assert(m_vBase.size() == m_vMasks.size());
    return m_vBase.size();
  }

  void addNacks(const std::vector<uint16_t>& vNacks)
  {
    std::for_each(vNacks.begin(), vNacks.end(), [this](uint16_t uiSN)
    {
      addNack(uiSN);
    });
  }

  void addNacks(const std::vector<uint32_t>& vNacks)
  {
    std::for_each(vNacks.begin(), vNacks.end(), [this](uint32_t uiSN)
    {
      addNack(static_cast<uint16_t>(uiSN));
    });
  }

  void addNack(uint16_t uiSN)
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
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

protected:
  virtual void writeFeedbackControlInformation(OBitStream& ob) const
  {
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
      m_vNacks.push_back(uiBase);

      // process mask
      for (int j = 15; j >= 0; --j)
      {
        if ((uiMask >> j) & 0x1)
          m_vNacks.push_back(uiBase + (16 - j));
      }
    }
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

private:

  // Finds the existing NACK group that the SN falls into if one exists
  int getNackIndex(uint16_t uiSN)
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

  // store raw sequence numbers
  std::vector<uint16_t> m_vNacks;
  // store base sequence numbers
  std::vector<uint16_t> m_vBase;
  // store masks
  std::vector<uint16_t> m_vMasks;
};

// TODO: move into own namespace or into mprtp namespace
// To do that however, each RTCP parser needs to be registered
// and code needs to be added to check if a parser can handle an
// RTCP subtype
/**
 * @brief The RtcpExtendedNack class is similar to the generic NACK but has
 * an additional flow id field so that loss can be reported on a per flow basis
 */
class RtcpExtendedNack : public rfc4585::RtcpFb
{
public:
  typedef boost::shared_ptr<RtcpExtendedNack> ptr;

  // default named constructor
  static ptr create();
  static ptr create(const uint16_t uiFlowId, uint16_t uiSN, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0);
  static ptr create(const uint16_t uiFlowId, const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC = 0, uint32_t uiSourceSSRC = 0);

  // raw data constructor: this can be used when the report will parse the body itself
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords );

  RtcpExtendedNack();
  RtcpExtendedNack(const uint16_t uiFlowId, uint16_t uiSN, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC);
  RtcpExtendedNack(const uint16_t uiFlowId, const std::vector<uint16_t>& vSNs, uint32_t uiSenderSSRC, uint32_t uiSourceSSRC);

  // NOTE: when using this constructor e.g. when parsing raw packets: the passed in values will
  // be checked after the read method
  RtcpExtendedNack(uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords);

  uint16_t getFlowId() const { return m_uiFlowId; }
  void setFlowId(uint16_t val) { m_uiFlowId = val; }

  std::vector<uint16_t> getNacks() const { return m_vNacks; }

  uint32_t calculateLength();
  void addNacks(const std::vector<uint16_t>& vNacks);
  void addNack(uint16_t uiSN);

protected:
  virtual void writeFeedbackControlInformation(OBitStream& ob) const;
  virtual void readFeedbackControlInformation(IBitStream& ib);

private:

  // Finds the existing NACK group that the SN falls into if one exists
  int getNackIndex(uint16_t uiSN);

  uint16_t m_uiFlowId;
  // store raw sequence numbers
  std::vector<uint16_t> m_vNacks;
  // store base sequence numbers
  std::vector<uint16_t> m_vBase;
  // store masks
  std::vector<uint16_t> m_vMasks;
};

#if 0
class RtcpGenericAck : public RtcpFb
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
  static ptr createRaw( uint8_t uiVersion, bool uiPadding, uint16_t uiLengthInWords )
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

  std::vector<uint16_t> getAcks() const { return m_vAcks; }

  uint32_t calculateLength()
  {
    // check how many sets we need to cover the range
    if (m_vAcks.empty()) return 0;
    auto rit = m_vAcks.rbegin();
    uint16_t uiLast = *rit;
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
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

protected:
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
      m_vAcks.push_back(uiBase);

      // process mask
      for (int j = 15; j >= 0; --j)
      {
        if ((uiMask >> j) & 0x1)
          m_vAcks.push_back(uiBase + (16 - j));
      }
    }
    setLength(BASIC_FB_LENGTH + calculateLength());
  }

private:

  // Finds the existing NACK group that the SN falls into if one exists
  int getAckIndex(uint16_t uiSN)
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

  // store raw sequence numbers
  std::vector<uint16_t> m_vAcks;
  // store base sequence numbers
  std::vector<uint16_t> m_vBase;
  // store masks
  std::vector<uint16_t> m_vMasks;
};
#endif

} // rfc4585
} // rtp_plus_plus
