#include "CldRtspServerPch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/application/Application.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include "CldRtspServer.h"

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
namespace po = boost::program_options;

// In this code we assume that the local and remote SDPs match!
int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::SESSION);
    ac.addOptions(ApplicationConfiguration::SENDER);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
    ac.addOptions(ApplicationConfiguration::MULTIPATH);
    ac.addOptions(ApplicationConfiguration::RTSP_SERVER);

    po::options_description cmdline_options = ac.generateOptions();

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      ostringstream ostr;
      ostr << cmdline_options;
      LOG(ERROR) << ostr.str();
      return 1;
    }

    if (ac.Application.logfile != "")
    {
      // update the log file: we want to be able to parse this file
      google::SetLogDestination(google::GLOG_INFO, (ac.Application.logfile + ".INFO").c_str());
      google::SetLogDestination(google::GLOG_WARNING, (ac.Application.logfile + ".WARNING").c_str());
      google::SetLogDestination(google::GLOG_ERROR, (ac.Application.logfile + ".ERROR").c_str());
    }

    ApplicationUtil::printApplicationInfo(argc, argv);
    // generic parameters
    GenericParameters applicationParameters = ac.getGenericParameters();

    bool bError = false;

    std::vector<InterfaceDescriptions_t> localConfigurations = ApplicationUtil::readNetworkConfigurationFiles(ac.Session.NetworkConf, bError, false);
    if (bError)
    {
      LOG(ERROR) << "Error parsing network configurations";
      return -1;
    }

    std::vector<rfc4566::SessionDescription> sessionDescriptions = ApplicationUtil::readSdps(ac.Session.sessionDescriptions, false, bError);
    if (bError)
    {
      LOG(ERROR) << "Error parsing session descriptions";
      return -1;
    }

    if (localConfigurations.size() != sessionDescriptions.size() || localConfigurations.empty())
    {
      LOG(ERROR) << "No sessions configured";
      return -1;
    }

    ServiceManager serviceManager;
    boost::system::error_code ec;
    CldRtspServer rtspServer(serviceManager.getIoService(), applicationParameters, ec, ac.Rtsp.RtspPort);

    if (ec)
    {
      LOG(ERROR) << "Error creating RTSP server: " << ec.message();
      return -1;
    }

    LOG(INFO) << "Starting RTSP server with sessions: " << sessionDescriptions.size();
    for (size_t i = 0; i < localConfigurations.size(); ++i)
    {
      LOG(INFO) << "Adding session for " << sessionDescriptions[i].getSessionName();
      rtspServer.addLiveSource(sessionDescriptions[i], localConfigurations[i]);
    }

    // register media session with manager
    uint32_t uiServiceId;
    bool bSuccess = serviceManager.registerService(boost::bind(&CldRtspServer::start, boost::ref(rtspServer)),
                                                   boost::bind(&CldRtspServer::shutdown, boost::ref(rtspServer)),
                                                   uiServiceId);

    if (bSuccess)
    {
      // the user can now specify the handlers for incoming samples.
      startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                     boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));

    }
    else
    {
      LOG(WARNING) << "Failed to register service";
    }

    if (!ApplicationContext::cleanupApplication())
    {
      LOG(WARNING) << "Failed to cleanup application cleanly";
    }
    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: " << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: " << e.what();
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

