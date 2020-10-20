#include "RtpReceiverPch.h"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/application/Application.h>
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc3264/OfferAnswerModel.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4588/Rfc4588.h>
#include <rtp++/RtpUtil.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;

namespace po = boost::program_options;

/**
 * @brief Handler to start multimedia services once media session has been started
 */
void onStart(media::SimpleMultimediaServiceV2& multimediaService)
{
  VLOG(2) << "Media session started: starting multimedia services";
  multimediaService.startServices();
}

int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  // read local SDP
  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::SESSION);
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);
    ac.addOptions(ApplicationConfiguration::RTP_RTCP);
    ac.addOptions(ApplicationConfiguration::NETWORK);

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
    ServiceManager serviceManager;

    boost::optional<InterfaceDescriptions_t> localConf;
    if (!ac.Session.NetworkConf.empty())
    {
      localConf = ApplicationUtil::readNetworkConfigurationFile(ac.Session.NetworkConf.at(0), ac.RtpRtcp.RtcpMux);
      if (!localConf) { return -1; }
    }
    else
    {
      localConf = ApplicationUtil::parseNetworkConfiguration(ac.Network.LocalNetworkConf, ac.RtpRtcp.RtcpMux);
      if (!localConf) { return -1; }
    }

    MediaSessionNetworkManager mediaSessionNetworkManager(serviceManager.getIoService(), ac.Network.LocalInterfaces);
    boost::optional<rfc4566::SessionDescription> sdp;
    if (!ac.Session.sessionDescriptions.empty())
    {
      // use SDP from file
      sdp = ApplicationUtil::readSdp(ac.Session.sessionDescriptions.at(0));
      if (!sdp) { return -1; }
    }
    else
    {
      if (ac.Network.RemoteHost == "")
      {
        LOG(WARNING) << "No remote host specified";
        return -1;
      }

      boost::optional<InterfaceDescriptions_t> remoteConf;
      remoteConf = ApplicationUtil::parseNetworkConfiguration(ac.Network.RemoteNetworkConf, ac.RtpRtcp.RtcpMux);
      if (!remoteConf)
      {
        LOG(WARNING) << "No remote network configuration specified";
        return -1;
      }

      // generate SDP: this method uses the MediaSessionNetworkManager to reserve ports
      uint32_t uiMaxBandwidthKbps = 0;
      std::vector<uint32_t> vBitrates = StringTokenizer::tokenizeV2<uint32_t>(ac.Video.VideoKbps, "|", true);
      assert(!vBitrates.empty());
      uiMaxBandwidthKbps = *std::max_element(vBitrates.begin(), vBitrates.end());

      bool bRapidSync = (ac.RtpRtcp.RapidSyncMode == 0)? false : true;
      rfc3261::SipContext context;
      rfc3264::OfferAnswerModel offerAnswer(context, mediaSessionNetworkManager, ac.RtpRtcp.EnableMpRtp, ac.RtpRtcp.RtcpMux, bRapidSync, ac.RtpRtcp.RtcpRs, ac.RtpRtcp.RtcpRtpHeaderExt);

      std::string sProfile = ac.RtpRtcp.RtpProfile;
      upgradeProfileIfNecessary(sProfile, ac.RtpRtcp.RtcpFb);
      offerAnswer.setPreferredProfile(sProfile);
      offerAnswer.setPreferredProtocol(ac.Network.Protocol);
      if (ac.Network.RtpPort != 0)
        offerAnswer.setPreferredStartingPort(ac.Network.RtpPort);
      offerAnswer.setBindPorts(false);
      std::string sDynamicFormat;
      offerAnswer.addFormat(sDynamicFormat, rfc4566::VIDEO, rfc4566::RtpMapping(ac.Video.VideoMediaType, "90000"), rfc4566::FormatDescription("packetization-mode=1"),
                            uiMaxBandwidthKbps, ac.RtpRtcp.RtcpFb, ac.RtpRtcp.RtcpXr);

      // RTX parameters
      if (ac.RtpRtcp.RtxTime > 0)
      {
        std::ostringstream ostr;
        ostr << "apt=" << sDynamicFormat << ";rtx-time=" << ac.RtpRtcp.RtxTime;
        std::string sRtxPayloadFormat;
        offerAnswer.addFormat(sRtxPayloadFormat, rfc4566::VIDEO, rfc4566::RtpMapping(rfc4588::RTX, "90000"), rfc4566::FormatDescription(ostr.str()), 0);
      }

      sdp = offerAnswer.generateOffer(ac.Network.RemoteHost, *remoteConf);

      if (!sdp) { return -1; }
#if 0
      // update host address
      sdp->updateAllConnectionAddresses(ac.Network.RemoteHost);
#endif
      VLOG(5) << "Generated sender SDP: " << sdp->toString();
    }


    media::SimpleMultimediaServiceV2 multimediaService;

    // perform SDP to RtpSessionParameter translation
    MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), *sdp, *localConf, false);
    SimpleMediaSessionFactory factory;
    boost::shared_ptr<SimpleMediaSessionV2> pMediaSession = factory.create(serviceManager.getIoService(), mediaSessionDescription, applicationParameters);
    if (pMediaSession->hasAudio())
    {
      boost::system::error_code ec = multimediaService.createAudioConsumer(pMediaSession->getAudioSource(), ac.Receiver.AudioFilename,
                                                                           pMediaSession->getAudioSessionManager()->getPrimaryRtpSessionParameters());
      if (ec)
      {
        LOG(WARNING) << "Error creating audio consumer: " << ec.message();
        return -1;
      }
    }

    if (pMediaSession->hasVideo())
    {
      boost::system::error_code ec = multimediaService.createVideoConsumer(pMediaSession->getVideoSource(), ac.Receiver.VideoFilename,
                                                                           pMediaSession->getVideoSessionManager()->getPrimaryRtpSessionParameters());
      if (ec)
      {
        LOG(WARNING) << "Error creating video consumer: " << ec.message();
        return -1;
      }
    }

    // set start handler for media session
    serviceManager.setOnStartHandler(boost::bind(&onStart, boost::ref(multimediaService)));

    // register media session with manager
    uint32_t uiServiceId;
    bool bSuccess = serviceManager.registerService(boost::bind(&SimpleMediaSessionV2::start, pMediaSession),
                                                   boost::bind(&SimpleMediaSessionV2::stop, pMediaSession),
                                                   uiServiceId);

    if (bSuccess)
    {
      // we will start it manually, but we do need to stop it automatically to end the asio event loop
      bSuccess = serviceManager.registerService(boost::bind(&media::SimpleMultimediaServiceV2::startServices, boost::ref(multimediaService) ),
                                                boost::bind(&media::SimpleMultimediaServiceV2::stopServices, boost::ref(multimediaService) ),
                                                uiServiceId,
                                                false); // NB: don't auto-start this component

      if (bSuccess)
      {
        // the user can now specify the handlers for incoming samples.
        startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
                       boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));
      }
      else
      {
        LOG(ERROR) << "Failed to register multimedia service";
      }
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
