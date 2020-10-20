#pragma once
#include <cstddef>
#include <memory>
#include <set>
#include <vector>
#include <cpputil/RandomUtil.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc2326/Transport.h>
#include <rtp++/rfc3550/SdesInformation.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc5285/Extmap.h>
#include <rtp++/rfc5888/Rfc5888.h>

namespace rtp_plus_plus
{

/// fwd
class RtpSessionParameters;
typedef std::vector<RtpSessionParameters> RtpSessionParametersGroup_t;

/**
 * @brief This class is the heart of the rtp++ library. It contains all information
 * necessary to setup and specialize an RTP session.
 */
class RtpSessionParameters
{
public:
  /**
   * @brief Named constructor
   *
   * Utility method that takes care of correct initialisation of RTP parameters
   */
  std::unique_ptr<RtpSessionParameters> createRtpParameters(const rfc3550::SdesInformation &sdesInfo);
  /**
   * @brief Named constructor
   *
   * Utility method that takes care of correct initialisation of RTP parameters
   */
  std::unique_ptr<RtpSessionParameters> createRtpParameters(const rfc3550::SdesInformation& sdesInfo,
                                                            const rfc4566::MediaDescription& media,
                                                            bool bLocalMediaDescription);
  /**
   * @brief Named constructor
   * 
   * Utility method that takes care of correct initialisation of RTP parameters
   */
  std::unique_ptr<RtpSessionParameters> createRtpParameters(const rfc3550::SdesInformation& sdesInfo,
                                                            const rfc4566::MediaDescription& media,
                                                            const rfc2326::Transport& transport,
                                                            bool bLocalMediaDescription);
  /**
   * @brief Default constructor
   */
  RtpSessionParameters();
  /**
   * @brief Constructor
   * @param sdesInfo: source description
   */
  RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo);
  /**
   * @brief Constructor
   * @param sdesInfo: source description
   * @param media: media description from SDP
   * @param bLocalMediaDescription determines if the media description is local
   */
  RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                       const rfc4566::MediaDescription& media,
                       bool bLocalMediaDescription);
  /**
   * @brief Constructor
   * @param sdesInfo
   * @param media
   * @param transport
   * @param bLocalMediaDescription
   */
  RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                       const rfc4566::MediaDescription& media,
                       const rfc2326::Transport& transport,
                       bool bLocalMediaDescription);
  /**
   * @brief Getter for the source description information
   * @return the source description
   */
  rfc3550::SdesInformation getSdesInfo() const { return m_sdesInfo; }
  /**
   * @brief Setter for the source description
   * @param val New source description
   */
  void setSdesInfo(rfc3550::SdesInformation val) { m_sdesInfo = val; }
  /**
   * @brief getMediaLineIndexInSdp returns the media line index in the SDP if the corresponding constructor was used
   * @return media line index in SDP or UINT32_MAX if not set.
   */
  uint32_t getMediaLineIndexInSdp() const { return m_uiMediaIndexInSdp; }
  /**
   * @brief getTransport returns Transport. 
   * 
   * If the RtpSessionParameters object was not created with the appropriate constructor
   * the transport will be invalid.
   * @return Transport
   *
   * TODO: better would be boost::optional
   */
  rfc2326::Transport getTransport() const { return m_transport; }
  /**
   * @brief Getter for if is multicast session
   */
  bool isMulticastSession() const;

  /// Accessor for transport protocol
  /// @return if UDP should be used
  bool useRtpOverUdp() const { return m_protocol == RTP_UDP; }
  /// Accessor for transport protocol
  /// @return if TCP should be used
  bool useRtpOverTcp() const { return m_protocol == RTP_TCP; }
  /// Accessor for transport protocol
  /// @return if DCCP should be used
  bool useRtpOverDccp() const { return m_protocol == RTP_DCCP; }
  /// Accessor for transport protocol
  /// @return if SCTP should be used
  bool useRtpOverSctp() const { return m_protocol == RTP_SCTP; }
  /// Connection is active or passive
  /// @return if the connection is active (for TCP and SCTP streaming)
  bool isActiveConnection() const { return m_bActive; }


  /// Accessor for RTP profile
  /// @return RTP profile if set
  std::string getProfile() const { return m_sProfile; }

  /// Accessor for media type
  /// @return media type if set
  std::string getMediaType() const { return m_sMediaType; }

  /// Accessor for mid
  /// @return mid if set
  std::string getMid() const { return m_sMid; }

  /// Accessor for the current session bandwidth in kbps
  /// @return the session bandwidth in kbps
  uint32_t getSessionBandwidthKbps() const { return m_uiSessionBandwidthKbps; }
  /// Mutator for the session bandwidth in kbps
  /// @param uiSessionBandwidthKbps the new value for the session bandwidth in kbps
  void setSessionBandwidthKbps(uint32_t uiSessionBandwidthKbps){ m_uiSessionBandwidthKbps = uiSessionBandwidthKbps;}

  /**
   * @brief setGroup Setter for group info
   * @param group
   */
  void setGroup(const rfc5888::Group& group) { m_group = group; }

  /// this method can be used to configure an RtpSessionParameters object without an SDP
  /// @param uiPayloadType The payload type to be added
  /// @param sCodec The codec/mimetype of the payload
  /// @param uiTimestampFrequency The RTP clock rate of the payload type
  /// @return true if the payload type could be added and false otherwise
  bool addPayloadType(const uint8_t uiPayloadType, const std::string& sCodec, const uint32_t uiTimestampFrequency);

  bool hasAttribute(const std::string& sAttributeName) const;

  std::vector<std::string> getAttributeValues(const std::string& sAttributeName) const;

  /// Accessor for active payload type
  /// @return the active payload type and 0xFF if none has been configured
  uint8_t getPayloadType() const ;

  /// Accessor for added payload types
  /// @ returns a list of all added payload types
  std::vector<uint8_t> getPayloadTypes() const;

  /// This method can be called to update the active payload type
  /// @param uiPt the new payload type to be used
  /// @return true of the payload type is valid and has been previously added and false if not.
  bool updatePayloadType(uint8_t uiPt);

  /// Accessor for active RTP clock rate
  /// @ return clock rate if payload type is active, 0 if not
  uint32_t getRtpTimestampFrequency() const;

  /// Accessor for active RTP codec/mime-type
  /// @ return codec if payload type is active, "" if not
  std::string getEncodingName() const;

  /// Add a local RTP/RTCP endpoint to the session
  /// @param endpoint An RTP and RTCP endpoint pair
  void addLocalEndPoint(const EndPointPair_t& endPoints);

  /// Add a local RTP/RTCP endpoint to the session
  /// @param endpoint An RTP and RTCP endpoint pair
  void addLocalEndPoints(const std::vector<EndPointPair_t>& endPoints);

  /// Add a remote RTP/RTCP endpoint to the session
  /// @param endpoint An RTP and RTCP endpoint pair
  void addRemoteEndPoint(const EndPointPair_t& endPoints);

  /// Add a remote RTP/RTCP endpoint to the session
  /// @param endpoint An RTP and RTCP endpoint pair
  void addRemoteEndPoints(const std::vector<EndPointPair_t>& endPoints);

  /// Returns the number of local endpoint pairs
  /// @return the number of local endpoint pairs
  std::size_t getLocalEndPointCount() const { return m_vLocalEndPoints.size(); }
  /// Returns the number of remote endpoint pairs
  /// @return the number of remote endpoint pairs
  std::size_t getRemoteEndPointCount() const { return m_vRemoteEndPoints.size(); }

  /// Accessor for local endpoint pair at the specified index.
  /// @param index the index of the requested local endpoint pair
  /// @return the local endpoint pair at the specified index. Throws an out_of_range exception if the index is invalid.
  const EndPointPair_t& getLocalEndPoint(unsigned index) const
  {
    return m_vLocalEndPoints.at(index);
  }

  /// Accessor for remote endpoint pair at the specified index.
  /// @param index the index of the requested remote endpoint pair
  /// @return the remote endpoint pair at the specified index. Throws an out_of_range exception if the index is invalid.
  const EndPointPair_t& getRemoteEndPoint(unsigned index) const
  {
    return m_vRemoteEndPoints.at(index);
  }

  std::vector<EndPoint> getLocalRtpEndPoints() const
  {
    return m_vLocalRtpEndPoints;
  }

  std::vector<EndPoint> getLocalRtcpEndPoints() const
  {
    return m_vLocalRtcpEndPoints;
  }

  std::vector<EndPoint> getRemoteRtpEndPoints() const
  {
    return m_vRemoteRtpEndPoints;
  }

  std::vector<EndPoint> getRemoteRtcpEndPoints() const
  {
    return m_vRemoteRtcpEndPoints;
  }

  void setRtxParameters(uint8_t uiPt, uint8_t uiApt, uint32_t uiRtxTime);

  /**
   * returns of retransmission is enabled for the RTP session
   * It suffices to check if the timeout is greater than zero
   * since this value is only set if the SDP RTX attributes
   * have been set.
   */
  bool isRetransmissionEnabled() const { return m_sProfile == rfc4585::AVPF && m_uiRtxTime > 0; }
  /// returns if the payload type is set for retransmissions
  bool isRtxPayloadType(uint8_t uiPt) const { return uiPt == m_uiRtxPt; }
  /// Returns the retransmission timeout in milliseconds
  uint32_t getRetransmissionTimeout() const { return m_uiRtxTime; }
  /// Returns the retransmission payload type
  uint8_t getRetransmissionPayloadType() const { return m_uiRtxPt; }
  /// Returns the associated retransmission payload type
  uint8_t getAssociatedRetransmissionPayloadType() const { return m_uiRtxApt; }
  /// Sequence number space for RTX
  // uint16_t getNextRetransmissionSequenceNumber(){ return m_uiRtxSequenceNumber++;}
  /// SSRC for RTX using SSRC multiplexing only
  // uint32_t getRetransmissionSSRC() const { return m_uiRtxSSRC; }

  /// Feedback related
  void addFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage);

  /// Feedback related
  void addFeedbackMessages(const uint8_t uiPayload, const std::vector<std::string>& vMessages);
  bool supportsFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage) const;
  bool supportsFeedbackMessage(const std::string& sMessage) const;

  /// returns if XR parameters have been configured
  bool isXrEnabled() const { return !m_mXr.empty(); }

  const std::map<std::string, std::string> getXrParameters() const
  {
    return m_mXr;
  }

  bool hasXrAttribute(const std::string& sAttributeName, std::string& sAttributeValue) const
  {
    auto it = m_mXr.find(sAttributeName);
    if (it != m_mXr.end())
    {
      sAttributeValue = it->second;
      return true;
    }
    else
    {
      return false;
    }
  }

  void setXrParameters(const std::map<std::string, std::string>& mXr)
  {
    m_mXr = mXr;
  }

  boost::optional<rfc5285::Extmap> lookupExtmap(const uint32_t uiId) const
  {
    const std::map<uint32_t, rfc5285::Extmap>::const_iterator it = m_mExtmapById.find(uiId);
    if (it == m_mExtmapById.end())
      return boost::optional<rfc5285::Extmap>();
    else
      return boost::optional<rfc5285::Extmap>(it->second);
  }

  boost::optional<rfc5285::Extmap> lookupExtmap(const std::string& sExtensionName) const
  {
    std::map<std::string, rfc5285::Extmap>::const_iterator it = m_mExtmapByName.find(sExtensionName);
    if (it == m_mExtmapByName.end())
      return boost::optional<rfc5285::Extmap>();
    else
      return boost::optional<rfc5285::Extmap>(it->second);
  }

  void addExtMap(const rfc5285::Extmap& extmap)
  {
    m_mExtmapById[extmap.getId()] = extmap;
    m_mExtmapByName[extmap.getExtensionName()] = extmap;
  }

