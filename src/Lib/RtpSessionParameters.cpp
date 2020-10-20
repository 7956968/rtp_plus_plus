#include "CorePch.h"
#include <rtp++/RtpSessionParameters.h>
#include <cassert>
#include <cstddef>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/Utility.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3550/SdesInformation.h>
#include <rtp++/rfc3611/Rfc3611.h>
#include <rtp++/rfc3711/Rfc3711.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4145/Rfc4145.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc4588/Rfc4588.h>
#include <rtp++/rfc5285/Rfc5285.h>

namespace rtp_plus_plus
{

std::unique_ptr<RtpSessionParameters>
RtpSessionParameters::createRtpParameters(const rfc3550::SdesInformation& sdesInfo)
{
  RtpSessionParameters* pParameters = new RtpSessionParameters(sdesInfo);

  VLOG(2) << "RTP session parameters:"
            << " Profile: " << pParameters->getProfile()
            << " Clock: " << pParameters->getRtpTimestampFrequency() << " Hz";

  return std::unique_ptr<RtpSessionParameters>(pParameters);
}

std::unique_ptr<RtpSessionParameters>
RtpSessionParameters::createRtpParameters(const rfc3550::SdesInformation& sdesInfo,
                                          const rfc4566::MediaDescription& media,
                                          bool bLocalMediaDescription)
{
  RtpSessionParameters* pParameters = new RtpSessionParameters(sdesInfo, media, bLocalMediaDescription);

  VLOG(2) << "RTP session parameters:"
            << " Profile: " << pParameters->getProfile()
            << " Clock: " << pParameters->getRtpTimestampFrequency() << " Hz";
  return std::unique_ptr<RtpSessionParameters>(pParameters);
}

std::unique_ptr<RtpSessionParameters>
RtpSessionParameters::createRtpParameters(const rfc3550::SdesInformation& sdesInfo,
                                          const rfc4566::MediaDescription& media,
                                          const rfc2326::Transport& transport,
                                          bool bLocalMediaDescription)
{
  RtpSessionParameters* pParameters = new RtpSessionParameters(sdesInfo, media, transport, bLocalMediaDescription);

  VLOG(2) << "RTP session parameters:"
            << " Profile: " << pParameters->getProfile()
            << " Clock: " << pParameters->getRtpTimestampFrequency() << " Hz";
  return std::unique_ptr<RtpSessionParameters>(pParameters);
}

RtpSessionParameters::RtpSessionParameters()
  :m_uiSessionBandwidthKbps(0),
    m_uiMediaIndexInSdp(UINT32_MAX),
    m_iPayloadTypeIndex(-1),
    m_protocol(RTP_UDP),
    m_bActive(false),
    m_uiRtxPt(0),
    m_uiRtxApt(0),
    m_uiRtxTime(0)
{

}

RtpSessionParameters::RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo)
  :m_sdesInfo(sdesInfo),
    m_uiSessionBandwidthKbps(0),
    m_uiMediaIndexInSdp(UINT32_MAX),
    m_iPayloadTypeIndex(-1),
    m_protocol(RTP_UDP),
    m_bActive(false),
    m_uiRtxPt(0),
    m_uiRtxApt(0),
    m_uiRtxTime(0)
{

}

RtpSessionParameters::RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                                           const rfc4566::MediaDescription& media,
                                           bool bLocalMediaDescription)
  :m_sdesInfo(sdesInfo),
    m_uiSessionBandwidthKbps(media.getSessionBandwidth()),
    m_uiMediaIndexInSdp(media.getIndexInSdp()),
    m_iPayloadTypeIndex(-1),
    m_protocol(RTP_UDP),
    m_bActive(false),
    m_uiRtxPt(0),
    m_uiRtxApt(0),
    m_uiRtxTime(0)
{
  parseMediaDescription(media, bLocalMediaDescription);
}

