#include "RtspClientPch.h"
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
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/rfc2326/RtspUri.h>
#include <rtp++/rfc2326/RtspClientSession.h>

#ifndef WIN32
#include <signal.h>
#else
#include <Windows.h>
#endif

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
using namespace rtp_plus_plus::rfc2326;
namespace po = boost::program_options;

void startApplicationHandler(const boost::system::error_code& ec, rfc2326::RtspClientSession& rtspClientSession, media::SimpleMultimediaServiceV2& multimediaService)
{
  if (ec)
  {
    VLOG(2) << "Error starting call: " << ec.message();
  }
  else
  {
    boost::system::error_code ec;
    VLOG(15) << "Call accepted";
    
    boost::shared_ptr<SimpleMediaSessionV2> pMediaSession = rtspClientSession.getMediaSession();
    if (pMediaSession->hasAudio())
    {
      ec = multimediaService.createAudioConsumer(
        pMediaSession->getAudioSource(),
        "AUDIO_",
        pMediaSession->getAudioSessionManager()->getPrimaryRtpSessionParameters());
      assert(!ec);
    }

    if (pMediaSession->hasVideo())
    {
      ec = multimediaService.createVideoConsumer(
        pMediaSession->getVideoSource(),
        "VIDEO_",
        pMediaSession->getVideoSessionManager()->getPrimaryRtpSessionParameters());
      assert(!ec);
    }

    VLOG(6) << "TODO: When should this service be stopped";
    ec = multimediaService.startServices();
    assert(!ec);
  }
}

void endApplicationHandler()
{
  VLOG(15) << "Simulating Ctrl-C to end session";
#ifndef _WIN32
  kill(getpid(), SIGINT);
#else
  GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); // generate Control+C event
#endif
}

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
    ac.addOptions(ApplicationConfiguration::RTSP_CLIENT);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
    ac.addOptions(ApplicationConfiguration::NETWORK);
    ac.addOptions(ApplicationConfiguration::SESSION);

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
    boost::optional<std::string> streamName = applicationParameters.getStringParameter(ApplicationParameters::rtsp_uri);
    if (!streamName)
    {
      LOG(WARNING) << "No RTSP URI specified.";
      return -1;
    }
    else
    {
      ServiceManager serviceManager;

      RtspUri rtspUri(*streamName);
      LOG(WARNING) << "Opening RTSP URI: " << *streamName;

      // check if local interfaces have been specified on the command line
      boost::optional<std::vector<std::string> > localInterfaces = applicationParameters.getStringsParameter(ApplicationParameters::local_interfaces);
      assert(localInterfaces);

      /// Multimedia service for simulation of consumption of media
      media::SimpleMultimediaServiceV2 multimediaService;

      RtspClientSession rtspClientSession(serviceManager.getIoService(), applicationParameters, rtspUri, *localInterfaces);
      rtspClientSession.setOnSessionStartHandler(boost::bind(&startApplicationHandler, _1, boost::ref(rtspClientSession), boost::ref(multimediaService)));
      rtspClientSession.setOnSessionEndHandler(&endApplicationHandler);

      // register media session with manager
      uint32_t uiServiceId;
      bool bSuccess = serviceManager.registerService(boost::bind(&rfc2326::RtspClientSession::start, boost::ref(rtspClientSession)),
                                                     boost::bind(&rfc2326::RtspClientSession::shutdown, boost::ref(rtspClientSession)),
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

