#pragma once
#include <set>
#include <vector>
#include <boost/optional.hpp>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{
namespace rfc3264
{

/**
 * @brief The OfferAnswerModel class manages media sources such as (compressed) video or audio sources,
 * and is able to returns a session description for the active/selected devices/sources
 */
class OfferAnswerModel
{
public:
  /**
   * @brief OfferAnswerModel Constructor
   * @param sipContext
   * @param mediaSessionNetworkManager
   * @param bEnableMpRtp
   * @param bRtcpMux
   * @param bRapidSync
   * @param bRtcpRs
   */
  OfferAnswerModel(rfc3261::SipContext& sipContext, MediaSessionNetworkManager& mediaSessionNetworkManager,
                   bool bEnableMpRtp, bool bRtcpMux, bool bRapidSync, bool bRtcpRs, bool bRtcpRtpHeaderExt);
  /**
   * @brief OfferAnswerModel Constructor
   * @param sipContext
   * @param mediaSessionNetworkManager
   * @param sRtpProfile
   * @param bEnableMpRtp
   * @param bRtcpMux
   * @param bRapidSync
   * @param bRtcpRs
   */
  OfferAnswerModel(rfc3261::SipContext& sipContext, MediaSessionNetworkManager& mediaSessionNetworkManager,
                   const std::string& sRtpProfile, bool bEnableMpRtp, bool bRtcpMux, bool bRapidSync, bool bRtcpRs, bool bRtcpRtpHeaderExt);
  /**
   * @brief Destructor
   */
  virtual ~OfferAnswerModel();
  /**
   * @brief addProfile
   * @param sRtpProfile
   */
  void addProfile(const std::string& sRtpProfile) { m_vSupportedRtpProfiles.insert(sRtpProfile); }
  /**
   * @brief setPreferredProfile
   * @param sRtpProfile
   */
  void setPreferredProfile(const std::string& sRtpProfile) { m_sRtpProfile = sRtpProfile; }
  /**
   * @brief setPreferredProtocol
   * @param sProtocol
   */
  void setPreferredProtocol(const std::string& sProtocol) { m_sProtocol = sProtocol; }
  /**
   * @brief Getter for network manager
   */
  MediaSessionNetworkManager& getMediaSessionNetworkManager() { return m_mediaSessionNetworkManager; }
  /**
   * @brief adds static payload format to be used in offer
   */
  bool addFormat(const std::string& sMediaType, const std::string& sFormat, uint32_t uiBandwidthKbps = 0,
                 const std::vector<std::string>& vRtcpFb = std::vector<std::string>(),
                 const std::vector<std::string>& vRtcpXr = std::vector<std::string>());
  /**
   * @brief adds dynamic payload format to be used in offer
   */
  bool addFormat(std::string& sFormat, const std::string& sMediaType,
                 boost::optional<rfc4566::RtpMapping> rtpMap,
                 boost::optional<rfc4566::FormatDescription> fmtp,
                 uint32_t uiBandwidthKbps = 0,
                 const std::vector<std::string>& vRtcpFb = std::vector<std::string>(),
                 const std::vector<std::string>& vRtcpXr = std::vector<std::string>());
  /**
   * @brief returns SDP offer for configuration
   */
  virtual boost::optional<rfc4566::SessionDescription> generateOffer();
  /**
   * @brief returns SDP offer for configuration using provided host and interfaces
   */
  virtual boost::optional<rfc4566::SessionDescription> generateOffer(const std::string& sHost, const InterfaceDescriptions_t& interfaces);
  /**
   * @brief returns SDP answer based on configured acceptable types
   */
  virtual boost::optional<rfc4566::SessionDescription> generateAnswer(const rfc4566::SessionDescription& offer);
  /**
   * @brief setPreferredStartingPort
   * @param uiPreferredStartingPort
   */
  void setPreferredStartingPort(const uint16_t uiPreferredStartingPort) { m_uiPreferredStartingPort = uiPreferredStartingPort; }
  /**
   * @brief setBindPorts configures whether the component should bind the ports when generating an offer or answer
   * @param bBindPorts
   *
   *  By default the ports are bound.
   */
  void setBindPorts(bool bBindPorts) { m_bBindPorts = bBindPorts; }
private:
  /**
   * @brief returns a MediaDescription based on the passed media object
   */
  virtual rfc4566::MediaDescription getMediaPreference(const rfc4566::MediaDescription& media);
  /**
   * @brief checks if the format is acceptable based on added formats
   */
  bool isFormatAcceptable(const std::string& sMediaType, const std::string& sFormat, boost::optional<rfc4566::RtpMapping> rtpMap, boost::optional<rfc4566::FormatDescription> fmtp) const;
  /**
   * @brief gets common feedback message types
   */
  std::vector<std::string> getCommonFeedbackTypes(const std::string& sFormat, const rfc4566::MediaDescription& media) const;
  /**
   * @brief allocates ports in the network manager and configures the port information in the media description.
   *
   * Takes into account both rtcp-mux and mprtp
   */
  void configurePorts(rfc4566::MediaDescription& media, uint16_t uiStartingPort);
  /**
   * @brief allocates ports in the network manager and configures the port information in the media description.
   *
   * Takes into account both rtcp-mux and mprtp
   */
  void configurePorts(const std::string& sHost, rfc4566::MediaDescription& media, const MediaInterfaceDescriptor_t& mediaInterface);
  /**
   * @brief createSessionDescription
   * @param sConnection
   * @return
   */
  rfc4566::SessionDescription createSessionDescription(const std::string& sConnection);

private:


