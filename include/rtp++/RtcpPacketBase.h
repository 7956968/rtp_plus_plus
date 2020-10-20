#pragma once
#include <cstdint>
#include <numeric>
#include <ostream>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3550/SdesInformation.h>

/// fwd
class IBitStream;
/// fwd
class OBitStream;

namespace rtp_plus_plus
{

/**
 * @brief Base class for Rtcp packets. This class contains the common header fields.
 * This class has tight coupling with the IBitStream and OBitStream classes for the 
 * purpose of reading and writing headers to a bitstream.
 * 
 * It is the responsibility of the derived classes to make sure that the members 
 * have the correct value. E.G m_uiLength must represent the length of the RTCP
 * packet in words excluding the common header.
 * 
 * Specialisation of the bitstream operations is achieved by overriding 
 *  virtual void writeTypeSpecific(OBitStream& ob) const = 0;
 *  virtual void readTypeSpecific(IBitStream& ib) = 0;
 *
 */
class RtcpPacketBase
{
  friend OBitStream& operator<< (OBitStream& stream, const RtcpPacketBase& rtcpPacket);
  friend OBitStream& operator<< (OBitStream& stream, const RtcpPacketBase* pRtcpPacket);

public:
  typedef boost::shared_ptr<RtcpPacketBase> ptr;
  /**
   * @brief Destructor
   */
  virtual ~RtcpPacketBase(){}
  /**
   * @brief getVersion Getter for version
   * @return
   */
  uint8_t getVersion() const { return m_uiVersion; }
  /**
   * @brief getPadding Getter for padding
   * @return true if there is padding, false otherwise
   */
  bool getPadding() const { return m_bPadding; }
  /**
   * @brief setPadding Setter for padding.
   * @param val
   */
  void setPadding(bool val) { m_bPadding = val; }
  /**
   * @brief getTypeSpecific Getter for type-specific field
   * @return
   */
  uint8_t getTypeSpecific() const { return m_uiTypeSpecific; }
  /**
   * @brief setTypeSpecific Setter for type-specific field
   */
  void setTypeSpecific(uint8_t val) { m_uiTypeSpecific = val; }
  /**
   * @brief getPacketType Getter for packet type
   * @return
   */
  uint8_t getPacketType() const { return m_uiPacketType; }
  /**
   * @brief setPacketType Setter for packet type
   * @param uiPayloadType
   */
  void setPacketType(uint8_t uiPayloadType) { m_uiPacketType = uiPayloadType; }
  /**
   * @brief getLength Getter for length in words excluding common header
   * @return
   */
  uint16_t getLength() const { return m_uiLength; }
  /**
   * @brief setLength Setter for length in words excluding common header
   * @param val
   */
  void setLength(uint16_t val) { m_uiLength = val; }

protected:
  /**
   * @brief RtcpPacketBase
   * @param uiPacketType
   */
  RtcpPacketBase(uint8_t uiPacketType);
  /**
   * @brief RtcpPacketBase
   * @param uiVersion
   * @param uiPadding
   * @param uiTypeSpecific
   * @param uiPayload
   * @param uiLengthInWords
   */
  RtcpPacketBase(uint8_t uiVersion, bool uiPadding, uint8_t uiTypeSpecific, uint8_t uiPayload, uint16_t uiLengthInWords);

public:
  /**
   * @brief read
   * @param ib
   */
  void read(IBitStream& ib);
  /**
   * @brief getArrivalTimeNtp
   * @return
   */
  uint64_t getArrivalTimeNtp() const { return m_uiArrivalTimeNtp; }
  /**
   * @brief setArrivalTimeNtp
   * @param val
   */
  void setArrivalTimeNtp(uint64_t val) { m_uiArrivalTimeNtp = val; }

private:
  void write(OBitStream& ob) const;

  virtual void writeTypeSpecific(OBitStream& ob) const = 0;
  virtual void readTypeSpecific(IBitStream& ib) = 0;
  // TODO: perhaps add virtual getRequiredPadding method that returns the amount of padding required with the current data

private:
  uint8_t m_uiVersion;
  bool m_bPadding;
  uint8_t m_uiTypeSpecific;
  uint8_t m_uiPacketType;
  uint16_t m_uiLength;	///< The length of the header in words excluding the common header
  uint64_t m_uiArrivalTimeNtp;
};


std::ostream& operator<< (std::ostream& ostr, const RtcpPacketBase& rtcpPacket);
OBitStream& operator<< (OBitStream& stream, const RtcpPacketBase& rtcpPacket);
OBitStream& operator<< (OBitStream& stream, const RtcpPacketBase* pRtcpPacket);

typedef std::vector<RtcpPacketBase::ptr> CompoundRtcpPacket;

static uint32_t compoundRtcpPacketSize(CompoundRtcpPacket compoundPacket)
{
  uint32_t uiSizeInWords = std::accumulate(compoundPacket.begin(), compoundPacket.end(), 0, [](int iSum, RtcpPacketBase::ptr pPacket)
  {
    return iSum + ( (1 /* common RTCP header */ + pPacket->getLength() /* length of additional header in words*/));
  });
  return uiSizeInWords << 2;
}

}
