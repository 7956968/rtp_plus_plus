#include "CorePch.h"
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc4566/MediaDescription.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4588/Rfc4588.h>
#include <rtp++/rfc5285/Rfc5285.h>
#include <rtp++/rfc5761/Rfc5761.h>
#include <rtp++/mprtp/MpRtp.h>

namespace rtp_plus_plus
{

std::vector<EndPointPair_t> extractEndPoints(const std::string& sIpAddress, const rfc4566::MediaDescription& media)
{
  uint16_t uiRtpPort = media.getPort();
  uint16_t uiRtcpPort = (media.hasAttribute(rfc5761::RTCP_MUX)) ? uiRtpPort : uiRtpPort + 1;

  std::vector<EndPointPair_t> endPoints;
  endPoints.push_back(std::make_pair(EndPoint(sIpAddress, media.getPort()), EndPoint(sIpAddress, uiRtcpPort)));

  if (media.hasAttribute(mprtp::MPRTP))
  {
    const std::multimap<std::string, std::string>& attributeMap = media.getAttributes();
    // first additional interface must match m= line interface
    auto range_it = attributeMap.equal_range(mprtp::MPRTP);
    for (auto it = range_it.first; it!=range_it.second; ++it)
    {
      // changing SDP to also feature bind parameter
      if (boost::algorithm::contains(it->second, "interface"))
      {
        // format has changed to 'a=mprtp interface:1 192.0.2.1:49170'
        std::vector<std::string> vTokens = StringTokenizer::tokenize(it->second, " /:");

        // check for simple mprtp attribute declaration with no interfaces
        if (vTokens.size() == 1)
        {
          break;
        }

        uint16_t uiRtpPort = convert<uint16_t>(vTokens[3], 0);
        uint16_t uiRtcpPort = (media.hasAttribute(rfc5761::RTCP_MUX)) ? uiRtpPort : uiRtpPort + 1;

        if (sIpAddress == vTokens[2] && media.getPort() == uiRtpPort)
        {
          DLOG(INFO) << "Default interface verified: " << vTokens[2] << ":" <<  vTokens[3];
          // update MPRTP IDs
          endPoints[0].first.setId(convert<uint32_t>(vTokens[1], 0));
          endPoints[0].second.setId(convert<uint32_t>(vTokens[1], 0));
        }
        else
        {
          // add additional interface
          EndPoint rtpEp(vTokens[2], uiRtpPort);
          // store ID: this will be used later to differentiate between endpoints
          rtpEp.setId(convert<uint32_t>(vTokens[1], 0));
          // RTCP
          EndPoint rtcpEp(vTokens[2], uiRtcpPort);
          rtcpEp.setId(convert<uint32_t>(vTokens[1], 0));

          endPoints.push_back(std::make_pair(rtpEp, rtcpEp));
          LOG(INFO) << "Adding additional interface: " << vTokens[0] << ":" <<  vTokens[1] << ":" << vTokens[2] << ":" << vTokens[3];
        }
      }
    }
  }
  return endPoints;
}

std::vector<EndPointPair_t> extractEndPoints(const MediaInterfaceDescriptor_t& mediaInterfaceDescriptor)
{
  std::vector<EndPointPair_t> endPoints;
  for (size_t i = 0; i < mediaInterfaceDescriptor.size(); ++i)
  {
    const AddressTuple_t& addressPair = mediaInterfaceDescriptor[i];
    AddressDescriptor rtp_descriptor = std::get<0>(addressPair);
    AddressDescriptor rtcp_descriptor = std::get<1>(addressPair);
    endPoints.push_back(std::make_pair(EndPoint(rtp_descriptor), EndPoint(rtcp_descriptor)));
  }
  return endPoints;
}

#if 0
void processXrParameters(RtpSessionParameters& parameters, const rfc4566::MediaDescription& media)
{
  parameters.setXrParameters(media.getXrMap());
}

void processExtmapParameters(RtpSessionParameters& parameters, const rfc4566::MediaDescription& media)
{
  const std::vector<std::string> vExtmap = media.getAttributeValues(rfc5285::EXTMAP);
  for (const std::string& sExtmapValue: vExtmap)
  {
    boost::optional<rfc5285::Extmap> extmap = rfc5285::Extmap::parseFromSdpAttribute(sExtmapValue);
    if (extmap)
    {
      parameters.addExtMap(*extmap);
    }
  }
}

void processRetransmissionParameters(RtpSessionParameters& parameters, const rfc4566::MediaDescription& media)
{
  // RTX: SSRC-multiplexing
  // a=rtpmap:97 rtx/90000
  // a=fmtp:97 apt=96;rtx-time=3000
  // check if rtx exists in rtpmap
  uint8_t uiPt = 0;
  std::string sApt;
  uint8_t uiApt = 0;
  uint32_t uiRtxTime = 0;
  const std::map<std::string, rfc4566::RtpMapping>& rtpMap = media.getRtpMap();
  for (auto it = rtpMap.begin(); it != rtpMap.end(); ++it)
  {
    if (it->second.EncodingName == rfc4588::RTX)
    {
      // found retransmission session
      bool bRes = true;
      uiPt = convert<uint8_t>(it->first, bRes);
      assert(bRes);
      LOG(INFO) << "Found retransmission payload type: " << static_cast<uint32_t>(uiPt);
      auto itRtp = media.getFormatMap().find(it->first);
      if (itRtp == media.getFormatMap().end())
      {
        LOG(INFO) << "No associated rtpmap line for " << static_cast<uint32_t>(uiPt);
        break;
      }
      else
      {
        std::vector<std::string> values = StringTokenizer::tokenize(itRtp->second.fmtp, ";");
        for (size_t i = 0; i < values.size(); ++i)
        {
          std::vector<std::string> nameValue = StringTokenizer::tokenize(values[i], "=");
          assert(nameValue.size() == 2);
          if (nameValue[0] == rfc4588::APT)
          {
            sApt = nameValue[1];
            uiApt = convert<uint8_t>(nameValue[1], bRes);
            assert(bRes);
            LOG(INFO) << "Found associated retransmission payload type: " << static_cast<uint32_t>(uiApt);
          }
          else if (nameValue[0] == rfc4588::RTX_TIME)
          {
            uiRtxTime = convert<uint32_t>(nameValue[1], bRes);
            assert(bRes);
            LOG(INFO) << "Found associated retransmission time: " << uiRtxTime;
          }
        }

        assert(uiPt != uiApt);
        if (uiRtxTime != 0)
        {
          parameters.setRtxParameters(uiPt, uiApt, uiRtxTime);
        }

        VLOG(2) << "Retrieving feedback for associated payload type " << sApt;
        // check for FB specific messages
        auto itFb = media.getFeedbackMap().find(sApt);
        if (itFb != media.getFeedbackMap().end())
        {
          const std::vector<std::string>& vFeedbackMessages = itFb->second.FeedbackTypes;
          bool bRes = false;
          uint8_t uiPayload = convert<uint8_t>(it->first, bRes);
          assert(bRes);
          VLOG(5) << "Adding feedback messages: " << ::toString(vFeedbackMessages);
          parameters.addFeedbackMessages(uiPayload, vFeedbackMessages);
        }
      }
    }
  }
}
#endif

RtpSessionParameters createRtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                                                const std::string& sSessionLevelIpAddress,
                                                const rfc4566::MediaDescription& media,
                                                const MediaInterfaceDescriptor_t& mediaInterface,
                                                MediaSessionDescription::SdpDescriptionType eSdpDescriptionType)
{
  RtpSessionParameters parameters(sdesInfo, media, eSdpDescriptionType == MediaSessionDescription::LOCAL_SDP);

  // Add SDP info
  const rfc4566::ConnectionData& connection = media.getConnection();
  // get connection from session level if not at media level
  std::string sIpAddress = connection.isSet() ? connection.ConnectionAddress : sSessionLevelIpAddress;

  std::vector<EndPointPair_t> endPoints = extractEndPoints(sIpAddress, media);
  if (eSdpDescriptionType == MediaSessionDescription::LOCAL_SDP)
    parameters.addLocalEndPoints(endPoints);
  else
    parameters.addRemoteEndPoints(endPoints);

#if 0
  // Retransmission
  processRetransmissionParameters(parameters, media);
  // XR
  processXrParameters(parameters, media);
  // extmap
  processExtmapParameters(parameters, media);
#endif

  std::vector<EndPointPair_t> endPoints2 = extractEndPoints(mediaInterface);
  if (eSdpDescriptionType == MediaSessionDescription::LOCAL_SDP)
    parameters.addRemoteEndPoints(endPoints2);
  else
    parameters.addLocalEndPoints(endPoints2);

  return parameters;
}

