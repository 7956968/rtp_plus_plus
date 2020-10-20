#pragma once

#include <string>
#include <boost/algorithm/string.hpp>
#include <cpputil/Conversion.h>

#define H264_PACKETIZATION_MODE "packetization-mode"
#define H264_PROFILE_LEVEL_ID "profile-level-id"
#define H264_SPROP_PARAMETER_SETS "sprop-parameter-sets"

namespace rtp_plus_plus
{
namespace media
{
namespace h264
{

/**
  * Parser for H264 format description
  */
struct H264FormatDescription
{
  /**
    * Constructor for H264 format description with minimal error checking.
    * @param sFmtp The fmtp line from an SDP description
    */
  H264FormatDescription(const std::string& sFmtp)
    :m_bValid(false)
  {
    std::vector<std::string> vResults;
    // if the string contains the format type there is a space before the fmtp string
    boost::algorithm::split(vResults, sFmtp, boost::algorithm::is_any_of(" ;"));

    if (vResults.empty()) return;

    // check for payload type
    bool bSuccess = true;
    uint8_t uiPayloadType = convert<uint8_t>(vResults[0], bSuccess);
    if (bSuccess)
    {
      PayloadType = uiPayloadType;
    }

    for ( size_t i = 0; i < vResults.size(); ++i) 
    {
      std::vector<std::string> params;
      boost::algorithm::split(params, vResults[i], boost::algorithm::is_any_of("="));
      if (params[0] == H264_PACKETIZATION_MODE)
      {
        PacketizationMode = params[1];
      }
      else if (params[0] == H264_PROFILE_LEVEL_ID)
      {
        ProfileLevelId = params[1];
      }
      else if (params[0] == H264_SPROP_PARAMETER_SETS)
      {
        SpropParameterSets = params[1];
        boost::algorithm::split(params, SpropParameterSets, boost::algorithm::is_any_of(","));
        if (params.size() == 2)
        {
          Sps = params[0];
          Pps = params[1];
        }
      }
    }

    // check if we managed to find all fields
    if (
        !PacketizationMode.empty() &&
        !ProfileLevelId.empty() &&
        !SpropParameterSets.empty() &&
        !Sps.empty() &&
        !Pps.empty()
       )
    {
      m_bValid = true;
    }
  }

  uint8_t PayloadType;
  std::string PacketizationMode;
  std::string ProfileLevelId;
  std::string SpropParameterSets;
  std::string Sps;
  std::string Pps;

private:
  bool m_bValid;
};

}
}
}
