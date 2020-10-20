#include "CorePch.h"
#include <rtp++/rfc4566/MediaDescription.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3611/Rfc3611.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc4585/Rfc4585.h>
#include <rtp++/rfc5285/Rfc5285.h>
#include <rtp++/rfc5888/Rfc5888.h>

namespace rtp_plus_plus
{
namespace rfc4566
{

RtpMapping::RtpMapping()
{

}

RtpMapping::RtpMapping(const std::string& sEncodingName, const std::string& sClockRate, const std::string& sEncodingParameters)
  :EncodingName(sEncodingName),
  ClockRate(sClockRate),
  EncodingParameters(sEncodingParameters)
{

}

FormatDescription::FormatDescription()
{

}

FormatDescription::FormatDescription(const std::string& sFmtp)
  :fmtp(sFmtp)
{

}

ConnectionData::ConnectionData()
  :InternetType("IN"),
  AddressType("IP4"),
  ConnectionAddress("0.0.0.0")
{

}

ConnectionData::ConnectionData(const std::string& sAddress)
  :InternetType("IN"),
  AddressType("IP4"),
  ConnectionAddress(sAddress)
{

}

bool ConnectionData::isSet() const
{
  return ConnectionAddress != "0.0.0.0" && ConnectionAddress != "";
}

std::ostream& operator<< (std::ostream& ostr, const ConnectionData& connection)
{
  ostr << connection.InternetType << " " << connection.AddressType << " " << connection.ConnectionAddress;
  return ostr;
}

MediaDescription::MediaDescription()
  :Port(0),
    SessionBandwidth(0),
    m_uiIndex(0)
{

}

MediaDescription::MediaDescription(const std::string& sMediaType, uint16_t uiPort, const std::string& sProtocol)
  :MediaType(sMediaType),
    Port(uiPort),
    Protocol(sProtocol),
    SessionBandwidth(0),
    m_uiIndex(0)
{

}

void MediaDescription::setProtocol(const std::string& sProfile, const std::string& sLowerLayerTransport)
{
  std::ostringstream ostr;
  std::string sLowerTransport = boost::to_upper_copy(sLowerLayerTransport);
  if (sLowerTransport != rfc3550::UDP)
    ostr << sLowerTransport << "/";
  ostr << "RTP/" << boost::to_upper_copy(sProfile);
  Protocol = ostr.str();
}

std::string MediaDescription::toString() const
{
  assert(!Protocol.empty());
  std::ostringstream ostr;
  // m-line
  ostr << "m=" << MediaType << " " << Port << " " << Protocol << " " << ::toString(Formats, ' ') << "\r\n";
  // i-line
  if (!MediaTitle.empty())
    ostr << "i=" << MediaTitle << "\r\n";
  // c-line if present0
  if (Connection.isSet())
  {
    ostr << "c=" << Connection << "\r\n";
  }
  // b-line if present
  if (SessionBandwidth > 0)
  {
    ostr << "b=AS:" << SessionBandwidth << "\r\n";
  }

  for (auto& format : Formats)
  {
    // rtp-map
    auto it = RtpMap.find(format);
    if (it != RtpMap.end())
    {
      ostr << "a=rtpmap:" << format << " " << it->second.EncodingName << "/" << it->second.ClockRate;
      if (!it->second.EncodingParameters.empty())
      {
        ostr << " " << it->second.EncodingParameters;
      }
      ostr << "\r\n";
    }
    // fmtp
    auto fmtpIt = FormatMap.find(format);
    if (fmtpIt != FormatMap.end())
    {
      ostr << "a=fmtp:" << format << " " << fmtpIt->second.fmtp << "\r\n";
    }
    // rtcp-fb
    auto fbIt = FeedbackMap.find(format);
    if (fbIt != FeedbackMap.end())
    {
      VLOG(2) << "Outputting feedback line: Format: " << format << " fb types: " << ::toString(fbIt->second.FeedbackTypes);
      ostr << "a=rtcp-fb:" << format << " " << ::toString(fbIt->second.FeedbackTypes) << "\r\n";
    }
  }

  for (auto it = AttributeMap.begin(); it != AttributeMap.end(); ++it)
  {
    if (it->first == mprtp::MPRTP)
    {
      // we need to explicitly write mprtp because of the space following the attribute name
      ostr << "a=mprtp";
      if (!it->second.empty())
        ostr << " " << it->second;
      ostr << "\r\n";
    }
    else if ((it->first != RTPMAP) &&
      (it->first != FMTP) &&
      (it->first != rfc4585::RTCP_FB) &&
      (it->first != rfc5888::MID)
      )
    {
      ostr << "a=" << it->first;
      if (!it->second.empty())
        ostr << ":" << it->second;
      ostr << "\r\n";
    }
  }    
  
  // add MID as last field
  if (!Mid.empty())
    ostr << "a=mid:" << Mid << "\r\n";
  return ostr.str();
}

void MediaDescription::processKnownAttributes()
{
  // iterate over media level attributes
  for (auto it = AttributeMap.begin(); it != AttributeMap.end(); ++it)
  {
    if (it->first == RTPMAP)
    {
      std::string sValue = it->second;
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sValue, " /");
      RtpMap[vTokens[0]].EncodingName = vTokens[1];
      RtpMap[vTokens[0]].ClockRate = vTokens[2];
    }
    else if (it->first == FMTP)
    {
      std::string sValue = it->second;
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sValue, " ");
      FormatMap[vTokens[0]].fmtp = vTokens[1];
    }
    else if (it->first == rfc4585::RTCP_FB)
    {
      std::string sValue = it->second;
      std::vector<std::string> vTokens = StringTokenizer::tokenize(sValue, " ");
      auto itTemp = vTokens.begin();
      itTemp++;
      FeedbackMap[vTokens[0]].FeedbackTypes.insert(FeedbackMap[vTokens[0]].FeedbackTypes.begin(), itTemp, vTokens.end());
    }
    else if (it->first == rfc3611::RTCP_XR)
    {
      /* RFC3611
        rcvr-rtt       = "rcvr-rtt" "=" rcvr-rtt-mode [":" max-size]
        rcvr-rtt-mode  = "all"
                       / "sender"
        */
      std::vector<std::string> vTokens = StringTokenizer::tokenize(it->second, " ");
      LOG(INFO) << "Found RTCP-XR attribute";
      for (std::string sToken: vTokens)
      {
        size_t posEquals = sToken.find("=");
        if (posEquals == std::string::npos)
        {
          XrMap.insert(std::make_pair(sToken, ""));
        }
        else
        {
          XrMap.insert(std::make_pair(sToken.substr(0, posEquals), sToken.substr(posEquals + 1)));
        }
      }
    }
    else if (it->first == rfc5888::MID)
    {
      Mid = it->second;
    }
  }
}

