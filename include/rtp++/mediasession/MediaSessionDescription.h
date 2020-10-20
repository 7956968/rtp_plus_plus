#pragma once
#include <boost/optional.hpp>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/rfc2326/Transport.h>
#include <rtp++/rfc4566/Rfc4566.h>

namespace rtp_plus_plus
{

/**
 * This class takes the SDP descriptions of the local and remote participants
 * and associated network configuration files and creates a media session description object.
 * It's an alternate representation of the information stored in SDP and the network
 * configuration files.
 *
 * The MediaSessionDescription class takes grouping into account.
 */
class MediaSessionDescription
{
public:
  enum SdpDescriptionType
  {
    LOCAL_SDP,
    REMOTE_SDP
  };

  enum GroupType
  {
    GROUPED,
    NON_GROUPED
  };
  typedef std::pair<uint32_t, MediaSessionDescription::GroupType> MediaIndex_t;

  /**
   * @brief MediaSessionDescription Constructor for SDP + network config combination
   * @param sdesInfo
   * @param sdp
   * @param addresses
   * @param bIsSender
   */
  MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                          const rfc4566::SessionDescription& sdp,
                          const InterfaceDescriptions_t& addresses,
                          bool bIsSender);
  /**
   * @brief MediaSessionDescription Constructor for sdp + RTSP transport combination
   * @param sdesInfo
   * @param sdp
   * @param vSelectedTransports
   * @param bIsSender
   */
  MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                          const rfc4566::SessionDescription& sdp,
                          std::vector<rfc2326::Transport>& vSelectedTransports,
                          bool bIsSender);
  /**
   * @brief MediaSessionDescription Constructor for 2 SDPs
   * @param sdesInfo
   * @param localSdp
   * @param remoteSdp
   */
  MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                          const rfc4566::SessionDescription& localSdp,
                          const rfc4566::SessionDescription& remoteSdp);

  bool isSender() const { return m_bIsSender; }

  uint32_t getTotalSessionParameterCount() const { return m_vNonGroupedRtpSessionParameters.size() + m_vGroupedRtpSessionParameters.size(); }

  MediaIndex_t getSessionParameter(uint32_t uiIndex) const
  {
    return m_mediaOrder.at(uiIndex);
  }

  const RtpSessionParameters& getNonGroupedSessionParameters(uint32_t uiIndex) const
  {
    return m_vNonGroupedRtpSessionParameters.at(uiIndex);
  }

  const RtpSessionParametersGroup_t& getGroupedSessionParameters(uint32_t uiIndex) const
  {
    return m_vGroupedRtpSessionParameters.at(uiIndex);
  }

  // search for media index: returns the media index of the first match and a null pointer if no match is found
  boost::optional<MediaIndex_t> find(const std::string& sMediaType) const;

private:

  MediaSessionDescription(const rfc4566::SessionDescription& sdp);

  bool m_bIsSender;
  rfc4566::SessionDescription m_sdp;
  /// The vector stores the media lines in SDP order
  std::vector< std::pair<uint32_t, GroupType> > m_mediaOrder;
  std::vector<RtpSessionParameters> m_vNonGroupedRtpSessionParameters;
  std::vector<RtpSessionParametersGroup_t> m_vGroupedRtpSessionParameters;
};

} // rtp_plus_plus
