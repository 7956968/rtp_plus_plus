#include "CorePch.h"
#include <rtp++/rfc4566/SessionDescription.h>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>

namespace rtp_plus_plus
{
namespace rfc4566
{

SessionDescription::SessionDescription()
  :ProtocolVersion("0")
{

}

SessionDescription::SessionDescription(const std::string& sSdp)
  : m_sOriginalSdp(sSdp),
  ProtocolVersion("0")
{

}

std::string SessionDescription::toString() const
{
  std::ostringstream ostr;
  ostr << "v=" << ProtocolVersion << "\r\n";
  ostr << "o=" << Origin << "\r\n";
  ostr << "s=" << SessionName << "\r\n";
  if (!SessionInfo.empty())
    ostr << "i=" << SessionInfo << "\r\n";
  if (!Uri.empty())
    ostr << "u=" << Uri << "\r\n";
  if (!Uri.empty())
    ostr << "u=" << Uri << "\r\n";
  for (auto& email : EmailFields)
    ostr << "e=" << email << "\r\n";
  for (auto& number : PhoneNumbers)
    ostr << "p=" << number << "\r\n";
  for (auto& timing : Timing)
    ostr << "t=" << timing << "\r\n";
  if (!Uri.empty())
    ostr << "u=" << Uri << "\r\n";
  if (!RepeatTimes.empty())
    ostr << "r=" << RepeatTimes << "\r\n";
  if (Connection.isSet())
  {
    ostr << "c=" << Connection << "\r\n";
  }
  // Attributes:
  for (auto it = AttributeMap.begin(); it != AttributeMap.end(); ++it)
  {
    ostr << "a=" << it->first;
    if (!it->second.empty())
      ostr << ":" << it->second;
    ostr << "\r\n";
  }

  // Media Lines
  for (const MediaDescription& media : MediaDescriptions)
  {
    ostr << media;
  }
  std::string sSdp = ostr.str();
#if 0
  // NB: trunc last \r\n
  return sSdp.substr(0, sSdp.length() - 2);
#else
  return sSdp;
#endif
}

std::string SessionDescription::getOriginalSdpString() const
{
  return m_sOriginalSdp;
}

void SessionDescription::process()
{
  // process m lines first, this will assign mid attributes
  for (MediaDescription& mediaDescription: MediaDescriptions)
  {
    mediaDescription.processKnownAttributes();
  }

  findUngroupedMediaDescriptions();
}

void SessionDescription::addAttribute(const std::string& sName, const std::string& sValue)
{
  AttributeMap.insert(make_pair(sName, sValue));
  // check for groups
  if (sName == rfc5888::GROUP)
  {
    std::vector<std::string> vTokens = StringTokenizer::tokenize(sValue, " ");
    std::string sSemantic = vTokens[0];
    vTokens.erase(vTokens.begin(), vTokens.begin() + 1);
    rfc5888::Group group(sSemantic, vTokens);
    Groups.push_back(group);
  }
}

bool SessionDescription::hasAttribute(const std::string& sAttributeName)
{
  return AttributeMap.find(sAttributeName) != AttributeMap.end();
}

boost::optional<std::string> SessionDescription::getAttributeValue(const std::string& sAttributeName) const
{
  auto it = AttributeMap.find(sAttributeName);
  if (it != AttributeMap.end())
    return boost::optional<std::string>(it->second);
  else
  {
    return boost::optional<std::string>();
  }
}

std::vector<std::string> SessionDescription::getAttributeValues(const std::string& sAttributeName ) const
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

int SessionDescription::getMediaDescriptionIndex(const std::string& sMid) const
{
  for (size_t i = 0; i < MediaDescriptions.size(); ++i)
  {
    if (MediaDescriptions[i].getMid() == sMid)
      return i;
  }
  return -1;
}

bool SessionDescription::isGrouped() const
{
  if (!Groups.empty())
  {
    // RFC5888 requires that all m-lines have a mid, otherwise no grouping should be performed
    for (const MediaDescription& media: MediaDescriptions)
    {
      if (!media.hasAttribute(rfc5888::MID))
      {
        LOG(WARNING) << "SDP has groups, but m line has no mid attribute";
        return false;
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}

std::vector<MediaDescription> SessionDescription::getMediaDescriptionsInGroup(uint32_t uiIndex) const
{
  std::vector<MediaDescription> vDescriptions;
  if (uiIndex < Groups.size())
  {
    const rfc5888::Group& group = Groups[uiIndex];
    std::vector<std::string> vMids = group.getMids();
    for (auto& mid: vMids)
    {
      int index = getMediaDescriptionIndex(mid);
      if (index == -1)
      {
        LOG(WARNING) << "Couldn't locate media description in SDP: " << mid;
      }
      else
      {
        assert((uint32_t)index < MediaDescriptions.size());
        vDescriptions.push_back(MediaDescriptions[index]);
      }
    }
  }
  return vDescriptions;
}

int SessionDescription::getGroupIndex(const std::string& sMid) const
{
  for (size_t i = 0; i < Groups.size(); ++i)
  {
    const rfc5888::Group& group = Groups[i];
    if (group.contains(sMid))
    {
      return i;
    }
  }
  return -1;
}

void SessionDescription::findUngroupedMediaDescriptions()
{
  // Determine which m lines are not grouped
  if (hasAttribute(rfc5888::GROUP))
  {
    // creates groups
    for (size_t i = 0; i < MediaDescriptions.size(); ++i)
    {
      if (!MediaDescriptions[i].hasAttribute(rfc5888::MID))
      {
        LOG(WARNING) << "Invalid media description: m line contains no mid attribute in SDP with group attribute";
      }
      else
      {
        if (getGroupIndex(MediaDescriptions[i].getMid()) == -1)
        {
          NonGrouped.push_back(MediaDescriptions[i].getMid());
        }
      }
    }
  }
}

void SessionDescription::updateAllConnectionAddresses(const std::string& sIpAddress)
{
  Connection.ConnectionAddress = sIpAddress;
  for (MediaDescription& media : MediaDescriptions)
  {
    ConnectionData connection = media.getConnection();
    connection.ConnectionAddress = sIpAddress;
    media.setConnection((connection));
  }
}

} // rfc4566
} // rtp_plus_plus