void MediaDescription::addFormat(const std::string& sFormat)
{
  // TODO: add check to see if format already exists
  Formats.push_back(sFormat);
}

void MediaDescription::addFormat(const std::string& sFormat, const RtpMapping& rtpMap, const FormatDescription& fmtp)
{
  // TODO: add check to see if format already exists
  Formats.push_back(sFormat);
  RtpMap[sFormat] = rtpMap;
  FormatMap[sFormat] = fmtp;
  // When "manually" creating SDP, we still need to add formats to attribute map

  std::ostringstream fmtpStr;
  fmtpStr << sFormat << " " << fmtp.fmtp;
  addAttribute(FMTP, fmtpStr.str());

  std::ostringstream rtpMapStr;
  rtpMapStr << sFormat << " " << rtpMap.EncodingName << "/" << rtpMap.ClockRate;
  addAttribute(RTPMAP, rtpMapStr.str());
  // TODO: do we need to add encoding parameters?!?
}

void MediaDescription::removeFormat(const std::string& sFormat)
{
  auto it = std::find(Formats.begin(), Formats.end(), sFormat);
  if (it != Formats.end())
  {
    Formats.erase(it);
  }
  auto it2 = RtpMap.find(sFormat);
  if (it2 != RtpMap.end())
  {
    RtpMap.erase(it2);
  }
  auto it3 = FormatMap.find(sFormat);
  if (it3 != FormatMap.end())
  {
    FormatMap.erase(it3);
  }
}