RtpSessionParameters createRtpSessionParametersWithoutEndpointInfo(const rfc3550::SdesInformation& sdesInfo,
                                                const rfc4566::MediaDescription& media,
                                                const rfc2326::Transport& transport,
                                                MediaSessionDescription::SdpDescriptionType eSdpDescriptionType)
{
  RtpSessionParameters parameters(sdesInfo, media, transport, eSdpDescriptionType == MediaSessionDescription::LOCAL_SDP);

#if 0
  // Retransmission
  processRetransmissionParameters(parameters, media);
  // XR
  processXrParameters(parameters, media);
  // extmap
  processExtmapParameters(parameters, media);
#endif

  return parameters;
}

RtpSessionParameters createRtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                                                const std::string& sLocalSessionLevelIpAddress,
                                                const rfc4566::MediaDescription& localMedia,
                                                const std::string& sRemoteSessionLevelIpAddress,
                                                const rfc4566::MediaDescription& remoteMedia)
{
  VLOG(2) << "createRtpSessionParameters: local media: " << localMedia.toString();

  RtpSessionParameters parameters(sdesInfo, localMedia, true);

  // local SDP
  std::string sLocalIpAddress = localMedia.getConnection().ConnectionAddress;
  if (sLocalIpAddress == "0.0.0.0")
  {
    // get connection from session level if not at media level
    sLocalIpAddress = sLocalSessionLevelIpAddress;
  }
  parameters.addLocalEndPoints(extractEndPoints(sLocalIpAddress, localMedia));

#if 0
  // Retransmission
  processRetransmissionParameters(parameters, localMedia);
  // XR
  processXrParameters(parameters, localMedia);
  // extmap
  processExtmapParameters(parameters, localMedia);
#endif

  // remote SDP
  std::string sRemoteIpAddress = remoteMedia.getConnection().ConnectionAddress;
  if (sRemoteIpAddress == "0.0.0.0")
  {
    // get connection from session level if not at media level
    sRemoteIpAddress = sRemoteSessionLevelIpAddress;
  }
  parameters.addRemoteEndPoints(extractEndPoints(sRemoteIpAddress, remoteMedia));

  return parameters;
}

