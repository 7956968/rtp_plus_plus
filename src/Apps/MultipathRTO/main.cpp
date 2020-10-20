#include "MultipathRTOPch.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/MakeService.h>
#include <rtp++/media/MediaGeneratorService.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/StreamMediaSource.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/VirtualMediaSessionFactory.h>
#include <rtp++/rto/RtoEstimationSession.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/application/Application.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;

namespace po = boost::program_options;

/// @def DEBUG_DISABLE_SOURCE Useful to disable async errors
// #define DEBUG_DISABLE_SOURCE

/**
 * Requirements: this program will simulate the transmission of RTP over multiple paths.
 * Various scheduling algorithms will be configurable via command line parameters.
 * Video will be input from stdin (this will enable piping from ffmpeg for example) and
 * from file (raw H.264 file)
 * We need to decide whether this program will run in 'real-time' or as fast as possible.
 * The reception times (simulated one way delay) will be based upon input data from traces
 * or distributions.

 Input:
- H.264 video frames from stdin 
- H.264 video frames from file
- One way delay distribution traces
- One way delay distributions (use stdlib)

Scheduling:
- Round-robin
- Round-robin with I-frames over one path
- Random scheduling
- Bitrate scheduling

RTO:
- Simple multipath RTO algorithm

Retransmission:

Measurement requirements:
- Measure early losses
- Redundant retransmissions

*/

void stopIoService(const boost::system::error_code& ec, boost::asio::io_service& io_service)
{
  if (!ec)
  {
#ifdef _WIN32
    io_service.stop();
#else
    // io_service.stop();
    // raise(SIGINT);
    kill(getpid(), SIGINT);
#endif
  }
}

void startAudioSession(boost::shared_ptr<SimpleMediaSession> pRtpSession,
                       uint32_t uiAudioRateMs, const std::string& sAudioPayload,
                       uint8_t uiAudioPayloadChar, const std::string& sAudioPayloadSize,
                       uint32_t uiDurationSeconds, uint32_t uiTotalAudioSamples)
{
  // calculate audio parameters
  // payload
  media::PayloadType eType = media::parsePayload(sAudioPayload);
  LOG(INFO) << "Audio source: " << uiAudioRateMs << "ms Payload: " << sAudioPayload << " Sizes: " << sAudioPayloadSize << " Char Filler: " << uiAudioPayloadChar;
  // start media simulation service in new thread
  MediaGeneratorServicePtr pAudioService = makeService<media::MediaGenerator>(uiAudioRateMs * 1000, eType, sAudioPayloadSize, uiAudioPayloadChar, uiTotalAudioSamples, 0);
  pAudioService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleAudioSample, _1, pRtpSession));
#ifndef DEBUG_DISABLE_SOURCE
  pAudioService->start();
#endif
  // asio event loop
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);
  boost::asio::deadline_timer timer(io_service);
  if (uiDurationSeconds != 0)
  {
    timer.expires_from_now(boost::posix_time::seconds(uiDurationSeconds));
    timer.async_wait(boost::bind(&stopIoService, _1, boost::ref(io_service)));
  }
  startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)), boost::bind(&boost::asio::io_service::stop, boost::ref(io_service)));

  pAudioService->stop();
}

void startVideoSession(boost::shared_ptr<SimpleMediaSession> pRtpSession,
                       uint32_t uiFps, const std::string& sVideoPayload,
                       uint8_t uiVideoPayloadChar, const std::string& sVideoPayloadSize,
                       uint32_t uiDurationSeconds, uint32_t uiTotalVideoSamples)
{
  // calculate video parameters
  // framerate
  uint32_t uiIntervalMicrosecs = static_cast<uint32_t>(1000000.0/uiFps + 0.5);
  // payload
  media::PayloadType eType = media::parsePayload(sVideoPayload);
  LOG(INFO) << "Video source: " << uiIntervalMicrosecs/1000.0 << "ms Payload: " << sVideoPayload << " Sizes: " << sVideoPayloadSize << " Char Filler: " << uiVideoPayloadChar;

  // start media simulation service in new thread
  MediaGeneratorServicePtr pVideoService = makeService<media::MediaGenerator>(uiIntervalMicrosecs, eType, sVideoPayloadSize, uiVideoPayloadChar, uiTotalVideoSamples, 0);
  pVideoService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleVideoSample, _1, pRtpSession));
#ifndef DEBUG_DISABLE_SOURCE
  pVideoService->start();