RtpSessionParameters::RtpSessionParameters(const rfc3550::SdesInformation& sdesInfo,
                     const rfc4566::MediaDescription& media,
                     const rfc2326::Transport& transport,
                     bool bLocalMediaDescription)
  :m_sdesInfo(sdesInfo),
    m_uiSessionBandwidthKbps(media.getSessionBandwidth()),
    m_uiMediaIndexInSdp(media.getIndexInSdp()),
    m_transport(transport),
    m_iPayloadTypeIndex(-1),
    m_protocol(RTP_UDP),
    m_bActive(false),
    m_uiRtxPt(0),
    m_uiRtxApt(0),
    m_uiRtxTime(0)
{
  parseMediaDescription(media, bLocalMediaDescription);

  if (transport.isInterleaved())
  {
    // TOREVISE
    // Adding dummy remote endpoint since this is required by the framework
    // Could add endp point with remote address of server but there is no way to get
    // the port used from here so it might make more sense just to use the  0 values
    VLOG(5) << "Adding dummy remote host address and port since connection is reused in any case";
    EndPoint ep("0.0.0.0", 0);
    if (!transport.getSource().empty())
      ep.setAddress(transport.getSource());
    addRemoteEndPoint(std::make_pair(ep, ep));
  }
}


bool RtpSessionParameters::addPayloadType(const uint8_t uiPayloadType, const std::string& sCodec, const uint32_t uiTimestampFrequency)
{
  // just to be sure
  if (m_mCodecs.find(uiPayloadType) == m_mCodecs.end())
  {
    m_payloadTypes.push_back(uiPayloadType);
    m_mCodecs[uiPayloadType] = sCodec;
    m_mClockRates[uiPayloadType] = uiTimestampFrequency;
    // update default index
    m_iPayloadTypeIndex = 0;
    return true;
  }
  return false;
}

bool RtpSessionParameters::hasAttribute(const std::string& sAttributeName) const
{
  return (m_mAttributes.find(sAttributeName) != m_mAttributes.end());
}

std::vector<std::string> RtpSessionParameters::getAttributeValues(const std::string& sAttributeName) const
{
  typedef std::multimap<std::string, std::string> MultiStringMap_t;
  std::vector<std::string> res;
  std::pair<MultiStringMap_t::const_iterator, MultiStringMap_t::const_iterator> ret;
  ret = m_mAttributes.equal_range(sAttributeName);
  for (MultiStringMap_t::const_iterator it = ret.first; it != ret.second; ++it)
  {
    res.push_back(it->second);
  }
  return res;
}

uint8_t RtpSessionParameters::getPayloadType() const
{
  if (m_iPayloadTypeIndex == -1) return 0xFF;
  return m_payloadTypes[m_iPayloadTypeIndex];
}

std::vector<uint8_t> RtpSessionParameters::getPayloadTypes() const
{
  return m_payloadTypes;
}

bool RtpSessionParameters::updatePayloadType(uint8_t uiPt)
{
  for (size_t i = 0; i < m_payloadTypes.size(); ++i)
  {
    if (uiPt == m_payloadTypes[i])
    {
      m_iPayloadTypeIndex = i;
      return true;
    }
  }
  return false;
}

uint32_t RtpSessionParameters::getRtpTimestampFrequency() const
{
  if (m_iPayloadTypeIndex == -1) return 0;
  return m_mClockRates.at(m_payloadTypes.at(m_iPayloadTypeIndex));
}

std::string RtpSessionParameters::getEncodingName() const
{
  if (m_iPayloadTypeIndex == -1) return "";
  return m_mCodecs.at(m_payloadTypes.at(m_iPayloadTypeIndex));
}

void RtpSessionParameters::addLocalEndPoint(const EndPointPair_t& endPoints)
{
  m_vLocalEndPoints.push_back(endPoints);
  m_vLocalRtpEndPoints.push_back(endPoints.first);
  m_vLocalRtcpEndPoints.push_back(endPoints.second);
}

void RtpSessionParameters::addLocalEndPoints(const std::vector<EndPointPair_t>& endPoints)
{
  for (const EndPointPair_t& endPointPair : endPoints)
  {
    addLocalEndPoint(endPointPair);
  }
}

