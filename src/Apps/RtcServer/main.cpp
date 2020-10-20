#include "RtcServerPch.h"
#include <sstream>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <rtp++/application/Application.h>
#include <rtp++/application/ApplicationConfiguration.h>
#include <rtp++/experimental/Nada.h>
#include <rtp++/experimental/Scream.h>
#include <rtp++/network/NetworkInterfaceUtil.h>

#include "RtcConfig.h"
#include "RtcServiceThread.h"

using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
namespace po = boost::program_options;

bool validateConfiguration(const RtcConfig& rtcConfig)
{
  RtcConfig& config = const_cast<RtcConfig&>(rtcConfig);

  // network interfaces
  std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
  if (!rtcConfig.LocalInterfaces.empty())
  {
    // check that the interface is valid
    for (const std::string& sInterface : rtcConfig.LocalInterfaces)
    {
      bool bFound = false;
      for (const NetworkInterface& if1 : networkInterfaces)
      {
        if (sInterface == if1.Address)
        {
          bFound = true;
          break;
        }
      }
      if (!bFound)
      {
        LOG(WARNING) << "Invalid configuration: couldn't find network interface: " << sInterface;
        return false;
      }
    }
  }
  else
  {
    // else manually load local interfaces
    for (const NetworkInterface& if1 : networkInterfaces)
    {
      config.LocalInterfaces.push_back(if1.Address);
    }
  }

  // check FQDN
  if (rtcConfig.FQDN == "")
  {
    config.FQDN = config.LocalInterfaces.at(0);
  }

  // protocol
  if (config.Protocol == "udp" || config.Protocol == "UDP")
  {
    config.Protocol = "UDP";
  }
  else if (config.Protocol == "tcp" || config.Protocol == "TCP")
  {
    config.Protocol = "TCP";
  }
  // DCCP unsupported for now?
#if 1
  else if (config.Protocol == "dccp" || config.Protocol == "DCCP")
  {
    config.Protocol = "DCCP";
  }
#endif
  else if (config.Protocol == "sctp" || config.Protocol == "SCTP")
  {
    config.Protocol = "SCTP";
  }
  else
  {
    LOG(WARNING) << "Unsupported transport protocol: " << config.Protocol;
    return false;
  }

  // profile
  if (config.RtpProfile == "avp" || config.RtpProfile == "AVP")
  {
    config.RtpProfile = "AVP";
  }
  else if (config.RtpProfile == "avpf" || config.RtpProfile == "AVPF")
  {
    config.RtpProfile = "AVPF";
  }
  else
  {
    LOG(WARNING) << "Unsupported RTP profile: " << config.RtpProfile;
    return false;
  }

  if (rtcConfig.NoAudio && rtcConfig.NoVideo)
  {
    LOG(WARNING) << "Invalid configuration: no audio and no video";
    return false;
  }

  // validate rtcp fb and xr options
  for (const std::string& sFb : rtcConfig.RtcpFb)
  {
    if (sFb == rfc4585::ACK)
    {
      // ACK
      if (config.RtpProfile == "AVP")
      {
        LOG(INFO) << "Upgrading RTP profile to AVPF to support ACK";
        config.RtpProfile = "AVPF";
      }
    }
    else if (sFb == rfc4585::NACK)
    {
      // NACK
      if (config.RtpProfile == "AVP")
      {
        LOG(INFO) << "Upgrading RTP profile to AVPF to support NACK";
        config.RtpProfile = "AVPF";
      }
    }
    else if (sFb == experimental::SCREAM)
    {
      // SCREAM
      if (config.RtpProfile == "AVP")
      {
        LOG(INFO) << "Upgrading RTP profile to AVPF to support SCREAM";
        config.RtpProfile = "AVPF";
      }
    }
    else if (sFb == experimental::NADA)
    {
      // NADA
      if (config.RtpProfile == "AVP")
      {
        LOG(INFO) << "Upgrading RTP profile to AVPF to support NADA";
        config.RtpProfile = "AVPF";
      }
    }
    else
    {
      LOG(WARNING) << "Unsupported RTCP Feedback message: " << sFb;
      return false;
    }
  }

  for (const std::string& sXr : rtcConfig.RtcpXr)
  {
    if (sXr == rfc3611::RCVR_RTT)
    {
      // rcvr-rtt
    }
    else
    {
      LOG(WARNING) << "Unsupported RTCP XR message: " << sXr;
      return false;
    }
  }

  // TODO: if not no-video: check if video parameters are valid
  // video media type must be set
  // we only accept video device source models for now

  // bandwidth must be set

  // TODO: if not no-audio: check if audio parameters are valid

  return true;
}

int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  try
  {
    RtcConfigOptions config;
    po::options_description cmdline_options = config.generateOptions();

    LOG(INFO) << "Public IP: " << config.RtcParameters.PublicIp;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      std::ostringstream ostr;
      ostr << cmdline_options;
      LOG(ERROR) << ostr.str();
      return 1;
    }

    if (config.RtcParameters.logfile != "")
    {
      // update the log file: we want to be able to parse this file
      google::SetLogDestination(google::GLOG_INFO, (config.RtcParameters.logfile + ".INFO").c_str());
      google::SetLogDestination(google::GLOG_WARNING, (config.RtcParameters.logfile + ".WARNING").c_str());
      google::SetLogDestination(google::GLOG_ERROR, (config.RtcParameters.logfile + ".ERROR").c_str());
    }

    ApplicationUtil::printApplicationInfo(argc, argv);

    LOG(INFO) << "main";
    if (!validateConfiguration(config.RtcParameters))
    {
      LOG(WARNING) << "Invalid Configuration";
      return -1;
    }


    LOG(INFO) << "Public IP: " << config.RtcParameters.PublicIp;
    RtcServiceThread rtcService(config.RtcParameters);
    // the user can now specify the handlers for incoming samples.
    startEventLoop(boost::bind(&RtcServiceThread::run, boost::ref(rtcService)), boost::bind(&RtcServiceThread::shutdown, boost::ref(rtcService)));

    LOG(INFO) << "app complete";

    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }

  if (!ApplicationContext::cleanupApplication())
  {
    LOG(WARNING) << "Failed to cleanup application cleanly";
  }
  return 0;
}
