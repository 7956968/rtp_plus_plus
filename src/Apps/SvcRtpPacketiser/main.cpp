#include "SvcRtpPacketiserPch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/mediasession/SvcRtpPacketisationMediaSessionFactory.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/application/Application.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
namespace po = boost::program_options;

void onStart(media::SimpleMultimediaServiceV2& multimediaService)
{
  VLOG(2) << "Media session started: starting multimedia services";
  multimediaService.startServices();
}

/**
 * @brief main This application allows us to check the RTP packetisation process. Every incoming sample is logged and
 *
 * @param argc
 * @param argv
 * @return
 */
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
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);
    ac.addOptions(ApplicationConfiguration::MULTIPATH);

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
    // HACK to prevent blocking SCTP connections
    applicationParameters.setBoolParameter(ApplicationParameters::sctp_dont_connect, true);

    boost::optional<InterfaceDescriptions_t> remoteConf = ApplicationUtil::readNetworkConfigurationFile(ac.Session.NetworkConf.at(0), false);
    if (!remoteConf) { return -1; }

    boost::optional<rfc4566::SessionDescription> localSdp = ApplicationUtil::readSdp(ac.Session.sessionDescriptions.at(0));
    if (!localSdp) { return -1; }

    media::SimpleMultimediaServiceV2 multimediaService;

    // perform SDP to RtpSessionParameter translation
    MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
                                                                              *localSdp, *remoteConf, true);
    ServiceManager serviceManager;
    SvcRtpPacketisationMediaSessionFactory factory;
    boost::shared_ptr<SimpleMediaSessionV2> pMediaSession = factory.create(serviceManager.getIoService(), mediaSessionDescription, applicationParameters);

    if (pMediaSession->hasVideo())
    {
      boost::system::error_code ec = multimediaService.createVideoProducer(pMediaSession, ac.Video.VideoDevice,
                                                                           ac.Video.Fps, ac.Video.LimitRate,
                                                                           ac.Video.PayloadType, ac.Video.PayloadSizeString,
                                                                           ac.Video.PayloadChar, ac.Video.TotalSamples,
                                                                           ac.Video.Loop, ac.Video.LoopCount
                                                                           );
      if (ec)
      {
        LOG(WARNING) << "Error creating video producer: " << ec.message();
        return -1;
      }
    }

    // set start handler for media session
    serviceManager.setOnStartHandler(boost::bind(&onStart, boost::ref(multimediaService)));

    // register media session with manager
    uint32_t uiServiceId;
    bool bSuccess = serviceManager.registerService(boost::bind(&SimpleMediaSessionV2::start, pMediaSession),
                                                   boost::bind(&SimpleMediaSessionV2::stop, pMediaSession), uiServiceId);

    if (bSuccess)
    {
      // the user can now specify the handlers for incoming samples.
      startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                     boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));

      multimediaService.stopServices();
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