RtpSessionParametersGroup_t createRtpSessionParametersGroup(const rfc5888::Group& group,
                                                            const rfc3550::SdesInformation& sdesInfo,
                                                            const rfc4566::SessionDescription &sdp,
                                                            const std::vector<rfc4566::MediaDescription>& vMediaDescriptions,
                                                            const InterfaceDescriptions_t& vMediaInterfaces,
                                                            MediaSessionDescription::SdpDescriptionType eSdpDescriptionType)
{
  assert(vMediaDescriptions.size() == vMediaInterfaces.size());
  RtpSessionParametersGroup_t vGroupParameters;
  const std::string sSessionLevelIp = sdp.getConnection().ConnectionAddress;
  for (size_t i = 0; i < vMediaDescriptions.size(); ++i)
  {
    RtpSessionParameters params = createRtpSessionParameters(sdesInfo, sSessionLevelIp, vMediaDescriptions[i], vMediaInterfaces[i], eSdpDescriptionType);
    params.setGroup(group);
    vGroupParameters.push_back(params);
  }
  return vGroupParameters;
}

RtpSessionParametersGroup_t createRtpSessionParametersGroup(const rfc5888::Group& group,
                                                            const rfc3550::SdesInformation& sdesInfo,
                                                            const rfc4566::SessionDescription &sdp,
                                                            const std::vector<rfc4566::MediaDescription>& vMediaDescriptions,
                                                            const std::vector<rfc2326::Transport>& vTransports,
                                                            MediaSessionDescription::SdpDescriptionType eSdpDescriptionType)
{
  // new for interleaving
  RtpSessionParametersGroup_t vGroupParameters;
  for (size_t i = 0; i < vMediaDescriptions.size(); ++i)
  {
    RtpSessionParameters params = createRtpSessionParametersWithoutEndpointInfo(sdesInfo, vMediaDescriptions[i], vTransports[i], eSdpDescriptionType);
    params.setGroup(group);
    vGroupParameters.push_back(params);
  }
  return vGroupParameters;
}

