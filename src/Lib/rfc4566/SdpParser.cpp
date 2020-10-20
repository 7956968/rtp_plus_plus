#include "CorePch.h"
#include <rtp++/rfc4566/SdpParser.h>

#define COMPONENT_LEVEL_VERBOSITY 10

namespace rtp_plus_plus
{
namespace rfc4566
{

bool validate(const SessionDescription& sdp)
{
  if (sdp.getProtocolVersion().empty())
  {
    VLOG(2) << "Invalid protocol version";
    return false;
  }
  if (sdp.getOrigin().empty())
  {
    VLOG(2) << "Invalid origin";
    return false;
  }
  // many SDPs seem to be missing the session name so
  // relax this constraint for now
#if 0
  if (sdp.getSessionName().empty())
  {
    VLOG(2) << "Invalid session name";
    return false;
  }
  if (sdp.getTiming().empty())
  {
    VLOG(2) << "Invalid timing";
    return false;
  }
#endif

  if (sdp.isGrouped())
  {
    // make sure that every m line has a mid
    for (size_t i = 0; i < sdp.getMediaDescriptionCount(); ++i)
    {
      const  MediaDescription& rMedia = sdp.getMediaDescription(i);
      if (!rMedia.hasAttribute(rfc5888::MID))
      {
        VLOG(2) << "No MID line in SDP";
        return false;
      }
    }
  }

  // TODO: add any other validation here
  return true;
}

boost::optional<SessionDescription> SdpParser::parse(const std::string& sSdp, bool bVerbose)
{
  VLOG(COMPONENT_LEVEL_VERBOSITY) << "Parsing SDP: " << sSdp;
  using boost::algorithm::trim;
  SessionDescription sdp;
  // parse sdp line by line
  std::string sLine;
  std::istringstream istr(sSdp);
  // flag if session or media is being parsed
  // formats for single media line
  std::vector<std::string> vFormats;

  bool bParsingMedia = false;

  int mediaIndexInSdp = -1;
  while (!getline(istr, sLine).eof())
  {
    VLOG(COMPONENT_LEVEL_VERBOSITY) << "Line: " << sLine;
    trim(sLine);
    // ignore empty lines for now: TODO: what does the spec say?
    if (sLine.length() == 0) continue;
    size_t pos = sLine.find("=");
    if (pos == std::string::npos)
    {
      LOG(WARNING) << "Invalid line, no '='";
      return boost::optional<SessionDescription>();
    }
    else
    {
      std::string sName = sLine.substr(0, pos);
      std::string sValue = sLine.substr(pos + 1);
      VLOG(COMPONENT_LEVEL_VERBOSITY) << "Name: " << sName << " Value: " << sValue;
      if (sName == "v")
      {
        sdp.setProtocolVersion(sValue);
      }
      else if (sName == "o")
      {
        sdp.setOrigin(sValue);
      }
      else if (sName == "s")
      {
        sdp.setSessionName(sValue);
      }
      else if (sName == "i")
      {
        if (!bParsingMedia)
        {
          sdp.setSessionInfo(sValue);
        }
        else
        {
          sdp.getLastAddedMediaDescription().setMediaTitle(sValue);
        }
      }
      else if (sName == "u")
      {
        sdp.setUri(sValue);
      }
      else if (sName == "e")
      {
        sdp.addEmailField(sValue);
      }
      else if (sName == "p")
      {
        sdp.addPhoneNumber(sValue);
      }
      else if (sName == "c")
      {
        std::istringstream istr(sValue);
        ConnectionData connection;
        istr >> connection.InternetType >> connection.AddressType >> connection.ConnectionAddress;
        if (!bParsingMedia)
        {
          sdp.setConnection(connection);
        }
        else
        {
          sdp.getLastAddedMediaDescription().setConnection(connection);
        }
      }
      else if (sName == "t")
      {
        sdp.addTiming(sValue);
      }
      else if (sName == "r")
      {
        sdp.setRepeatTimes(sValue);
      }
      else if (sName == "m")
      {
        bParsingMedia = true;
        vFormats.clear();
        ++mediaIndexInSdp;

        std::istringstream istr(sValue);
        std::string sMediaType, sProtocol, sFormat;
        uint16_t uiPort;
        istr >> sMediaType >> uiPort >> sProtocol;
        MediaDescription media(sMediaType, uiPort, sProtocol);
        media.setIndexInSdp(mediaIndexInSdp);
        while( !istr.eof())
        {
          istr >> sFormat;
          media.addFormat(sFormat);
        }
        sdp.addMediaDescription(media);
      }
      else if (sName == "a")
      {
        using boost::algorithm::trim;
        // handle generic attributes
        // some attributes begin with the tag COLON payload_format ...
        // others with tag SP ...
        size_t pos_colon = sValue.find(":");
        size_t pos_space = sValue.find(" ");

        std::string sKey;
        std::string sTrimmedValue;
        if (pos_colon != std::string::npos)
        {
          if ((pos_space != std::string::npos) &&
              (pos_colon > pos_space))
          {
            // there is a space before the colon: use the space as separator
            sKey = sValue.substr(0, pos_space);
            sTrimmedValue = sValue.substr(pos_space + 1);
            trim(sTrimmedValue);
          }
          else
          {
            // process map item
            sKey = sValue.substr(0, pos_colon);
            sTrimmedValue = sValue.substr(pos_colon + 1);
            trim(sTrimmedValue);
          }
        }
        else
        {
          sKey = sValue;
        }
        // attribute value pair
        if (!bParsingMedia)
        {
          // update session attribute
          sdp.addAttribute(sKey, sTrimmedValue);
        }
        else
        {
          // update media description level attribute
          sdp.getLastAddedMediaDescription().addAttribute(sKey, sTrimmedValue);
        }
      }
      else if (sName == "b")
      {
        // check for AS
        size_t pos = sValue.find("AS:");
        if (pos != std::string::npos)
        {
          sdp.getLastAddedMediaDescription().setSessionBandwidth(convert<uint32_t>(sValue.substr(pos + 3), 0));
        }
      }
    }
  }

  sdp.process();

  if (validate(sdp))
  {
    if (bVerbose)
      printSessionDescriptionInfo(sdp);

    return boost::optional<SessionDescription>(sdp);
  }
  else
  {
    LOG(WARNING) << "SDP validation failed";
    return boost::optional<SessionDescription>();
  }
}

void SdpParser::printSessionDescriptionInfo(const SessionDescription &sdp)
{
  std::ostringstream ostr;
  ostr << "Session attributes:\n";
  // session attributes
  // iterate over media level attributes
  const std::multimap<std::string, std::string>& attributes = sdp.getAttributes();
  for (auto it = attributes.begin(); it != attributes.end(); ++it)
  {
    ostr << "Name:" << it->first  << " Value: " << it->second << "\n";
  }
  for (size_t i = 0; i < sdp.getMediaDescriptionCount(); ++i)
  {
    const MediaDescription& rMedia = sdp.getMediaDescription(i);
    ostr << "Media: " << rMedia << "\n";
    ostr << "Media Map: \n";
    // iterate over media level attributes
    const std::multimap<std::string, std::string>& attributeMap = rMedia.getAttributes();
    for (auto it = attributeMap.begin(); it != attributeMap.end(); ++it)
    {
      ostr << "Name:" << it->first  << " Value: " << it->second << "\n";
    }
  }
  LOG(INFO) << ostr.str();
}

} // rfc4566
} // rtp_plus_plus