#endif

  // asio event loop
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);
  boost::asio::deadline_timer timer(io_service);
  if (uiDurationSeconds != 0)
  {
    timer.expires_from_now(boost::posix_time::seconds(uiDurationSeconds));
    timer.async_wait(boost::bind(&stopIoService, _1, boost::ref(io_service)));
  }
  startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)), boost::bind(&boost::asio::io_service::stop, boost::ref(io_service)));
  pVideoService->stop();
}

void startStdInVideoSession(boost::shared_ptr<SimpleMediaSession> pMediaSession,
                            std::istream& in1, bool bTryLoop, uint32_t uiFps, bool bLimitRate,
                            uint32_t uiDurationSeconds)
{
  // start media simulation service in new thread
  StreamMediaSourceServicePtr pStdInVideoService = makeService<media::StreamMediaSource>(boost::ref(in1), rfc6184::H264, 10000, bTryLoop, 0, uiFps, bLimitRate, media::StreamMediaSource::AccessUnit);
  pStdInVideoService->get()->setReceiveAccessUnitCB(boost::bind(&ApplicationUtil::handleVideoAccessUnit, _1, pMediaSession));
#ifndef DEBUG_DISABLE_SOURCE
  pStdInVideoService->start();
#endif
  // asio event loop
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);
  boost::asio::deadline_timer timer(io_service);
  if (uiDurationSeconds != 0)
  {
    timer.expires_from_now(boost::posix_time::seconds(uiDurationSeconds));
    timer.async_wait(boost::bind(&stopIoService, _1, boost::ref(io_service)));
  }
  startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)), boost::bind(&boost::asio::io_service::stop, boost::ref(io_service)));

  pStdInVideoService->stop();
}

// In this code we assume that the local and remote SDPs match!
int main(int argc, char** argv)
{
  google::InitGoogleLogging(argv[0]);
  boost::filesystem::path app = boost::filesystem::system_complete(argv[0]);

  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::SESSION);
    ac.addOptions(ApplicationConfiguration::SENDER);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);
    ac.addOptions(ApplicationConfiguration::MULTIPATH);
    ac.addOptions(ApplicationConfiguration::SIMULATION);

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
    boost::optional<InterfaceDescriptions_t> remoteConf = ApplicationUtil::readNetworkConfigurationFile(ac.Session.NetworkConf.at(0), false);
    if (!remoteConf) { return -1; }

    boost::optional<rfc4566::SessionDescription> sdp = ApplicationUtil::readSdp(ac.Session.sessionDescriptions.at(0));
    if (!sdp) { return -1; }

    // make sure channel configuration file is valid
    std::string sChannelConfigFile(ac.Simulation.ChannelConfigFile);
    if (sChannelConfigFile.empty() || !FileUtil::fileExists(sChannelConfigFile)) 
    { 
      LOG(ERROR) << "Invalid channel configuration file: " << sChannelConfigFile;
      return -1; 
    }

    // Only a single H.264 stream can be read from stdin
    bool bGen = false, bStdin = false;
    std::ifstream in1;
    if (!ApplicationUtil::checkAndInitialiseSource(ac.Video.VideoDevice, bGen, bStdin, in1))
    {
      return -1;
    }

    // LIMITATION: we assume that the SDP contains exactly one media line!
    if (sdp->getMediaDescriptionCount() != 1)
    {
      LOG(ERROR) << "Only one RTP session is currently supported";
      return -1;
    }

    rto::RtoEstimationSession session(*sdp, *remoteConf, sChannelConfigFile, applicationParameters);
    session.start();

    rfc4566::MediaDescription media = sdp->getMediaDescription(0);

    if (media.getMediaType() == rfc4566::AUDIO)
    {
      startAudioSession(session.getSenderRtpSession(), ac.Audio.RateMs, ac.Audio.PayloadType, ac.Audio.PayloadChar, ac.Audio.PayloadSizeString, ac.Session.Duration, ac.Audio.TotalSamples);
    }
    else if (media.getMediaType() == rfc4566::VIDEO)
    {
      if (bStdin)
      {
        startStdInVideoSession(session.getSenderRtpSession(), std::cin, false, ac.Video.Fps, ac.Video.LimitRate, ac.Session.Duration);
      }
      else if (bGen)
      {
        startVideoSession(session.getSenderRtpSession(), ac.Video.Fps, ac.Video.PayloadType, ac.Video.PayloadChar, ac.Video.PayloadSizeString, ac.Session.Duration, ac.Video.TotalSamples);
      }
      else
      {
        startStdInVideoSession(session.getSenderRtpSession(), in1, ac.Video.Loop, ac.Video.Fps, ac.Video.LimitRate, ac.Session.Duration);
      }
    }

    session.stop();
    LOG(INFO) << "Done";

    if (in1.is_open())
      in1.close();

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
}