RtpSessionParametersGroup_t createRtpSessionParametersGroup(const rfc5888::Group& group,
                                                            const rfc3550::SdesInformation& sdesInfo,
                                                            const std::string& sLocalSessionLevelIp,
                                                            std::vector<rfc4566::MediaDescription>& localMediaDescriptions,
                                                            const std::string& sRemoteSessionLevelIp,
                                                            std::vector<rfc4566::MediaDescription>& remoteMediaDescriptions)
{
  assert(localMediaDescriptions.size() == remoteMediaDescriptions.size());

  RtpSessionParametersGroup_t vGroupParameters;
  for (size_t i = 0; i < localMediaDescriptions.size(); ++i)
  {
    RtpSessionParameters params = createRtpSessionParameters(sdesInfo, sLocalSessionLevelIp, localMediaDescriptions[i], sRemoteSessionLevelIp, remoteMediaDescriptions[i]);
    params.setGroup(group);
    vGroupParameters.push_back(params);
  }
  return vGroupParameters;
}

MediaSessionDescription::MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                                                 const rfc4566::SessionDescription& rSdp,
                                                 const InterfaceDescriptions_t& addresses,
                                                 bool bIsSender)
  :m_bIsSender(bIsSender)
{
  VLOG(2) << "media count: " << rSdp.getMediaDescriptionCount() << " address count: " << addresses.size();
  assert (rSdp.getMediaDescriptionCount() == addresses.size());

  MediaSessionDescription::SdpDescriptionType eSdpDescriptionType = bIsSender ? LOCAL_SDP : REMOTE_SDP;

  // cast away const for map access via []
  rfc4566::SessionDescription& sdp = const_cast<rfc4566::SessionDescription&>(rSdp);

  if (sdp.isGrouped())
  {
    // set to store mids that have already been processed in a group
    std::set<std::string> processedMids;
    for (size_t i = 0; i < sdp.getMediaDescriptionCount(); ++i)
    {
      const rfc4566::MediaDescription& media = sdp.getMediaDescription(i);
      if (processedMids.find(media.getMid()) == processedMids.end())
      {
        int32_t index = rSdp.getGroupIndex(media.getMid());
        if (index != -1)
        {
          std::vector<rfc4566::MediaDescription> mediaDescriptions;
          InterfaceDescriptions_t mediaInterfaces;

          // process entire group
          const rfc5888::Group& group = sdp.getGroup(index);
          for (const std::string& sMid : group.getMids())
          {
            if (processedMids.find(sMid) == processedMids.end())
            {
              int mediaIndex = sdp.getMediaDescriptionIndex(sMid);
              assert(mediaIndex != -1);
              mediaDescriptions.push_back(sdp.getMediaDescription(mediaIndex));
              mediaInterfaces.push_back(addresses[mediaIndex]);
              processedMids.insert(sMid);
            }
          }
          // create Rtp session parameters for group
          RtpSessionParametersGroup_t rtpParameters = createRtpSessionParametersGroup(group, sdesInfo, sdp, mediaDescriptions, mediaInterfaces, eSdpDescriptionType);
          assert(!rtpParameters.empty());
          m_vGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, GROUPED));
        }
        else
        {
          // this m line is not part of a group: process individual media line
          processedMids.insert(media.getMid());
          std::string sSessionLevelIp = sdp.getConnection().ConnectionAddress;
          RtpSessionParameters rtpParameters = createRtpSessionParameters(sdesInfo, sSessionLevelIp, sdp.getMediaDescription(i), addresses.at(i), eSdpDescriptionType);
          m_vNonGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vNonGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, NON_GROUPED));
        }
      }
    }
  }
  else
  {
    // create RtpSessionParameters for entire group
    m_vNonGroupedRtpSessionParameters = createRtpSessionParametersGroup(rfc5888::Group(), sdesInfo, rSdp, rSdp.getMediaDescriptions(), addresses, eSdpDescriptionType);
    for (size_t i = 0; i < m_vNonGroupedRtpSessionParameters.size(); ++i)
    {
      m_mediaOrder.push_back(std::make_pair(i, NON_GROUPED));
    }
  }
}

