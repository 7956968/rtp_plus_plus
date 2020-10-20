#pragma once
#include <fstream>
#include <string>
#include <cpputil/FileUtil.h>
#include <cpputil/StringTokenizer.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * @brief The MediaConfiguration struct is used to load media configurations from file
 * for the purpose of offer/answer negotiations.
 * The file has the format of
 * media type | static payload type (leave empty for dynamic payloads |  media subtype | clock rate | Format specific (number of channels for audio, unused for video) | fmtp parameters \n
 */
struct MediaConfiguration
{
  /// media type e.g. audio or video
  std::string MediaType;
  /// static or dynamic payload type
  std::string StaticPayload;
  /// media subtype
  std::string MediaSubType;
  std::string ClockRate;
  std::string FormatSpecific;
  std::string Fmtp;
  std::string Bitrate;
};

class MediaConfigurationParser
{
public:

  static std::vector<MediaConfiguration> parseMediaConfiguration(const std::string& sMediaConfiguration)
  {
    std::vector<MediaConfiguration> configurations;
    std::istringstream in(sMediaConfiguration);
    std::string sLine;
    while (!getline(in, sLine).eof())
    {
      MediaConfiguration cfg;
      std::vector<std::string> values = StringTokenizer::tokenize(sLine, "|", true, false);
      if (values.size() >= 3)
      {
        cfg.MediaType = values[0];
        cfg.StaticPayload = values[1];
        if (cfg.StaticPayload.empty())
        {
          if (values.size() < 7)
          {
            LOG(WARNING) << "Too few parameters for dynamic payload type";
            continue;
          }
          // dynamic payload type
          cfg.MediaSubType = values[2];
          cfg.ClockRate = values[3];
          cfg.FormatSpecific = values[4];
          cfg.Fmtp = values[5];
          cfg.Bitrate = values[6];
        }
        else
        {
          // static payload type
          cfg.Bitrate = values[2];
        }
        configurations.push_back(cfg);
      }
      else
      {
        VLOG(6) << "Too little tokens for configuration";
        continue;
      }
    }
    return configurations;
  }

  static std::vector<MediaConfiguration> parseMediaConfigurationFile(const std::string& sMediaConfigurationFile)
  {
    std::string sContent = FileUtil::readFile(sMediaConfigurationFile, false);
    return parseMediaConfiguration((sContent));
  }
};

} // media
} // rtp_plus_plus