  rfc3261::SipContext m_sipContext;
  MediaSessionNetworkManager& m_mediaSessionNetworkManager;
  std::string m_sRtpProfile;
  std::string m_sProtocol;

  // vector used during answer generation: check if we support the profile
  std::set<std::string> m_vSupportedRtpProfiles;
  bool m_bEnableMpRtp;
  bool m_bRtcpMux;
  bool m_bRapidSync;
  bool m_bRtcpRs;
  bool m_bRtcpRtpHeaderExt;

  // static payload formats should only have "Format" set,  dynamic ones should leave Format empty
  struct PayloadFormat
  {
    std::string Format;
    boost::optional<rfc4566::RtpMapping> rtpMap;
    boost::optional<rfc4566::FormatDescription> fmtp;
    bool Dynamic;
    std::vector<std::string> RtcpFb;
    std::vector<std::string> RtcpXr;

    PayloadFormat()
      :Dynamic(false)
    {

    }

    PayloadFormat(const std::string& sFormat,
                  const std::vector<std::string> vRtcpFb = std::vector<std::string>(),
                  const std::vector<std::string> vRtcpXr = std::vector<std::string>())
      :Format(sFormat),
        Dynamic(false),
        RtcpFb(vRtcpFb),
        RtcpXr(vRtcpXr)
    {

    }

    PayloadFormat(const std::string& sFormat,
                  boost::optional<rfc4566::RtpMapping> rtpMapping,
                  boost::optional<rfc4566::FormatDescription> format,
                  const std::vector<std::string> vRtcpFb = std::vector<std::string>(),
                  const std::vector<std::string> vRtcpXr = std::vector<std::string>())
      :Format(sFormat),
      rtpMap(rtpMapping),
      fmtp(format),
      Dynamic(true),
      RtcpFb(vRtcpFb),
      RtcpXr(vRtcpXr)
    {

    }
  };
  std::vector<PayloadFormat> m_vAudioPayloadFormats;
  std::vector<PayloadFormat> m_vVideoPayloadFormats;
  uint32_t m_uiNextDynamicPayload;

  uint32_t m_uiMaxVideoBandwidthKbps;
  uint32_t m_uiMaxAudioBandwidthKbps;

  uint16_t m_uiPreferredStartingPort;
  bool m_bBindPorts;
};

} // media
} // rtp_plus_plus