// TODO: refactor, for now just copying code
MediaSessionDescription::MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                                                 const rfc4566::SessionDescription& rSdp,
                                                 std::vector<rfc2326::Transport>& vTransports,
                                                 bool bIsSender)
  :m_bIsSender(bIsSender)
{
  MediaSessionDescription::SdpDescriptionType eSdpDescriptionType = bIsSender ? LOCAL_SDP : REMOTE_SDP;

  // cast away const for map access via []
  rfc4566::SessionDescription& sdp = const_cast<rfc4566::SessionDescription&>(rSdp);

  if (sdp.isGrouped())
  {
    LOG(FATAL) << "Not implemented";
    // TODO: implement grouped SDP + interleaving?
    assert(false);
#if 0
    // set to store mids that have already been processed in a group
    std::set<std::string> processedMids;
    for (size_t i = 0; i < sdp.getMediaDescriptionCount(); ++i)
    {
      const rfc4566::MediaDescription& media = sdp.getMediaDescription(i);
      if (processedMids.find(media.getMid()) == processedMids.end())
      {
        int32_t index = rSdp.getGroupIndex(media.getMid());
        if (index != -1)
        {
          std::vector<rfc4566::MediaDescription> mediaDescriptions;
          InterfaceDescriptions_t mediaInterfaces;

          // process entire group
          const rfc5888::Group& group = sdp.getGroup(index);
          for (const std::string& sMid : group.getMids())
          {
            if (processedMids.find(sMid) == processedMids.end())
            {
              int mediaIndex = sdp.getMediaDescriptionIndex(sMid);
              assert(mediaIndex != -1);
              mediaDescriptions.push_back(sdp.getMediaDescription(mediaIndex));
              processedMids.insert(sMid);
            }
          }
          // create Rtp session parameters for group
          RtpSessionParametersGroup_t rtpParameters = createRtpSessionParametersGroup(sdesInfo, sdp, mediaDescriptions, mediaInterfaces, eSdpDescriptionType);
          assert(!rtpParameters.empty());
          m_vGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, GROUPED));
        }
        else
        {
          // process individual media line
          processedMids.insert(media.getMid());
          std::string sSessionLevelIp = sdp.getConnection().ConnectionAddress;
          RtpSessionParameters rtpParameters = createRtpSessionParameters(sdesInfo, sSessionLevelIp, sdp.getMediaDescription(i), addresses.at(i), eSdpDescriptionType);
          m_vNonGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vNonGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, NON_GROUPED));
        }
      }
    }
#endif
  }
  else
  {
    // create RtpSessionParameters for entire group
    m_vNonGroupedRtpSessionParameters = createRtpSessionParametersGroup(rfc5888::Group(), sdesInfo, rSdp, rSdp.getMediaDescriptions(), vTransports, eSdpDescriptionType);
    for (size_t i = 0; i < m_vNonGroupedRtpSessionParameters.size(); ++i)
    {
      m_mediaOrder.push_back(std::make_pair(i, NON_GROUPED));
    }
  }
}

