#pragma once
#include <cstdint>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/rfc4566/Rfc4566.h>

namespace rtp_plus_plus
{
namespace rfc5285
{

class Extmap
{
public:
  static boost::optional<Extmap> parseFromSdpAttribute(const std::string& sAttribute)
  {
    std::vector<std::string> tokens = StringTokenizer::tokenize(sAttribute);
    if (tokens.size() < 2) return boost::optional<Extmap>();

    uint32_t uiId = 0;
    rfc4566::Direction eDirection = rfc4566::DIR_UNSPECIFIED;
    std::string sExtensionName;
    std::string sExtensionAttributes;

    bool bSuccess = false;
    size_t pos = tokens[0].find("/");
    if (pos != std::string::npos)
    {
      uiId = convert<uint32_t>(tokens[0].substr(0, pos), bSuccess);
      if (!bSuccess) return boost::optional<Extmap>();
      eDirection = rfc4566::getDirectionFromString(tokens[0].substr(pos + 1));
      if (eDirection == rfc4566::DIR_UNKNOWN) return boost::optional<Extmap>();
    }
    else
    {
      uiId = convert<uint32_t>(tokens[0], bSuccess);
      if (!bSuccess) return boost::optional<Extmap>();
    }

    sExtensionName = tokens[1];

    for (size_t i = 2; i < tokens.size(); ++i)
      sExtensionAttributes = sExtensionAttributes + tokens[i];
    return boost::optional<Extmap>(Extmap(uiId, eDirection, sExtensionName, sExtensionAttributes));
  }

  Extmap()
    :m_uiId(0),
      m_eDirection(rfc4566::DIR_UNSPECIFIED)
  {

  }

  Extmap(uint32_t uiId, rfc4566::Direction eDirection, const std::string& sExtensionName, const std::string& sExtensionAttributes)
    :m_uiId(uiId),
      m_eDirection(eDirection),
      m_sExtensionName(sExtensionName),
      m_sExtensionAttributes(sExtensionAttributes)
  {

  }

  uint32_t getId() const { return m_uiId; }
  rfc4566::Direction getDirection() const { return m_eDirection; }
  std::string getExtensionName() const { return m_sExtensionName; }
  std::string getExtensionAttributes() const { return m_sExtensionAttributes; }

  void setId(const uint32_t val) { m_uiId = val; }
  void setDirection(const rfc4566::Direction val) { m_eDirection = val; }
  void setExtensionName(const std::string& val) { m_sExtensionName = val; }
  void setExtensionAttributes(const std::string& val) { m_sExtensionAttributes = val; }


private:

  uint32_t m_uiId;
  rfc4566::Direction m_eDirection;
  std::string m_sExtensionName;
  std::string m_sExtensionAttributes;
};

} // rfc5285
} // rtp_plus_plus
