#include "CorePch.h"
#include <rtp++/rfc3550/RtpMediaParameters.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <cpputil/Conversion.h>
#include <cpputil/Utility.h>

#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4145/Rfc4145.h>

std::unique_ptr<RtpMediaParameters>
RtpMediaParameters::createRtpParameters(const rfc4566::MediaDescription& media)
{
  return std::unique_ptr<RtpSessionParameters>(new RtpMediaParameters(media));
}

RtpMediaParameters::RtpMediaParameters()
  :m_uiSessionBandwidthKbps(0),
  m_bTcp(false),
  m_bActive(false),
  m_uiRtxPt(0),
  m_uiRtxApt(0),
  m_uiRtxTime(0)
{
}

RtpMediaParameters::RtpMediaParameters(const rfc4566::MediaDescription& media)
  :m_uiSessionBandwidthKbps(media.SessionBandwidth),
  m_bTcp(false),
  m_bActive(false),
  m_uiRtxPt(0),
  m_uiRtxApt(0),
  m_uiRtxTime(0)
{
  parseMediaDescription(media);
}

bool RtpMediaParameters::addPayloadType(const uint8_t uiPayloadType, const std::string& sCodec, const uint32_t uiTimestampFrequency)
{
  // just to be sure
  if (m_mCodecs.find(uiPayloadType) == m_mCodecs.end())
  {
    m_payloadTypes.push_back(uiPayloadType);
    m_mCodecs[uiPayloadType] = sCodec;
    m_mClockRates[uiPayloadType] = uiTimestampFrequency;
    return true;
  }
  return false;
}

bool RtpMediaParameters::hasAttribute(const std::string& sAttributeName) const
{
  return (m_mAttributes.find(sAttributeName) != m_mAttributes.end());
}

std::vector<std::string> RtpMediaParameters::getAttributeValues(const std::string& sAttributeName) const
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

uint8_t RtpMediaParameters::getPayloadType() const
{
  return m_payloadTypes[0];
}

std::vector<uint8_t> RtpMediaParameters::getPayloadTypes() const
{
  return m_payloadTypes;
}

uint32_t RtpMediaParameters::getRtpTimestampFrequency() const
{
  return m_mClockRates.at(m_payloadTypes.at(0));
}

std::string RtpMediaParameters::getMimeType() const
{
  return m_mCodecs.at(m_payloadTypes.at(0));
}

void RtpMediaParameters::addLocalEndPoint(const EndPointPair_t& endPoints)
{
  m_vLocalEndPoints.push_back(endPoints);
  m_vLocalRtpEndPoints.push_back(endPoints.first);
  m_vLocalRtcpEndPoints.push_back(endPoints.second);
}

void RtpMediaParameters::addRemoteEndPoint(const EndPointPair_t& endPoints)
{
  m_vRemoteEndPoints.push_back(endPoints);
  m_vRemoteRtpEndPoints.push_back(endPoints.first);
  m_vRemoteRtcpEndPoints.push_back(endPoints.second);
}

void RtpMediaParameters::setRtxParameters(uint8_t uiPt, uint8_t uiApt, uint32_t uiRtxTime)
{
  m_uiRtxPt = uiPt;
  m_uiRtxApt = uiApt;
  m_uiRtxTime = uiRtxTime;
}

void RtpMediaParameters::addFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage)
{
  m_mFeedback[uiPayload].push_back(sMessage);
  m_feedbackMessageTypes.insert(sMessage);
}

void RtpMediaParameters::addFeedbackMessages(const uint8_t uiPayload, const std::vector<std::string>& vMessages)
{
  m_mFeedback[uiPayload].insert(m_mFeedback[uiPayload].end(), vMessages.begin(), vMessages.end());
  for (size_t i = 0; i < vMessages.size(); ++i )
  {
    m_feedbackMessageTypes.insert(vMessages[i]);
  }
}

bool RtpMediaParameters::supportsFeedbackMessage(const uint8_t uiPayload, const std::string& sMessage)
{
  auto it = m_mFeedback.find(uiPayload);
  if (it == m_mFeedback.end()) return false;

  std::vector<std::string>& vMessages = it->second;
  for (size_t i = 0; i < vMessages.size(); ++i )
  {
    if (vMessages[i] == sMessage) return true;
  }
  return false;
}

bool RtpMediaParameters::supportsFeedbackMessage(const std::string& sMessage)
{
  auto it = m_feedbackMessageTypes.find(sMessage);
  return (it != m_feedbackMessageTypes.end());
}

void RtpMediaParameters::parseMediaDescription(const rfc4566::MediaDescription& media)
{
  // we only supporting RTP
  if (boost::algorithm::starts_with(media.Protocol, "RTP/") )
  {
    m_sProfile = media.Protocol.substr(4);
  }
  else if (boost::algorithm::starts_with(media.Protocol, "TCP/RTP/") )
  {
    m_sProfile = media.Protocol.substr(8);
    // double check that the status has been set before setting TCP
    if (rfc4145::isActive(media))
    {
      m_bActive = true;
      m_bTcp = true;
    }
    else if (rfc4145::isPassive(media))
    {
      m_bActive = false;
      m_bTcp = true;
    }
    else
    {
      // has to be set!
      assert(false);
    }
  }

  for (size_t i = 0; i < media.Formats.size(); ++i)
  {
    uint8_t uiPt = convert<uint8_t>(media.Formats[i], 0);
    m_payloadTypes.push_back(uiPt);
    m_mCodecs[uiPt] = media.getMimeType(media.Formats[i]);
    m_mClockRates[uiPt] = media.getClockRate(media.Formats[i]);
  }

  for ( auto it = media.AttributeList.begin(); it != media.AttributeList.end(); ++it)
  {
    m_mAttributes.insert(std::make_pair(*it, ""));
  }

  for (auto it = media.AttributeMap.begin(); it != media.AttributeMap.end(); ++it)
  {
    m_mAttributes.insert(std::make_pair(it->first, it->second));
  }

  m_sMid = media.Mid;
}


