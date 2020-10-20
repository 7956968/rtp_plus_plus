#pragma once
#include <list>
#include <utility>
#include <cpputil/Buffer.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/optional.hpp>
#include <rtp++/mprtp/MpRtpHeader.h>
#include <rtp++/rfc3550/RtpHeader.h>

/// typedef for extended sequence number and payload pair
typedef std::pair<uint32_t, Buffer> PacketData_t;
typedef std::list<PacketData_t> PacketDataList_t;

namespace rtp_plus_plus
{

/**
 * @brief The RtpPacket class
 */
class RtpPacket
{
public:
  /**
   * @brief Constructor
   */
  RtpPacket()
    :m_uiRtpArrivalTimestamp(0),
      m_tNtpArrival(0),
      m_uiExtendedSequenceNumber(0),
      m_dOwdSeconds(-1.0),
      m_iId(-1)
  {

  }
  /**
   * @brief Constructor
   */
  RtpPacket(const rfc3550::RtpHeader& rtpHeader)
    :m_header(rtpHeader),
      m_uiRtpArrivalTimestamp(0),
      m_tNtpArrival(0),
      m_uiExtendedSequenceNumber(0),
      m_dOwdSeconds(-1.0),
      m_iId(-1)
  {

  }
  /**
   * @brief Convenience accessor for RTP timestamp
   */
  uint32_t getRtpTimestamp() const { return m_header.getRtpTimestamp(); }
  /**
   * @brief Convenience accessor for RTP sequence number
   */
  uint32_t getSequenceNumber() const { return m_header.getSequenceNumber(); }
  /**
   * @brief Convenience accessor for RTP SSRC
   */
  uint32_t getSSRC() const { return m_header.getSSRC(); }
  /**
   * @brief getExtendedSequenceNumber Getter for extended RTP sequence number:
   * This field should be used with case as it
   * will only be valid if it has been set prior!
   * @return
   */
  uint32_t getExtendedSequenceNumber() const { return m_uiExtendedSequenceNumber; }
  /**
   * @brief setExtendedSequenceNumber Setter for extended sequence number
   * @param uiSN
   */
  void setExtendedSequenceNumber(uint32_t uiSN)
  {
    m_uiExtendedSequenceNumber = uiSN;
    // update sequence number as well
    m_header.setSequenceNumber(static_cast<uint16_t>(uiSN));
  }
  /**
   * @brief getHeaderExtension is a convenience method to get the header extension
   * @return
   */
  rfc5285::RtpHeaderExtension& getHeaderExtension()
  {
    return m_header.getHeaderExtension();
  }
  /**
   * @brief getHeaderExtension is a convenience method to get the header extension
   * @return
   */
  const rfc5285::RtpHeaderExtension& getHeaderExtension() const
  {
    return m_header.getHeaderExtension();
  }
  /**
   * @brief addRtpHeaderExtension is a convenience method to add an RTP extension header
   * @param headerExtension
   */
  void addRtpHeaderExtension(const rfc5285::HeaderExtensionElement& headerExtension)
  {
    m_header.addRtpHeaderExtension(headerExtension);
  }
  /**
   * @brief getHeader Getter for RTP header
   * @return
   */
  rfc3550::RtpHeader& getHeader() { return m_header; }
  /**
   * @brief getHeader Getter for RTP header
   * @return
   */
  const rfc3550::RtpHeader& getHeader() const { return m_header; }
  /**
   * @brief setRtpHeader
   * @param rHeader
   */
  void setRtpHeader(const rfc3550::RtpHeader& rHeader) { m_header = rHeader; }
  /**
   * @brief getPayload
   * @return
   */
  const Buffer getPayload() const { return m_rtpPayload; }
  /**
   * @brief setPayload
   * @param payload
   */
  void setPayload(Buffer payload){ m_rtpPayload = payload; }
  /**
   * @brief getPayloadSize
   * @return
   */
  uint32_t getPayloadSize() const { return m_rtpPayload.getSize(); }
  /**
   * @brief getRawRtpPacketData Getter for raw packet data
   * @return
   */
  Buffer getRawRtpPacketData() const { return m_rawRtpPacketData; }
  /**
   * @brief setRawRtpPacketData Setter for raw packet data
   * @param rtpData
   */
  void setRawRtpPacketData(Buffer rtpData){ m_rawRtpPacketData = rtpData; }
  /**
   * @brief getSize Getter for size including header and payload
   * @return
   */
  uint32_t getSize() const
  {
    return m_header.getSize() + m_rtpPayload.getSize();
  }
  /**
   * @brief getArrivalTime Getter for arrival time
   * @return
   */
  boost::posix_time::ptime getArrivalTime() const { return m_tArrival; }
  /**
   * @brief setArrivalTime Setter for packet arrival time
   * @param tArrival
   */
  void setArrivalTime(const boost::posix_time::ptime& tArrival) { m_tArrival = tArrival; }
  /**
   * @brief getSendTime
   * @return
   */
  boost::posix_time::ptime getSendTime() const { return m_tSent; }
  /**
   * @brief setArrivalTime
   * @param tSent
   */
  void setSendTime(const boost::posix_time::ptime& tSent) { m_tSent = tSent; }
  /**
   * @brief getNtpArrivalTime Getter for arrival time as NTP timestamp
   * @return
   */
  uint64_t getNtpArrivalTime() const { return m_tNtpArrival; }
  /**
   * @brief setNtpArrivalTime Setter for arrival time as NTP timestamp
   * @param tArrival
   */
  void setNtpArrivalTime(uint64_t tArrival) { m_tNtpArrival = tArrival; }
  /**
   * @brief getRtpArrivalTimestamp
   * @return
   */
  uint32_t getRtpArrivalTimestamp() const { return m_uiRtpArrivalTimestamp; }
  /**
   * @brief setRtpArrivalTimestamp
   * @param val
   */
  void setRtpArrivalTimestamp(uint32_t val) { m_uiRtpArrivalTimestamp = val; }
  /**
   * @brief getId
   * @return
   */
  int32_t getId() const { return m_iId; }
  /**
   * @brief setId
   * @param iId
   */
  void setId(int32_t iId) { m_iId = iId; }
  /**
   * @brief getOwdSeconds
   * @return
   */
  double getOwdSeconds() const { return m_dOwdSeconds;  }
  /**
   * @brief setOwdSeconds
   * @param dOwdSeconds
   */
  void setOwdSeconds(double dOwdSeconds) { m_dOwdSeconds = dOwdSeconds;  }
  /**
   * @brief getSource
   * @return
   */
  const EndPoint& getSource() const { return m_source; }
  /**
   * @brief setSource
   * @param ep
   */
  void setSource(const EndPoint& ep) { m_source = ep; }
  /**
   * @brief getMpRtpSubflowHeader
   * @return
   */
  boost::optional<mprtp::MpRtpSubflowRtpHeader> getMpRtpSubflowHeader() const { return m_pMpRtpSubflow; }
  /**
   * @brief setMpRtpSubflowHeader sets the MPRTP subflow header of the RTP packet.
   * @param subflowHeader
   */
  void setMpRtpSubflowHeader(const mprtp::MpRtpSubflowRtpHeader& subflowHeader)
  {
    m_pMpRtpSubflow = boost::optional<mprtp::MpRtpSubflowRtpHeader>(subflowHeader);
  }

private:
  /// RTP header
  rfc3550::RtpHeader m_header;
  /// payload
  Buffer m_rtpPayload;
  /// arrival time ptime
  boost::posix_time::ptime m_tArrival;
  /// send time ptime
  boost::posix_time::ptime m_tSent;
  /// arrival time RTP
  uint32_t m_uiRtpArrivalTimestamp;
  /// arrival time NTP
  uint64_t m_tNtpArrival;
  /// extended RTP SN
  uint32_t m_uiExtendedSequenceNumber;
  /// OWD
  double m_dOwdSeconds;
  /// optional buffer to store the raw RTP data
  Buffer m_rawRtpPacketData;
  /// Id: to be used for miscellaneous reasons
  int32_t m_iId;
  /// optional source endpoint
  EndPoint m_source;
  /// MPRTP
  boost::optional<mprtp::MpRtpSubflowRtpHeader> m_pMpRtpSubflow;
};

} // rtp_plus_plus
