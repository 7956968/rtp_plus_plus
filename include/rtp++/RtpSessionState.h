#pragma once
#include <cstdint>
#include <cpputil/RandomUtil.h>
#include <cpputil/Utility.h>

namespace rtp_plus_plus
{

/**
 * @brief The RtpSessionState class stores the RTP session state information
 * such as sequence numbers, SSRCs, etc.
 */
class RtpSessionState
{
public:
  /**
   * @brief RtpSessionState Constructor
   * @param uiDefaultPayloadType
   * @param uiSSRC The SSRC can be specified in cases where the user wants to use a specific SSRC
   */
  explicit RtpSessionState(uint8_t uiDefaultPayloadType, uint32_t uiSSRC = 0, bool bIsSender = false, uint8_t uiRtxPayloadType = 0)
    :m_uiCurrentPayloadType(uiDefaultPayloadType),
      m_uiSequenceNumber(0),
      m_uiRtpTimestampBase(0),
      m_uiSSRC(uiSSRC),
      m_uiRemoteSSRC(0),
      m_bIsSender(bIsSender),
      m_uiRtxPayloadType(uiRtxPayloadType),
      m_uiRtxSSRC(0),
      m_uiRemoteRtxSSRC(0)
  {
    initialiseRandomVariables();
  }
  /**
   * @brief getSSRC Getter for SSRC
   * @return the current SSRC
   */
  uint32_t getSSRC() const { return m_uiSSRC; }
  /**
   * @brief getLocalSSRCs gets a vector containing all local SSRCs
   * @return
   */
  std::vector<uint32_t> getLocalSSRCs()
  {
    std::vector<uint32_t> vSSRCs;
    vSSRCs.push_back(m_uiSSRC);
    if (m_uiRtxPayloadType != 0)
      vSSRCs.push_back(m_uiRtxSSRC);
    return vSSRCs;
  }
  /**
   * @brief updateSSRC is used to update the SSRC of the RTP session if a collision
   * has been detected
   */
  void updateSSRC()
  {
    m_uiSSRC = RandomUtil::getInstance().rand32();
  }
  /**
   * @brief updateSSRC is used to update the SSRC of the RTP session
   * This method is useful in situations where the calling code is responsible
   * for SSRC generation
   * @param uiSSRC new SSRC
   */
  void updateSSRC(uint32_t uiSSRC) { m_uiSSRC = uiSSRC;}
  /**
   * @brief getRemoteSSRC Getter for remote SSRC
   * @return remote SSRC
   */
  uint32_t getRemoteSSRC() const{ return m_uiRemoteSSRC; }
  /**
   * @brief setRemoteSSRC Setter for remote SSRC
   * @return remote SSRC
   */
  void setRemoteSSRC(uint32_t uiSSRC) { m_uiRemoteSSRC = uiSSRC;}
  /**
   * @brief getNextSequenceNumber for senders: gets the next sequence number in the current RTP session
   * @return the sequence number of the next RTP packet
   */
  uint16_t getNextSequenceNumber() const
  {
    return m_uiSequenceNumber++;
  }
  /**
   * @brief getCurrentSequenceNumber for senders: gets the next sequence number in the current RTP session
   * without incrementing it
   * @return the sequence number of the next RTP packet
   */
  uint16_t getCurrentSequenceNumber() const
  {
    return m_uiSequenceNumber;
  }
  /**
   * @brief getRtpTimestampBase For senders: get the timestamp base for the RTP session
   * @return the timebase for the RTP session
   */
  uint32_t getRtpTimestampBase() const
  {
    return m_uiRtpTimestampBase;
  }
  /**
   * @brief getCurrentPayloadType Getter for current payload type
   * @return payload type of RTP session
   */
  uint8_t getCurrentPayloadType() const
  {
    return m_uiCurrentPayloadType;
  }
  /**
   * @brief setCurrentPayloadType Setter for payload type
   * @param uiPayloadType
   */
  void setCurrentPayloadType(uint8_t uiPayloadType)
  {
    m_uiCurrentPayloadType = uiPayloadType;
  }
  /**
   * @brief overrideRtpSequenceNumber overrides random RTP sequence number
   * @param uiSN
   */
  void overrideRtpSequenceNumber(uint16_t uiSN)
  {
    m_uiSequenceNumber = uiSN;
  }
  /**
   * @brief overrideRtpTimestampBase overrides random RTP timestamp base
   * @param uiRtpTsBase
   */
  void overrideRtpTimestampBase(uint32_t uiRtpTsBase)
  {
    m_uiRtpTimestampBase = uiRtpTsBase;
  }
  /**
   * @brief overrideSSRC overrides random RTP SSRC
   * @param uiSSRC
   */
  void overrideSSRC(uint32_t uiSSRC)
  {
    m_uiSSRC = uiSSRC;
  }
  /**
   * @brief isSender Getter for if local SSRC is a sender
   * @return returns if local SSRC is a sender
   */
  bool isSender() const { return m_bIsSender; }
  /**
   * @brief setSender Setter for if local SSRC is sender
   * @param bIsSender
   */
  void setSender(bool bIsSender) { m_bIsSender = bIsSender; }
  /**
   * @brief getRtxPayloadType Getter for RTX payload type
   * @return payload type of retransmission RTP session
   */
  uint8_t getRtxPayloadType() const
  {
    return m_uiRtxPayloadType;
  }
  /**
   * @brief setRtxPayloadType Setter for RTX payload type
   * @param uiPayloadType RTX payload type
   */
  void setRtxPayloadType(uint8_t uiPayloadType)
  {
    m_uiRtxPayloadType = uiPayloadType;
  }
  /**
   * @brief getSSRC Getter for SSRC
   * @return the current SSRC
   */
  uint32_t getRtxSSRC() const { return m_uiRtxSSRC; }
  /**
   * @brief getNextRtxSequenceNumber for senders: gets the next RTX sequence number in the current RTP session
   * @return the sequence number of the next RTX RTP packet
   */
  uint16_t getNextRtxSequenceNumber() const
  {
    return m_uiRtxSequenceNumber++;
  }
  /**
   * @brief getCurrentRtxSequenceNumber for senders: gets the current RTX sequence number in the current RTP session
   * without incrementing it
   * @return the sequence number of the current RTX RTP packet
   */
  uint16_t getCurrentRtxSequenceNumber() const
  {
    return m_uiRtxSequenceNumber;
  }

private:
  /// initialises the random variables such as SSRC
  void initialiseRandomVariables()
  {
    RandomUtil& rRandom = RandomUtil::getInstance();
    // check for explicit SSRC
    if (m_uiSSRC == 0)
      m_uiSSRC = rRandom.rand32();
    m_uiSequenceNumber = rRandom.rand16();
    m_uiRtpTimestampBase = rRandom.rand32();
    m_uiRtxSSRC = rRandom.rand32();
    VLOG(5) << "[" << this << "] RtpSessionState::initialiseRandomVariables"
            << " SSRC: " << hex(m_uiSSRC)
            << " SN: " << m_uiSequenceNumber
            << " TSB: " << m_uiRtpTimestampBase;
  }

private:
  // current payload type
  uint8_t m_uiCurrentPayloadType;
  // RTP sequence number: only important for senders
  mutable uint16_t m_uiSequenceNumber;
  // Random RTP timestamp base: only important for senders
  uint32_t m_uiRtpTimestampBase;
  // SSRC of host: this may change if there is an SSRC collision
  uint32_t m_uiSSRC;
  // SSRC of remote host: this may change if there is an SSRC collision
  uint32_t m_uiRemoteSSRC;
  // is local SSRC a sender
  bool m_bIsSender;
  // RTX enabled
  uint8_t m_uiRtxPayloadType;
  // RTX sequence number: only important for senders
  mutable uint16_t m_uiRtxSequenceNumber;
  // RTX SSRC
  uint32_t m_uiRtxSSRC;
  // remote RTX SSRC
  uint32_t m_uiRemoteRtxSSRC;
};

} // rtp_plus_plus
