#include "SvcRtpSenderPch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <rtp++/application/Application.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/SimpleMultimediaService.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/SvcMediaSessionFactory.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4566/SessionDescription.h>

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

  ApplicationUtil::printApplicationInfo(argc, argv);

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

    // generic parameters
    GenericParameters applicationParameters = ac.getGenericParameters();

    boost::optional<InterfaceDescriptions_t> remoteConf = ApplicationUtil::readNetworkConfigurationFile(ac.Session.NetworkConf.at(0), false);
    if (!remoteConf) { return -1; }

    boost::optional<rfc4566::SessionDescription> localSdp = ApplicationUtil::readSdp(ac.Session.sessionDescriptions.at(0));
    if (!localSdp) { return -1; }

    // TOOD: add some kind of check to make sure that we are dealing with an SVC session?

    // perform SDP to RtpSessionParameter translation
    MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), *localSdp, *remoteConf, true);
    SvcMediaSessionFactory factory;
    boost::shared_ptr<SimpleMediaSession> pMediaSession = factory.create(mediaSessionDescription, applicationParameters);
    assert (pMediaSession->hasVideo());

    media::SimpleMultimediaService multimediaService;
    boost::system::error_code ec = multimediaService.createVideoProducer(pMediaSession, ac.Video.VideoDevice,
                                                                         ac.Video.Fps, ac.Video.LimitRate,
                                                                         ac.Video.PayloadType, ac.Video.PayloadSizeString,
                                                                         ac.Video.PayloadChar, ac.Video.TotalSamples,
                                                                         ac.Video.Loop, ac.Video.LoopCount
                                                                         );
    if (!ec)
    {
      multimediaService.startServices();

      // the user can now specify the handlers for incoming samples.
      startEventLoop(boost::bind(&SimpleMediaSession::start, pMediaSession), boost::bind(&SimpleMediaSession::stop, pMediaSession));

      multimediaService.stopServices();

      if (!ApplicationContext::cleanupApplication())
      {
        LOG(WARNING) << "Failed to cleanup application cleanly";
      }

      return 0;
    }
    else
    {
      LOG(WARNING) << "Error creating video producer: " << ec.message();
      return -1;
    }
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
