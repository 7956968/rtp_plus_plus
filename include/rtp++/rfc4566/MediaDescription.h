#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <rtp++/rfc4566/Rfc4566.h>

namespace rtp_plus_plus
{
namespace rfc4566
{

/**
 * @brief struct containing rtpmap info
 */
struct RtpMapping
{
  std::string EncodingName;
  std::string ClockRate;
  std::string EncodingParameters;

  RtpMapping();

  RtpMapping(const std::string& sEncodingName, const std::string& sClockRate, const std::string& sEncodingParameters = "");
};

/**
 * @brief struct containing fmtp info
 */
struct FormatDescription
{
  std::string fmtp;

  FormatDescription();

  FormatDescription(const std::string& sFmtp);
};

/**
 * @brief struct containing connection info or c-line in SDP.
 */
struct ConnectionData
{
  /**
   * @brief Constructor.
   */
  ConnectionData();

  /**
   * @brief Constructor with set address.
   */
  ConnectionData(const std::string& sAddress);

  /**
   * @brief returns if the address has been set.
   *
   * @return true if the address has been set, and false otherwise.
   * This means that the address is not an empty string or "0.0.0.0"
   */
  bool isSet() const;

  std::string InternetType;
  std::string AddressType;
  std::string ConnectionAddress;
  // TODO: might need to add multicast TTL
};

/**
 * @brief ostream operator for connection info
 */
std::ostream& operator<< (std::ostream& ostr, const ConnectionData& connection);

/**
 * @brief struct containing feedback description
 */
struct FeedbackDescription
{
  std::vector<std::string> FeedbackTypes;
};

/**
 * @brief The MediaDescription class represents the information stored in a m-line of an SDP.
 */
class MediaDescription
{
public:
  /**
   * @brief Constructor
   */
  MediaDescription();
  /**
   * @brief Constructor
   */
  MediaDescription(const std::string& sMediaType, uint16_t uiPort, const std::string& sProtocol);
  /**
   * @brief Returns a string in the SDP format.
   */
  std::string toString() const;
  /**
   * @brief Once a MediaDescription object has been populated, process() can be called to extract 
   * semantic information. Currently this extracts rtpmap, fmtp, rtcp-fb and rtcp-xr information
   */
  void processKnownAttributes();
  /**
   * @brief Getter for media type e.g. video or audio
   */
  std::string getMediaType() const { return MediaType; }
  /**
   * @brief Setter for media type e.g. video or audio
   */
  void setMediaType(const std::string& val){ MediaType = val; }
  /**
   * @brief Getter for port
   */
  uint16_t getPort() const { return Port; }
  /**
   * @brief Setter for port
   */
  void setPort(const uint16_t val){ Port = val; }
  /**
   * @brief Getter for protocol e.g. RTP/AVP
   */
  std::string getProtocol() const { return Protocol; }
  /**
   * @brief Setter for protocol e.g. RTP/AVP as one string
   */
  void setProtocol(const std::string& val){ Protocol = val; }
  /**
   * @brief setProtocol Sets protocol i.e. RTP/AVP (default)
   * @param sProfile AVP, AVPF, SAVP, SAVPF
   * @param sLowerLayerTransport UDP, TCP, SCTP, DCCP
   */
  void setProtocol(const std::string& sProfile, const std::string& sLowerLayerTransport);
  /**
   * @brief Getter for connection
   */
  const ConnectionData& getConnection() const { return Connection; }
  /**
   * @brief Getter for connection
   */
  ConnectionData& getConnection(){ return Connection; }
  /**
   * @brief Setter for connection
   */
  void setConnection(const ConnectionData& connection) { Connection = connection; }
  /**
   * @brief Getter for session bandwidth in kbps
   */
  uint32_t getSessionBandwidth() const { return SessionBandwidth; }
  /**
   * @brief Setter for session bandwidth in kbps
   */
  void setSessionBandwidth(uint32_t uiSessionBw) { SessionBandwidth = uiSessionBw; }
  /**
   * @brief Getter for format
   */
  std::string getFormat(uint32_t uiIndex) const { return Formats.at(uiIndex); }
  /**
   * @brief Getter for formats
   */
  std::vector<std::string>& getFormats() { return Formats;  }
  /**
   * @brief Getter for formats
   */
  const std::vector<std::string>& getFormats() const { return Formats; }
  /**
   * @brief Adds a payload type format e.g. 96
   */
  void addFormat(const std::string& sFormat);
  /**
   * @brief Adds a payload type format e.g. 96 with RTPMAP and FMTP
   */
  void addFormat(const std::string& sFormat, const RtpMapping& rtpMap, const FormatDescription& fmtp);
  void removeFormat(const std::string& sFormat);
  /**
   * @brief Returns if the media description has the specified attribute
   */
  bool hasAttribute(const std::string& sAttributeName) const;
  /**
   * @brief Returns the attribute value if the media description has the specified attribute
   */
  boost::optional<std::string> getAttributeValue(const std::string& sAttributeName) const;
  /**
   * @brief Returns a vector of the attribute values if the media description has the specified attribute
   */
  std::vector<std::string> getAttributeValues(const std::string& sAttributeName) const;
  /**
   * @brief Adds the attribute with the specified name and value to the media description
   */
  void addAttribute(const std::string& sName, const std::string& sValue);
  /**
   * @brief removes all attributes of the specified key/name
   * @return the number of attributes removed
   */
  size_t removeAttribute(const std::string& sName);
  /**
   * @brief Getter for attribute map
   * @return Returns a const reference to the attribute map
   */
  const std::multimap<std::string, std::string>& getAttributes() const { return AttributeMap; }
  /**
   * @brief Getter for attribute map
   * @return Returns a reference to the attribute map
   */
  std::multimap<std::string, std::string>& getAttributes() { return AttributeMap; }
  /**
   * @brief Returns the XR value if the media description has the specified XR type
   */
  boost::optional<std::string> getXrValue(const std::string& sAttributeName);
  /**
   * @brief Getter for fmtp
   */
  const std::map<std::string, FormatDescription>& getFormatMap() const { return FormatMap; }
  /**
   * @brief Getter for fmtp
   */
  std::map<std::string, FormatDescription>& getFormatMap() { return FormatMap; }
  /**
   * @brief Getter for fmtp
   */
  boost::optional<FormatDescription> getFmtp(const std::string& sFormat) const
  { 
    auto it = FormatMap.find(sFormat);
    if (it == FormatMap.end())
      return boost::optional<FormatDescription>();
    return boost::optional<FormatDescription>(it->second);
  }
  /**
   * @brief Getter for rtpmap
   */
  const std::map<std::string, RtpMapping>& getRtpMap() const { return RtpMap; }
  /**
   * @brief Getter for rtpmap
   */
  std::map<std::string, RtpMapping>& getRtpMap() { return RtpMap; }
  /**
   * @brief Getter for rtpmap
   */
  boost::optional<RtpMapping> getRtpMap(const std::string& sFormat) const
  { 
    auto it = RtpMap.find(sFormat);
    if (it == RtpMap.end())
      return boost::optional<RtpMapping>();
    return boost::optional<RtpMapping>(it->second);
  }
  /**
   * @brief setFeedback
   */
  void setFeedback(const std::string& sFormat, const std::string& sFeedback ) { FeedbackMap[sFormat].FeedbackTypes.push_back(sFeedback); }
  /**
   * @brief setFeedback
   */
  void setFeedback(const std::string& sFormat, const std::vector<std::string>& vFeedback )
  {
    for (auto& sFb : vFeedback)
    {
      FeedbackMap[sFormat].FeedbackTypes.push_back(sFb);
    }
  }
  /**
   * @brief Getter for feedback map
   */
  const std::map<std::string, FeedbackDescription>& getFeedbackMap() const { return FeedbackMap; }
  /**
   * @brief Getter for feedback map
   */
  std::map<std::string, FeedbackDescription>& getFeedbackMap() { return FeedbackMap; }
  /**
   * @brief Returns the map containing all XR values
   */
  std::map<std::string, std::string> getXrMap() const { return XrMap; }
  /**
   * @brief Returns if the media description has any XR values
   */
  bool hasXrValues() const { return !XrMap.empty(); }
  /**
   * @brief Looks up extension URN and returns true if it finds the URN, false otherwise.
   * If successful, the method updates the uiExtmapId parameter.
   */
  bool lookupExtmap(const std::string& sUrn, uint32_t& uiExtmapId) const;
  /**
   * @brief returns map of extmap values
   */
  std::map<uint32_t, std::string> getExtMap() const;
  /**
   * @brief Returns the clockrate of the specified format
   * @return The clockrate of the specified format and zero if the format does not exist
   */
  uint32_t getClockRate(const std::string& sFormat) const;
  /**
   * @brief Getter for the media title
   * @return The media title
   */
  std::string getMediaTitle() const { return MediaTitle; }
  /**
   * @brief Setter for the media title
   */
  void setMediaTitle(const std::string& sMediaTitle) { MediaTitle = sMediaTitle; }
  /**
   * @brief Getter for the mime type of the specified format
   * @return The mime type
   */
  std::string getMimeType(const std::string& sFormat) const;
  /**
   * @brief Getter for the mime type
   * @return The mime type of the first format
   */
  std::string getMimeType() const;
  /**
   * @brief Getter for the encoding name of the specified format
   * @return The mime type
   */
  std::string getEncodingName(const std::string& sFormat) const;
  /**
   * @brief Getter for the encoding name
   * @return The mime type of the first format
   */
  std::string getEncodingName() const;
  /**
   * @brief Getter for the MID of the media description
   * @return The MID
   */
  std::string getMid() const { return Mid; }
  /**
   * @brief Setter for the MID of the media description
   */
  void setMid(const std::string& sMid) { Mid = sMid; }
  /**
   * @brief Getter of the index of the media description in the SDP
   */
  uint32_t getIndexInSdp() const { return m_uiIndex; }
  /**
   * @brief Setter for the index of the media description in the SDP
   */
  void setIndexInSdp(uint32_t uiIndex) { m_uiIndex = uiIndex; }

private:

  /// media type of session
  std::string MediaType;
  /// port of session
  uint16_t Port;
  /// protocol/profile
  std::string Protocol;
  /// Media title
  std::string MediaTitle;
  /// connection information
  ConnectionData Connection;
  /// session bandwidth in kbps
  uint32_t SessionBandwidth; // TODO: this should be a vector for all types of bw
  /// List of available formats
  std::vector<std::string> Formats;
  /// fmtp lines per format
  std::map<std::string, FormatDescription> FormatMap;
  /// rtpmap lines per format
  std::map<std::string, RtpMapping> RtpMap;
  /// general name value attributes
  std::multimap<std::string, std::string> AttributeMap;
  /// rtcp feedback
  std::map<std::string, FeedbackDescription> FeedbackMap;
  /// rtcp XR
  std::map<std::string, std::string> XrMap;
  /// MID attribute
  std::string Mid;
  /// index in SDP
  uint32_t m_uiIndex;
};

/**
 * @brief ostream operator for MediaDescription
 */
std::ostream& operator<< (std::ostream& ostr, const MediaDescription& media);

} // rfc4566
} // rtp_plus_plus
