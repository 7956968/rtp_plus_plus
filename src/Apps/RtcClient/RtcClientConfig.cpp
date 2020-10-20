#include "CorePch.h"
#include "RtcClientConfig.h"
#include <rtp++/application/ApplicationParameters.h>

namespace po = boost::program_options;

namespace rtp_plus_plus {

using namespace app;

RtcClientConfigOptions::RtcClientConfigOptions()
  :m_options("Options")
{
  m_options.add_options()
      (ApplicationParameters::help.c_str(), "produce help message")
      (ApplicationParameters::logfile.c_str(), po::value<std::string>(&RtcParameters.logfile), "Logfile")
      (ApplicationParameters::rtc_offerer.c_str(), po::value<std::string>(&RtcParameters.RtcOfferer), "RTC Offerer")
      (ApplicationParameters::rtc_answerer.c_str(), po::value<std::string>(&RtcParameters.RtcAnswerer), "RTC Answerer")
      (ApplicationParameters::rtc_config.c_str(), po::value<std::string>(&RtcParameters.Config), "Config file for auto play")
      (ApplicationParameters::rtc_config_string.c_str(), po::value<std::string>(&RtcParameters.ConfigString), "Config string for auto play")
      (ApplicationParameters::rtc_kill_session.c_str(), po::bool_switch(&RtcParameters.KillSession)->default_value(false), "Kill existing sessions")
      ;
}

boost::program_options::options_description RtcClientConfigOptions::generateOptions()
{
  po::options_description cmdline_options;
  cmdline_options.add(m_options);
  return cmdline_options;
}

} // rtp_plus_plus