bool MediaDescription::hasAttribute(const std::string& sAttributeName) const
{
  auto it = AttributeMap.find(sAttributeName);
  return (it != AttributeMap.end());
}

boost::optional<std::string> MediaDescription::getAttributeValue(const std::string& sAttributeName) const
{
  auto it = AttributeMap.find(sAttributeName);
  if (it != AttributeMap.end())
    return boost::optional<std::string>(it->second);
  return boost::optional<std::string>();
}

std::vector<std::string> MediaDescription::getAttributeValues(const std::string& sAttributeName ) const
{
  typedef std::multimap<std::string, std::string> MultiStringMap_t;
  std::vector<std::string> res;
  std::pair<MultiStringMap_t::const_iterator, MultiStringMap_t::const_iterator> ret;
  ret = AttributeMap.equal_range(sAttributeName);
  for (MultiStringMap_t::const_iterator it = ret.first; it != ret.second; ++it)
  {
    res.push_back(it->second);
  }
  return res;
}

void MediaDescription::addAttribute(const std::string& sName, const std::string& sValue)
{
  AttributeMap.insert(make_pair(sName, sValue));
}

size_t MediaDescription::removeAttribute(const std::string& sName)
{
  size_t count = AttributeMap.erase(sName);
  return count;
}

boost::optional<std::string> MediaDescription::getXrValue(const std::string& sAttributeName)
{
  auto it = XrMap.find(sAttributeName);
  if (it != XrMap.end())
    return boost::optional<std::string>(it->second);
  else
  {
    return boost::optional<std::string>();
  }
}

uint32_t MediaDescription::getClockRate(const std::string& sFormat) const
{
  auto it = RtpMap.find(sFormat);
  if (it != RtpMap.end())
  {
    return convert<uint32_t>(it->second.ClockRate, 0);
  }
  return 0;
}

std::string MediaDescription::getMimeType(const std::string& sFormat) const
{
  auto it = RtpMap.find(sFormat);
  if (it != RtpMap.end())
  {
    return MediaType + "/" + it->second.EncodingName;
  }
  return "";
}

std::string MediaDescription::getMimeType() const
{
  auto it = RtpMap.find(*(Formats.begin()));
  if (it != RtpMap.end())
  {
    return MediaType + "/" + it->second.EncodingName;
  }
  return "";
}

std::string MediaDescription::getEncodingName(const std::string& sFormat) const
{
  auto it = RtpMap.find(sFormat);
  if (it != RtpMap.end())
  {
    return it->second.EncodingName;
  }
  return "";
}

std::string MediaDescription::getEncodingName() const
{
  auto it = RtpMap.find(*(Formats.begin()));
  if (it != RtpMap.end())
  {
    return it->second.EncodingName;
  }
  return "";
}

bool MediaDescription::lookupExtmap(const std::string& sUrn, uint32_t& uiExtmapId) const
{
  const std::vector<std::string> vExtmap = getAttributeValues(rfc5285::EXTMAP);
  for (const std::string& sExtmapValue : vExtmap)
  {
    boost::optional<rfc5285::Extmap> extmap = rfc5285::Extmap::parseFromSdpAttribute(sExtmapValue);
    if (extmap)
    {
      if (extmap->getExtensionName() == sUrn)
      {
        uiExtmapId = extmap->getId();
        return true;
      }
    }
  }
  return false;
}

std::map<uint32_t, std::string> MediaDescription::getExtMap() const
{
  std::map<uint32_t, std::string> map;
  const std::vector<std::string> vExtmap = getAttributeValues(rfc5285::EXTMAP);
  for (const std::string& sExtmapValue : vExtmap)
  {
    boost::optional<rfc5285::Extmap> extmap = rfc5285::Extmap::parseFromSdpAttribute(sExtmapValue);
    if (extmap)
    {
      map[extmap->getId()] = extmap->getExtensionName();
    }
  }
  return map;
}

std::ostream& operator<< (std::ostream& ostr, const MediaDescription& media)
{
  ostr << media.toString();
  return ostr;
}


} // rfc4566
} // rtp_plus_plus
