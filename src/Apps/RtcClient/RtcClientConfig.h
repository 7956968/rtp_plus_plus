#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <cpputil/GenericParameters.h>

namespace rtp_plus_plus 
{

struct RtcClientConfig
{
  /// app: name of log file
  std::string logfile;
  /// RTC server offerer
  std::string RtcOfferer;
  /// RTC server answerer
  std::string RtcAnswerer;
  /// config file for auto-play
  std::string Config;
  /// config string for auto-play
  std::string ConfigString;
  /// kill flag for kill existing sessions
  bool KillSession;
};

/**
 * @brief The RtcConfig class stores common application parameters.
 * Note: parameter names map onto their text version by replacing underscores "_" with dashes "-"
 */
class RtcClientConfigOptions
{
public:

  RtcClientConfigOptions();
  boost::program_options::options_description generateOptions();

public:
  RtcClientConfig RtcParameters;

private:
  boost::program_options::options_description m_options;
};

} // rtp_plus_plus
