#include "RtspServerPch.h"
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
#include <rtp++/rfc2326/LiveRtspServer.h>
#include <rtp++/rfc6184/Rfc6184.h>

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
    ac.addOptions(ApplicationConfiguration::MULTIPATH);
    ac.addOptions(ApplicationConfiguration::RTSP_SERVER);
    ac.addOptions(ApplicationConfiguration::NETWORK);
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);

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
    boost::optional<std::string> streamName = applicationParameters.getStringParameter(ApplicationParameters::stream_name);
    std::string sStreamName = (streamName) ? *streamName : "liveStream";

    boost::optional<std::string> deviceName = applicationParameters.getStringParameter(ApplicationParameters::video_device);
    if (!deviceName)
    {
      LOG(WARNING) << "No video device specified.";
      return -1;
    }
    else
    {
      // fps has default value
      boost::optional<uint32_t> fps = applicationParameters.getUintParameter(ApplicationParameters::video_fps);

      ServiceManager serviceManager;

      std::shared_ptr<media::IVideoDevice> pVideoDevice = media::VirtualVideoDeviceV2::create(serviceManager.getIoService(), *deviceName, rfc6184::H264, *fps);
      if (pVideoDevice)
      {
        boost::system::error_code ec;
        // Note: RTSP server and video device MUST use same IO service so that we can run everything from
        // one thread
        rfc2326::LiveRtspServer rtspServer(sStreamName, pVideoDevice, serviceManager.getIoService(), applicationParameters, 
                                           ec, ac.Rtsp.RtspPort, ac.Rtsp.RtspRtpPort, ac.Rtsp.RtpSessionTimeout);

        if (ec)
        {
          LOG(WARNING) << "Failed to create RTSP server";
          return -1;
        }

        // register media session with manager
        uint32_t uiServiceId;
        bool bSuccess = serviceManager.registerService(boost::bind(&rfc2326::RtspServer::start, boost::ref(rtspServer)),
                                                       boost::bind(&rfc2326::RtspServer::shutdown, boost::ref(rtspServer)),
                                                       uiServiceId);

        if (bSuccess)
        {
          // the user can now specify the handlers for incoming samples.
          startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                         boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));

//        // the user can now specify the handlers for incoming samples.
//        startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)),
//                       boost::bind(&rfc2326::RtspServer::shutdown, boost::ref(rtspServer)));
        }
        else
        {
          LOG(WARNING) << "Failed to register service";
        }
      }
      else
      {
        LOG(WARNING) << "Invalid video device: " << *deviceName;
      }
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

