#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <boost/optional.hpp>
#include <cpputil/Buffer.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/NetworkPacket.h>
#include <rtp++/rfc3550/Rfc3550RtcpValidator.h>
#include <rtp++/rfc3550/RtpHeader.h>

/// fwd
class IBitStream;

namespace rtp_plus_plus
{

/**
 * @brief The RtpPacketiser class
 * Implementation of RFC3550 RTP/RTCP packetisation
 * - RTP extension header parsers can be registered by calling
 * registerExtensionHeaderParser
 * - This class can be subclasses to handle other types of RTCP reports
 * by overriding isAdditionalTypeSupported and parseRtcpPacket
 * - The RTCP validation rules of 3550 can be changed by overriding
 * validateCompoundRtcpPacket: class Rfc5506Packetiser does this
 * TODO: need to separate parsing and validation rules
 */
class RtpPacketiser
{
public:
  /**
   * @brief RtpPacketiser Constructor
   */
  RtpPacketiser();
  /**
   * @brief RtpPacketiser Constructor
   */
  RtpPacketiser(std::unique_ptr<rfc3550::RtcpValidator> pValidator);
  /**
   * @brief RtpPacketiser Destructor
   */
  virtual ~RtpPacketiser();
  /**
   * @brief setValidate
   * @param bValidate if the packetiser should validate incoming RTCP packets
   */
  void setValidateRtcp(bool bValidate){ m_bValidate = bValidate; }
  /**
   * @brief setRtcpValidator Setter for RTCP validator
   * @param pValidator
   */
  void setRtcpValidator(std::unique_ptr<rfc3550::RtcpValidator> pValidator) { m_pValidator = std::move(pValidator); }
  /**
   * @brief registerRtcpParser
   * @param pParser
   */
  void registerRtcpParser(std::unique_ptr<RtcpParserInterface> pParser)
  {
    m_vRtcpParsers.push_back(std::move(pParser));
  }
  /**
   * @brief validate
   * @param rtcpPackets
   * @return
   */
  bool validate(const CompoundRtcpPacket& rtcpPackets);
  /**
   * @fn  Buffer RtpPacketiser::packetise(const RtpPacket& rtpPacket);
   *
   * @brief Packetises the passed in RTP packet.
   *
   * @param rtpPacket RTP packet.
   *
   * @return a buffer containing the packetised RTP packet.
   */
  static Buffer packetise(const RtpPacket& rtpPacket);
  /**
   * @brief packetise
   * @param rtpPacket
   * @param uiPreferredBufferSize
   * @return
   */
  static Buffer packetise(const RtpPacket& rtpPacket, uint32_t uiPreferredBufferSize);
  /**
   * @fn  Buffer RtpPacketiser::packetise(CompoundRtcpPacket rtcpPackets);
   *
   * @brief Packetises a compound RTCP packet.
   * 				TODO: Not sure what to do here: want to allocate packets on stack, but need heap for polymorphism.
   * 				Hence smart::ptr? Perhaps a unique_ptr would work here?
   *
   * @param rtcpPackets A compound RTCP packet.
   *
   * @return a buffer containing the packetised RTCP packets.
   */
  static Buffer packetise(CompoundRtcpPacket rtcpPackets);
  /**
   * @brief packetise
   * @param rtcpPackets
   * @param uiPreferredBufferSize
   * @return
   */
  static Buffer packetise(CompoundRtcpPacket rtcpPackets, uint32_t uiPreferredBufferSize);
  /**
   * @fn  boost::optional<RtpPacket> RtpPacketiser::depacketise(Buffer buffer);
   *
   * @brief Depacketises a raw RTP packet
   *
   * @param buffer  The buffer containing the raw RTP packet.
   *
   * @return An RtpPacket if the parsing of the raw data was successful.
   */
  boost::optional<RtpPacket> depacketise(Buffer buffer);  
  /**
   * @fn  CompoundRtcpPacket RtpPacketiser::depacketiseRtcp(Buffer buffer);
   *
   * @brief Depacketises a buffer containing raw RTCP packets. 
   * 				The compound packet is validated according to the rules in RFC3550 or of the configured validator.
   * 				Subclasses can change the validation behaviour by overriding validateCompoundRtcpPacket and parseRtcpPacket
   * 				Unknown RTCP packet types are ignored.
   * @param buffer  The buffer.
   *
   * @return A compound RTCP packet containing all successfully parsed and validated RTCP packets.
   */
  CompoundRtcpPacket depacketiseRtcp(Buffer buffer);
  /**
   * @fn  boost::optional<RtpPacket> RtpPacketiser::depacketise(NetworkPacket networkPacket);
   *
   * @brief Convenience method that calls depacketise and stores the arrival time of the network packet in the RtpPacket
   *
   * @param networkPacket Packet received from the network.
   *
   * @return The RtpPacket is the parsing of the network packet was successful.
   */
  boost::optional<RtpPacket> depacketise(NetworkPacket networkPacket);
  /**
   * @fn  CompoundRtcpPacket RtpPacketiser::depacketiseRtcp(NetworkPacket networkPacket);
   *
   * @brief Convenience method that calls depacketise and stores the arrival time of the network packet in the RTCP packets
   *
   * @param networkPacket Message describing the network.
   *
   * @return A compound RTCP packet containing all successfully parsed and validated RTCP packets.
   */
  CompoundRtcpPacket depacketiseRtcp(NetworkPacket networkPacket);
  /**
   * @fn  CompoundRtcpPacket RtpPacketiser::parseRtcpPacket(IBitStream& ib);
   *
   * @brief Convenience method thatparses a single RTCP packet from an IBitStream
   *
   * @param ib The bitstream containing the RTCP packet.
   *
   * @return The parsed RTCP packet is returned on success, otherwise a null pointer is returned
   */
  RtcpPacketBase::ptr parseRtcpPacket(IBitStream& ib);

private:

  void registerDefaultRtcpParser();

  /// iterates over registered RTCP parser and returns vector containing indices of capable parsers
  /// returns empty vector if no such parser is registered
  std::vector<int> findRtcpParsers(uint32_t uiType) const;

  // convenience methods that set the arrival time
  boost::optional<RtpPacket> doDepacketise(Buffer buffer);
  // unknown RTCP reports are ignored
  CompoundRtcpPacket doDepacketiseRtcp(Buffer buffer);

private:
  std::vector<std::unique_ptr<RtcpParserInterface> > m_vRtcpParsers;
  bool m_bValidate;
  std::unique_ptr<rfc3550::RtcpValidator> m_pValidator;
};

} // rtp_plus_plus