MediaSessionDescription::MediaSessionDescription(const rfc3550::SdesInformation& sdesInfo,
                                                 const rfc4566::SessionDescription& localSdp,
                                                 const rfc4566::SessionDescription& remoteSdp)
{
  // NB:: assuming SDPs are compatible!
  // cast away const for map access via []
  rfc4566::SessionDescription& lSdp = const_cast<rfc4566::SessionDescription&>(localSdp);
  rfc4566::SessionDescription& rSdp = const_cast<rfc4566::SessionDescription&>(remoteSdp);

  // set to store mids that have already been processed in a group
  const std::string sLocalSessionLevelIp = lSdp.getConnection().ConnectionAddress;
  VLOG(6) << "Local SDP: connection: " << sLocalSessionLevelIp;
  const std::string sRemoteSessionLevelIp = rSdp.getConnection().ConnectionAddress;
  VLOG(6) << "Remote SDP: connection: " << sRemoteSessionLevelIp;

  // NB: This code assumed that the SDPs match: no validation is being done
  if (localSdp.isGrouped())
  {
    std::set<std::string> processedMids;
    for (size_t i = 0; i < lSdp.getMediaDescriptionCount(); ++i)
    {
      const rfc4566::MediaDescription& media = lSdp.getMediaDescription(i);
      if (processedMids.find(media.getMid()) == processedMids.end())
      {
        int32_t index = lSdp.getGroupIndex(media.getMid());
        if (index != -1)
        {
          std::vector<rfc4566::MediaDescription> localMediaDescriptions;
          std::vector<rfc4566::MediaDescription> remoteMediaDescriptions;

          // process entire group
          const rfc5888::Group& group = lSdp.getGroup(index);
          for (const std::string& sMid : group.getMids())
          {
            if (processedMids.find(sMid) == processedMids.end())
            {
              int mediaIndex = lSdp.getMediaDescriptionIndex(sMid);
              assert(mediaIndex != -1);
              localMediaDescriptions.push_back(lSdp.getMediaDescription(mediaIndex));
              remoteMediaDescriptions.push_back(rSdp.getMediaDescription(mediaIndex));
              processedMids.insert(sMid);
            }
          }
          // create Rtp session parameters for group

          RtpSessionParametersGroup_t rtpParameters = createRtpSessionParametersGroup(group, sdesInfo, sLocalSessionLevelIp, localMediaDescriptions, sRemoteSessionLevelIp, remoteMediaDescriptions);
          assert(!rtpParameters.empty());
          m_vGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, GROUPED));
        }
        else
        {
          // process individual media line
          processedMids.insert(media.getMid());
          RtpSessionParameters rtpParameters = createRtpSessionParameters(sdesInfo, sLocalSessionLevelIp, lSdp.getMediaDescription(i), sRemoteSessionLevelIp, rSdp.getMediaDescription(i));
          m_vNonGroupedRtpSessionParameters.push_back(rtpParameters);
          uint32_t groupIndex = m_vNonGroupedRtpSessionParameters.size() - 1;
          m_mediaOrder.push_back(std::make_pair(groupIndex, NON_GROUPED));
        }
      }
    }
  }
  else
  {
    for (size_t i = 0; i < localSdp.getMediaDescriptionCount(); ++i)
    {
      m_vNonGroupedRtpSessionParameters.push_back(
            createRtpSessionParameters(sdesInfo, sLocalSessionLevelIp, lSdp.getMediaDescription(i), sRemoteSessionLevelIp, rSdp.getMediaDescription(i))
            );
      uint32_t groupIndex = m_vNonGroupedRtpSessionParameters.size() - 1;
      m_mediaOrder.push_back(std::make_pair(groupIndex, NON_GROUPED));
    }
  }
}

// search for media index: returns the media index of the first match and a null pointer if no match is found
boost::optional<MediaSessionDescription::MediaIndex_t> MediaSessionDescription::find(const std::string& sMediaType) const
{
  for (size_t i = 0; i < getTotalSessionParameterCount(); ++i)
  {
    const MediaIndex_t& index = m_mediaOrder.at(i);
    switch( index.second )
    {
      case MediaSessionDescription::GROUPED:
      {
        const RtpSessionParametersGroup_t& group = m_vGroupedRtpSessionParameters.at(index.first);
        // class needs to know how to create an appropriate group session manager object
        for (const auto& parameters : group)
        {
          if (parameters.getMediaType() == sMediaType)
            return boost::optional<MediaIndex_t>(index);
        }
        break;
      }
      case MediaSessionDescription::NON_GROUPED:
      {
        const RtpSessionParameters& parameters = m_vNonGroupedRtpSessionParameters.at(index.first);
        if (parameters.getMediaType() == sMediaType)
          return boost::optional<MediaIndex_t>(index);
        break;
      }
    }
  }
  return boost::optional<MediaIndex_t>();
}

}
