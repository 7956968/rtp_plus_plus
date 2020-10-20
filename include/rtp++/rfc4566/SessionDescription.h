#pragma once
#include <cstdint>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <rtp++/rfc4566/MediaDescription.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc5888/Rfc5888.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{
namespace rfc4566
{

/**
 * @brief The Origin struct is just a utility class for o-line creation
 */
struct Origin
{
  Origin()
    :Username("-"),
      NetType("IN"),
      AddrType("IP4"),
      UnicastAddress("0.0.0.0")
  {
    RtpTime::getNTPTimeStamp(SessionId);
    RtpTime::getNTPTimeStamp(SessionVersion);
  }

  Origin(const std::string& sUnicastAddress)
    :Username("-"),
      NetType("IN"),
      AddrType("IP4"),
      UnicastAddress(sUnicastAddress)
  {
    RtpTime::getNTPTimeStamp(SessionId);
    RtpTime::getNTPTimeStamp(SessionVersion);
  }

  std::string toString() const
  {
    std::ostringstream ostr;
    ostr << Username << " "
         << SessionId << " "
         << SessionVersion << " "
         <<  NetType << " "
         << AddrType << " "
         << UnicastAddress;
    return ostr.str();
  }

  std::string Username;
  uint64_t SessionId;
  uint64_t SessionVersion;
  std::string NetType;
  std::string AddrType;
  std::string UnicastAddress;
};

/**
 * @brief The SessionDescription class stores the information found in an SDP.
 */
class SessionDescription
{
public:
  /**
   * @brief Constructor
   */
  SessionDescription();
  /**
   * @brief Constructor that stores original SDP string
   */
  SessionDescription(const std::string& sSdp);
  /**
   * @brief This method can be called once all attributes have been set on the SDP
   */ 
  void process();
  /**
   * @brief Returns a string representation of the session description
   */
  std::string toString() const;
  /**
   * @brief Returns the original string representation of the session description if set
   */
  std::string getOriginalSdpString() const;
  /**
   * @brief Getter for protocol version
   */
  std::string getProtocolVersion() const { return ProtocolVersion; }
  /**
   * @brief Setter for protocol version
   */
  void setProtocolVersion(const std::string& sVal) { ProtocolVersion = sVal; }
  /**
   * @brief Getter for origin
   */
  std::string getOrigin() const { return Origin; }
  /**
   * @brief Setter for origin
   */
  void setOrigin(const std::string& sVal) { Origin = sVal; }
  /**
   * @brief Getter for session name
   */
  std::string getSessionName() const { return SessionName; }
  /**
   * @brief Setter for session name
   */
  void setSessionName(const std::string& sVal) { SessionName = sVal; }
  /**
   * @brief Getter for session info
   */
  std::string getSessionInfo() const { return SessionInfo; }
  /**
   * @brief Setter for session info
   */
  void setSessionInfo(const std::string& sVal) { SessionInfo = sVal; }
  /**
   * @brief Getter for URI
   */
  std::string getUri() const { return Uri; }
  /**
   * @brief Setter for URI
   */
  void setUri (const std::string& sVal) { Uri = sVal; }
  /**
   * @brief Getter for email fields
   */
  std::vector<std::string> getEmailFields() const { return EmailFields; }
  /**
   * @brief Adds an email field
   */
  void addEmailField(const std::string& sVal) { EmailFields.push_back(sVal); }
  /**
   * @brief Getter for phone numbers
   */
  std::vector<std::string> getPhoneNumbers() const { return PhoneNumbers; }
  /**
   * @brief Adds an phone number
   */
  void addPhoneNumber(const std::string& sVal) { PhoneNumbers.push_back(sVal) ; }
  /**
   * @brief Getter for connection
   */
  const ConnectionData& getConnection() const { return Connection; }
  /**
   * @brief Getter for connection
   */
  ConnectionData& getConnection() { return Connection; }
  /**
   * @brief Setter for connection
   */
  void setConnection(const ConnectionData& connection) { Connection = connection; }
  /**
   * @brief Getter for bandwidth attributes
   */
  std::vector<std::string> getBandwidth() { return Bandwidth; }
  /**
   * @brief Adds an bandwidth attribute
   */
  void addBandwidth(const std::string& sVal) { Bandwidth.push_back(sVal); }
  /**
   * @brief Getter for timings
   */
  const std::vector<std::string>& getTiming() const { return Timing; }
  /**
   * @brief Getter for timings
   */
  std::vector<std::string>& getTiming() { return Timing; }
  /**
   * @brief Adds an timing
   */
  void addTiming(const std::string& sVal) { Timing.push_back(sVal); }
  /**
   * @brief Setter for timings
   */
  void setTiming(const std::vector<std::string>& vTimings) { Timing = vTimings; }
  /**
   * @brief Getter for repeat times
   */
  std::string getRepeatTimes() const { return RepeatTimes; }
  /**
   * @brief Setter for repeat times
   */
  void setRepeatTimes(const std::string& sVal) { RepeatTimes = sVal; }
  /**
   * @brief Getter for time zones
   */
  std::string getTimeZones() const { return TimeZones; }
  /**
   * @brief Setter for time zones
   */
  void setTimeZones(const std::string& sVal) { TimeZones = sVal; }
  /**
   * @brief Getter for encryption times
   */
  std::string getEncryptionKeys() const { return EncryptionKeys; }
  /**
   * @brief Setter for encryption times
   */
  void setEncryptionKeys(const std::string& sVal) { EncryptionKeys = sVal; }
  /**
   * @brief Adds a media description
   */
  void addMediaDescription(const MediaDescription& mediaDescription)
  {
    MediaDescriptions.push_back(mediaDescription);
  }
  /**
   * @brief Removes the media description at the specified index
   */
  bool removeMediaDescription(uint32_t uiIndex)
  {
    if (uiIndex < MediaDescriptions.size())
    {
      MediaDescriptions.erase(MediaDescriptions.begin() + uiIndex);
      return true;
    }
    return false;
  }
  /**
   * @brief Clears all media descriptions
   */
  void clearMediaDescriptions()
  {
    MediaDescriptions.clear();
  }
  /**
   * @brief returns a count of media descriptions
   */
  uint32_t getMediaDescriptionCount() const { return MediaDescriptions.size(); }
  /**
   * @brief returns the media descriptions at the specified index.
   */
  MediaDescription& getMediaDescription(uint32_t uiIndex){ return MediaDescriptions.at(uiIndex); }
  /**
   * @brief returns the media descriptions at the specified index.
   */
  const MediaDescription& getMediaDescription(uint32_t uiIndex) const { return MediaDescriptions.at(uiIndex); }
  /**
   * @brief returns the media descriptions last added the session description.
   */
  MediaDescription& getLastAddedMediaDescription(){ return MediaDescriptions.at(MediaDescriptions.size() - 1); }
  /**
   * @brief returns the media descriptions last added the session description.
   */
  const MediaDescription& getLastAddedMediaDescription() const { return MediaDescriptions.at(MediaDescriptions.size() - 1); }
  /**
   * @brief returns a vector containing all media descriptions.
   */
  const std::vector<MediaDescription>& getMediaDescriptions() const { return MediaDescriptions; }
  /**
   * @brief returns a vector containing all media descriptions.
   */
  std::vector<MediaDescription>& getMediaDescriptions() { return MediaDescriptions; }
  /**
   * @brief returns the media descriptions in the group at the specified index.
   * @return The media description at the specified index. If the index is invalid, an empty vector is returned.
   */
  std::vector<MediaDescription> getMediaDescriptionsInGroup(uint32_t uiIndex) const;
  /**
   * @brief Gets the group at the specified index.
   * @return A const reference to the group at the specified index.
   */
  const rfc5888::Group& getGroup(uint32_t uiIndex) const { return Groups.at(uiIndex); }
  /**
   * @brief Gets the group at the specified index.
   * @return A reference to the group at the specified index.
   */
  rfc5888::Group& getGroup(uint32_t uiIndex) { return Groups.at(uiIndex); }
  /**
   * @brief Gets the groups
   * @return A reference to a vector containing all groups.
   */
  std::vector<rfc5888::Group>& getGroups()  { return Groups; }
  /**
   * @brief Gets the groups
   * @return A const reference to a vector containing all groups.
   */
  const std::vector<rfc5888::Group>& getGroups() const { return Groups; }
  /**
   * @brief Gets the m-line indentifiers that are non-grouped in the SDP.
   * @return A reference to a vector containing all non-grouped mids.
   */
  std::vector<std::string>& getNonGroupedMids()  { return NonGrouped; }
  /**
   * @brief Gets the m-line indentifiers that are non-grouped in the SDP.
   * @return A const reference to a vector containing all non-grouped mids.
   */
  const std::vector<std::string>& getNonGroupedMids() const { return NonGrouped; }
  /**
   * @brief Checks if the attribute exists at session level
   * @return Returns true if the attribute exists at session level, false otherwise.
   */
  bool hasAttribute(const std::string& sAttributeName);
  /**
   * @brief Gets the first attribute with the specified name
   * @return A optional containing the first attribute with the specified name.
   * This is a null pointer if no such attributes exist
   */
  boost::optional<std::string> getAttributeValue(const std::string& sAttributeName) const;
  /**
   * @brief Gets a vector containing all attributes with the specified name
   * @return A vector containing all attributes with the specified name.
   */
  std::vector<std::string> getAttributeValues(const std::string& sAttributeName ) const;
  /**
   * @brief Adds a value to the attribute map
   * @param[in] sName The name of the attribute
   * @param[in] sValue The value of the attribute
   */
  void addAttribute(const std::string& sName, const std::string& sValue);
  /**
   * @brief Getter for AttributeMap
   * @return A const reference to the attribute map 
   */
  const std::multimap<std::string, std::string>& getAttributes() const { return AttributeMap; }
  /**
   * @brief Getter for AttributeMap
   * @return A reference to the attribute map 
   */
  std::multimap<std::string, std::string>& getAttributes() { return AttributeMap; }
  /**
   * @brief returns the index if the media description indentified by sMid
   * @param[in] sMid Identifier of the m-line being searched for
   * @return The index of the m-line identifed by sMid, and -1 otherwise
   */
  int getMediaDescriptionIndex(const std::string& sMid) const;
  /**
   * @brief returns if there groups are defined in the SDP
   */
  bool isGrouped() const;
  /**
   * @brief returns the index of the group containing the m-line identifed by sMid
   * @param[in] sMid Identifier of the m-line being searched for
   * @return The index of the group, -1 if the identifier could not be located
   */
  int getGroupIndex(const std::string& sMid) const;
  /**
   * @brief Utility method to change all connection addresses at session and media level.
   */
  void updateAllConnectionAddresses(const std::string& sIpAddress);
private:

  void findUngroupedMediaDescriptions();

private:
  std::string m_sOriginalSdp;
  /// v= line
  std::string ProtocolVersion;
  /// o= line
  std::string Origin;
  /// s= line
  std::string SessionName;
  /// i= line
  std::string SessionInfo;
  /// u= line
  std::string Uri;
  /// e= line
  std::vector<std::string> EmailFields;
  /// p= line
  std::vector<std::string> PhoneNumbers;
  /// c=<nettype> <addrtype> <connection-address>
  ConnectionData Connection;
  /// b= line
  std::vector<std::string> Bandwidth;
  /// t= line
  std::vector<std::string> Timing;
  /// r= line
  std::string RepeatTimes;
  /// z= line
  std::string TimeZones;
  /// k= line
  std::string EncryptionKeys;
  /// a= line
  std::multimap<std::string, std::string> AttributeMap;
  /// m= lines
  std::vector<MediaDescription> MediaDescriptions;
  /// grouped m-lines
  std::vector<rfc5888::Group> Groups;
  /// non-grouped m-lines
  std::vector<std::string> NonGrouped;
};

} // rfc4566
} // rtp_plus_plus