void RtpSessionParameters::addRemoteEndPoint(const EndPointPair_t& endPoints)
{
  m_vRemoteEndPoints.push_back(endPoints);
  m_vRemoteRtpEndPoints.push_back(endPoints.first);
  m_vRemoteRtcpEndPoints.push_back(endPoints.second);
}

void RtpSessionParameters::addRemoteEndPoints(const std::vector<EndPointPair_t>& endPoints)
{
  for (const EndPointPair_t& endPointPair : endPoints)
  {
    addRemoteEndPoint(endPointPair);
  }
}

void RtpSessionParameters::setRtxParameters(uint8_t uiPt, uint8_t uiApt, uint32_t uiRtxTime)
{
  // TODO: could add check to see if PT and APT are valid
  m_uiRtxPt = uiPt;
  m_uiRtxApt = uiApt;
  m_uiRtxTime = uiRtxTime;
}

void RtpSessionParameters::addFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage)
{
  VLOG(2) << "Adding feedback message for PT: " << (int) uiPayload << ": " << sMessage;
  m_mFeedback[uiPayload].push_back(sMessage);
  m_feedbackMessageTypes.insert(sMessage);
}

void RtpSessionParameters::addFeedbackMessages(const uint8_t uiPayload, const std::vector<std::string>& vMessages)
{
  VLOG(2) << "Adding feedback message for PT: " << (int) uiPayload << ": " << ::toString(vMessages);
  m_mFeedback[uiPayload].insert(m_mFeedback[uiPayload].end(), vMessages.begin(), vMessages.end());
  for (size_t i = 0; i < vMessages.size(); ++i )
  {
    m_feedbackMessageTypes.insert(vMessages[i]);
  }
}

bool RtpSessionParameters::supportsFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage) const
{
  auto it = m_mFeedback.find(uiPayload);
  if (it == m_mFeedback.end()) return false;

  const std::vector<std::string>& vMessages = it->second;
  for (size_t i = 0; i < vMessages.size(); ++i )
  {
    if (vMessages[i] == sMessage) return true;
  }
  return false;
}

bool RtpSessionParameters::supportsFeedbackMessage(const std::string& sMessage) const
{
  std::ostringstream ostr;
  ostr << "Supported feedback messages: (";
  for (auto& s : m_feedbackMessageTypes)
  {
    ostr << s << ", ";
  }
  ostr << ")";
  VLOG(2) << ostr.str();
  auto it = m_feedbackMessageTypes.find(sMessage);
  return (it != m_feedbackMessageTypes.end());
}