private:

  // helper method to parse MediaDescription and translate SDP into local format
  void parseMediaDescription(const rfc4566::MediaDescription& media, bool bLocalMediaDescription);
  // process RTX parameters helper
  void processRetransmissionParameters(const rfc4566::MediaDescription& media);
  // process XR
  void processXrParameters(const rfc4566::MediaDescription& media);
  // process extmap
  void processExtmapParameters(const rfc4566::MediaDescription& media);

private:

  // SDES information of host
  rfc3550::SdesInformation m_sdesInfo;
  // estimated session bandwidth
  uint32_t m_uiSessionBandwidthKbps;
  // nth media line in SDP
  uint32_t m_uiMediaIndexInSdp;
  // Transport
  rfc2326::Transport m_transport;
  // index of current payload: this is -1 if not set
  int32_t m_iPayloadTypeIndex;
  // group
  rfc5888::Group m_group;

  // vector of endpoints to cater for multihomed hosts
  std::vector<EndPointPair_t> m_vLocalEndPoints;
  std::vector<EndPointPair_t> m_vRemoteEndPoints;
  // convenience: store endpoints separately
  std::vector<EndPoint> m_vLocalRtpEndPoints;
  std::vector<EndPoint> m_vLocalRtcpEndPoints;
  std::vector<EndPoint> m_vRemoteRtpEndPoints;
  std::vector<EndPoint> m_vRemoteRtcpEndPoints;

  enum RtpProtocol
  {
    RTP_UDP = 0,
    RTP_TCP = 1,
    RTP_DCCP = 2,
    RTP_SCTP = 3
  };
  RtpProtocol m_protocol;

  // connection initiator or acceptor
  bool m_bActive;
  // RTP profile
  std::string m_sProfile;
  // Type
  std::string m_sMediaType;
  // Mid
  std::string m_sMid;

  // general media attributes
  std::multimap<std::string, std::string> m_mAttributes;

  // payload specific members
  // payload types
  std::vector<uint8_t> m_payloadTypes;
  // payload codec
  std::map<uint8_t, std::string> m_mCodecs;
  // payload clock rates
  std::map<uint8_t, uint32_t> m_mClockRates;

  // RTX
  uint8_t m_uiRtxPt;
  uint8_t m_uiRtxApt;
  uint32_t m_uiRtxTime;

  // FB
  std::map<uint8_t, std::vector<std::string> > m_mFeedback;
  std::set<std::string> m_feedbackMessageTypes;

  // XR attributes
  std::map<std::string, std::string> m_mXr;

  // extmap by id and name
  std::map<uint32_t, rfc5285::Extmap> m_mExtmapById;
  std::map<std::string, rfc5285::Extmap> m_mExtmapByName;
};

} // rtp_plus_plus