void RtpSessionParameters::parseMediaDescription(const rfc4566::MediaDescription& media, bool bLocalMediaDescription)
{
  m_sMediaType = media.getMediaType();

  if (media.hasAttribute(rfc5888::MID))
  {
    m_sMid = *media.getAttributeValue(rfc5888::MID);
  }

  // we only supporting RTP
  if (!boost::algorithm::contains(media.getProtocol(), "RTP"))
  {
    VLOG(2) << "Unsupported protocol: " << media.getProtocol();
    return;
  }

  // start with longest comparisons to get profile
  if (boost::algorithm::contains(media.getProtocol(), rfc3711::SAVPF))
  {
    m_sProfile = rfc3711::SAVPF;
  }
  else if (boost::algorithm::contains(media.getProtocol(), rfc3711::SAVP))
  {
    m_sProfile = rfc3711::SAVP;
  }
  else if (boost::algorithm::contains(media.getProtocol(), rfc4585::AVPF))
  {
    m_sProfile = rfc4585::AVPF;
  }
  else if (boost::algorithm::contains(media.getProtocol(), rfc3550::AVP))
  {
    m_sProfile = rfc3550::AVP;
  }

  // transport layer protocol
  if (boost::algorithm::contains(media.getProtocol(), "TCP"))
  {
    VLOG(5) << "Transport layer TCP";
    // double check that the status has been set before setting TCP
    if (rfc4145::isActive(media))
    {
      m_bActive = true;
    }
    else if (rfc4145::isPassive(media))
    {
      m_bActive = false;
    }
    else
    {
      // has to be set!
      assert(false);
    }
    if (!bLocalMediaDescription)
      m_bActive = !m_bActive;

    m_protocol = RTP_TCP;
  }
  else if (boost::algorithm::contains(media.getProtocol(), "DCCP"))
  {
    VLOG(5) << "Transport layer DCCP";

    // double check that the status has been set before setting DCCP
    if (rfc4145::isActive(media))
    {
      m_bActive = true;
    }
    else if (rfc4145::isPassive(media))
    {
      m_bActive = false;
    }
    else
    {
      // has to be set!
      assert(false);
    }
    if (!bLocalMediaDescription)
      m_bActive = !m_bActive;

    m_protocol = RTP_DCCP;
  }
  else if (boost::algorithm::contains(media.getProtocol(), "SCTP"))
  {
    VLOG(5) << "Transport layer SCTP";

    // double check that the status has been set before setting SCTP
    if (rfc4145::isActive(media))
    {
      m_bActive = true;
    }
    else if (rfc4145::isPassive(media))
    {
      m_bActive = false;
    }
    else
    {
      // has to be set!
      assert(false);
    }
    if (!bLocalMediaDescription)
      m_bActive = !m_bActive;

    m_protocol = RTP_SCTP;
  }
  else // if (boost::algorithm::contains(media.getProtocol(), "UDP"))
  {
    VLOG(5) << "Transport layer UDP";
  }
 
  for (size_t i = 0; i < media.getFormats().size(); ++i)
  {
    uint8_t uiPt = convert<uint8_t>(media.getFormat(i), 0);
    m_payloadTypes.push_back(uiPt);
    m_mCodecs[uiPt] = media.getEncodingName(media.getFormat(i));
    m_mClockRates[uiPt] = media.getClockRate(media.getFormat(i));
  }

  const std::multimap<std::string, std::string>& attributeMap = media.getAttributes();
  for (auto it = attributeMap.begin(); it != attributeMap.end(); ++it)
  {
    m_mAttributes.insert(std::make_pair(it->first, it->second));
  }

  // RTX
  processRetransmissionParameters(media);
  // XR
  processXrParameters(media);
  // extmap
  processExtmapParameters(media);

  // update index
  if (!m_payloadTypes.empty())
    m_iPayloadTypeIndex = 0;

}

void RtpSessionParameters::processRetransmissionParameters(const rfc4566::MediaDescription& media)
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
    // process retransmission lines
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
          setRtxParameters(uiPt, uiApt, uiRtxTime);
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
          VLOG(5) << "Adding " << vFeedbackMessages.size() << " feedback messages: " << ::toString(vFeedbackMessages);
          addFeedbackMessages(uiPayload, vFeedbackMessages);
        }
      }
    }
    else
    {
      // process standard type
      bool bRes = true;
      uiPt = convert<uint8_t>(it->first, bRes);
      assert(bRes);
      LOG(INFO) << "Processing payload type: " << static_cast<uint32_t>(uiPt);
      VLOG(2) << "Retrieving feedback for payload type " << it->first;
      // check for FB specific messages
      auto itFb = media.getFeedbackMap().find(it->first);
      if (itFb != media.getFeedbackMap().end())
      {
        const std::vector<std::string>& vFeedbackMessages = itFb->second.FeedbackTypes;
        VLOG(5) << "Adding " << vFeedbackMessages.size() << " feedback messages: " << ::toString(vFeedbackMessages);
        addFeedbackMessages(uiPt, vFeedbackMessages);
      }
    }
  }
}

void RtpSessionParameters::processXrParameters(const rfc4566::MediaDescription& media)
{
  setXrParameters(media.getXrMap());
}

void RtpSessionParameters::processExtmapParameters(const rfc4566::MediaDescription& media)
{
  const std::vector<std::string> vExtmap = media.getAttributeValues(rfc5285::EXTMAP);
  for (const std::string& sExtmapValue: vExtmap)
  {
    boost::optional<rfc5285::Extmap> extmap = rfc5285::Extmap::parseFromSdpAttribute(sExtmapValue);
    if (extmap)
    {
      addExtMap(*extmap);
    }
  }
}

} // rtp_plus_plus
