#include "SipClientPch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/Conversion.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/application/Application.h>
#include <rtp++/media/MediaConfigurationParser.h>
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/network/NetworkInterfaceUtil.h>
#include <rtp++/rfc3261/SipAgent.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3264/OfferAnswerModel.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc4867/Rfc4867.h>
#include <rtp++/rfc6184/Rfc6184.h>

#ifndef WIN32
#include <signal.h>
#else
#include <Windows.h>
#endif

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
using namespace rtp_plus_plus::rfc3261;
namespace po = boost::program_options;

void startApplicationHandler(const boost::system::error_code& ec, rfc3261::SipAgent& sipAgent, media::SimpleMultimediaServiceV2& multimediaService)
{
  if (ec)
  {
    VLOG(2) << "Error starting call: " << ec.message();
  }
  else
  {
    boost::system::error_code ec;
    VLOG(15) << "Call accepted";    
    boost::shared_ptr<SimpleMediaSessionV2> pMediaSession = sipAgent.getMediaSession();
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

void endApplicationHandler(const boost::system::error_code& ec)
{
  VLOG(15) << "Simulating Ctrl-C to end session";

  if (ec)
  {
    VLOG(2) << "Ending session: " << ec.message();
  }
#ifndef _WIN32
  kill(getpid(), SIGINT);
#else
  GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); // generate Control+C event
#endif
}

void endCallHandler(const boost::system::error_code& ec, rfc3261::SipAgent& sipAgent, media::SimpleMultimediaServiceV2& multimediaService)
{
  VLOG(15) << "Simulating Ctrl-C to end session";

  boost::system::error_code error = multimediaService.stopServices();
  if (error)
  {
    VLOG(2) << "Error stopping multimedia services: " << error.message();
  }
  endApplicationHandler(boost::system::error_code());
}

#define RING_COUNT 1
void handleCall(const boost::system::error_code& ec, const uint32_t uiTxId, const SipMessage& invite, SipAgent& sipAgent, boost::asio::deadline_timer* pTimer)
{
  static int ring_count = 0;
  if (ring_count < RING_COUNT)
  {
    VLOG(2) << "Ringing";
    if (ring_count == 0)
      sipAgent.ringing(uiTxId);
    pTimer->expires_from_now(boost::posix_time::seconds(2));
    pTimer->async_wait(boost::bind(&handleCall, _1, uiTxId, invite, boost::ref(sipAgent), pTimer));
    ++ring_count;
  }
  else
  {
    delete pTimer;
#if 0
    VLOG(2) << "Rejecting call";
    sipAgent.reject(uiTxId);
#else
    VLOG(2) << "Accepting call";
    sipAgent.accept(uiTxId);
#endif
  }
}

void inviteHandler(const uint32_t uiTxId, const SipMessage& invite, SipAgent& sipAgent, boost::asio::io_service& ioService)
{
  std::string sName = invite.getFromDisplayName().empty() ? invite.getFromUri() : invite.getFromDisplayName();
  VLOG(2) << "Incoming call from " << sName;

  boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(ioService);
  pTimer->expires_from_now(boost::posix_time::seconds(2));
  pTimer->async_wait(boost::bind(&handleCall, _1, uiTxId, invite, boost::ref(sipAgent), pTimer));
}

void inviteResponseHandler(const boost::system::error_code& ec, uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (ec)
  {
    LOG(WARNING) << "Call failed: " << ec.message();
    endApplicationHandler(ec);
  }
  else
  {
    VLOG(2) << "Call succeeded. Do something in application layer.";
  }
}

void registerResponseHandler(const boost::system::error_code& ec, uint32_t uiTxId, const SipMessage& sipResponse)
{
  if (ec)
  {
    LOG(WARNING) << "REGISTER failed: " << ec.message();
//    endApplicationHandler(ec);
  }
  else
  {
    VLOG(2) << "REGISTER succeeded. Do something";
  }
}

void loadMediaConfigurationIntoOfferAnswerModel(std::vector<media::MediaConfiguration>& mediaConfig, std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswerModel)
{
  for (media::MediaConfiguration& media : mediaConfig)
  {
    bool bDummy;
    uint32_t uiBitrate = ::convert<uint32_t>(media.Bitrate, bDummy);
    assert(media.MediaType == rfc4566::AUDIO || media.MediaType == rfc4566::VIDEO);
    if (media.StaticPayload.empty())
    {
      boost::optional<rfc4566::RtpMapping> rtpMapping(rfc4566::RtpMapping(media.MediaSubType, media.ClockRate));
      boost::optional<rfc4566::FormatDescription> fmtp(rfc4566::FormatDescription(media.Fmtp));
      // dynamic payload
      std::string sDynamicPayloadType;
      // TODO: load from config?
      std::vector<std::string> vRtcpFb;
      std::vector<std::string> vRtcpXr;
      if (!pOfferAnswerModel->addFormat(sDynamicPayloadType, media.MediaType, rtpMapping, fmtp, uiBitrate, vRtcpFb, vRtcpXr))
      {
        LOG(WARNING) << "Failed to add format to offer/answer model";
      }
    }
    else
    {
      // static payload
      uint32_t uiPt = ::convert<uint32_t>(media.StaticPayload, bDummy);
      assert(uiPt < 96);
      std::vector<std::string> vRtcpFb;
      std::vector<std::string> vRtcpXr;
      if (!pOfferAnswerModel->addFormat(media.MediaType, media.StaticPayload, uiBitrate, vRtcpFb, vRtcpXr))
      {
        LOG(WARNING) << "Failed to add static format to offer/answer model";
      }
    }
  }

  // RG: leaving this out for now, not sure if linphone requires this...
#if 0
  std::string sProfileLevelId, sSPropParameterSets;
  ec = pVideoDevice->getParameter(rfc6184::PROFILE_LEVEL_ID, sProfileLevelId);
  if (ec)
  {
    LOG(WARNING) << "Failed to get video device profile level id";
    return -1;
  }

  ec = pVideoDevice->getParameter(rfc6184::SPROP_PARAMETER_SETS, sSPropParameterSets);
  if (ec)
  {
    LOG(WARNING) << "Failed to get video device parameter sets";
    return -1;
  }
  std::ostringstream ostr;
  ostr << rfc6184::PROFILE_LEVEL_ID << "=" << sProfileLevelId << ";";
  ostr << rfc6184::PACKETIZATION_MODE << "=1;";
#if 0
  // Adding this causes offer/answer to fail?
  ostr << rfc6184::SPROP_PARAMETER_SETS << "=" << sSPropParameterSets;
#endif
  fmtpH264->fmtp = ostr.str();
  pOfferAnswerModel->addFormat(rfc4566::VIDEO, rtpMapH264, fmtpH264, 500);
#endif
}

/**
 * @brief  Sample SIP client application
 * @author Ralf Globisch
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
    ac.addOptions(ApplicationConfiguration::SIP_CLIENT);
    ac.addOptions(ApplicationConfiguration::MULTIPATH);
    ac.addOptions(ApplicationConfiguration::SESSION);
    ac.addOptions(ApplicationConfiguration::SENDER);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
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

    boost::optional<std::string> sipUser = applicationParameters.getStringParameter(ApplicationParameters::sip_user);
    boost::optional<std::string> fqdn = applicationParameters.getStringParameter(ApplicationParameters::fqdn);
    boost::optional<std::string> domain = applicationParameters.getStringParameter(ApplicationParameters::domain);
    boost::optional<std::string> sipProxy = applicationParameters.getStringParameter(ApplicationParameters::sip_proxy);
    boost::optional<bool> useSipProxy = applicationParameters.getBoolParameter(ApplicationParameters::use_sip_proxy);
    boost::optional<bool> registerWithProxy = applicationParameters.getBoolParameter(ApplicationParameters::register_with_proxy);
    boost::optional<std::string> sipCallee = applicationParameters.getStringParameter(ApplicationParameters::sip_callee);
    boost::optional<std::string> videoDeviceName = applicationParameters.getStringParameter(ApplicationParameters::video_device);
    boost::optional<std::string> audioDeviceName = applicationParameters.getStringParameter(ApplicationParameters::audio_device);
    boost::optional<bool> enableMpRtp = applicationParameters.getBoolParameter(ApplicationParameters::enable_mprtp);
    boost::optional<bool> rtcpMux = applicationParameters.getBoolParameter(ApplicationParameters::rtcp_mux);
    boost::optional<uint32_t> rapidSync = applicationParameters.getUintParameter(ApplicationParameters::rapid_sync_mode);
    boost::optional<std::string> mediaConfigurationFile = applicationParameters.getStringParameter((ApplicationParameters::media_configuration_file));
    boost::optional<uint32_t> duration = applicationParameters.getUintParameter((ApplicationParameters::dur));
    boost::optional<bool> rtcpRs = applicationParameters.getBoolParameter(ApplicationParameters::rtcp_rs);

    std::string sFqdn;
    if (!fqdn || *fqdn == "")
    {
      std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
      sFqdn = networkInterfaces.at(0).Address;
    }
    else
    {
      sFqdn = *fqdn;
    }

    std::string sDomain = domain ? *domain : sFqdn;

    if (!videoDeviceName)
    {
      LOG(WARNING) << "No video device specified.";
      return -1;
    }

    if (!audioDeviceName)
    {
      LOG(WARNING) << "No audio device specified.";
      return -1;
    }

    VLOG(2) << "SIP User:" << *sipUser << "@[" << sFqdn << "]";
    if (sipProxy)
      VLOG(2) << "Proxy / REGISTER server : " << *sipProxy << " register with proxy: " << *registerWithProxy << " Route outgoing call via proxy: " << *useSipProxy;
    if (sipCallee)
      VLOG(2) << "Callee : " << *sipCallee;
    if (videoDeviceName)
      VLOG(2) << "Video Device : " << *videoDeviceName;
    if (audioDeviceName)
      VLOG(2) << "Audio Device : " << *audioDeviceName;

#if 0
    if (!sipProxy && !sipCallee)
    {
      LOG(WARNING) << "No SIP proxy or callee specified.";
      return -1;
    }
#endif

    std::vector<media::MediaConfiguration> mediaConfig;
    if (mediaConfigurationFile)
    {
      if (!boost::filesystem::exists(*mediaConfigurationFile))
      {
        LOG(WARNING) << "Specified media configuration file " << *mediaConfigurationFile  << " does not exist";
        return -1;
      }
      else
      {
        mediaConfig = media::MediaConfigurationParser::parseMediaConfigurationFile(*mediaConfigurationFile);
      }
    }
    else
    {
      // DEFAULT: not sure if lin phone requires profile level id
      std::string sDefaultConfiguration = "audio||AMR|8000|1|octet-align=1|13\n"\
          "audio||opus|48000|1|useinbandfec=1; usedtx=1|64\n"\
          "audio||speex|8000|1|vbr=on|64\n"\
          "audio|0|64\n"\
          "audio|8|64\n"\
          "video||H264|90000||packetization-mode=1;|500\n";
      mediaConfig = media::MediaConfigurationParser::parseMediaConfiguration(sDefaultConfiguration);
    }
    if (mediaConfig.empty())
    {
      LOG(WARNING) << "Invalid media configuration.";
      return -1;
    }

    // fps has default value
    boost::optional<uint32_t> fps = applicationParameters.getUintParameter(ApplicationParameters::video_fps);

    uint32_t uiFps = fps ? *fps : 10;
    ServiceManager serviceManager;
    std::shared_ptr<media::IVideoDevice> pVideoDevice;

    // NOTE: video device should actually only be initialised after offer/answer as we don't know what media type will be used
    // before hand...
    if (*videoDeviceName == "gen")
    {
      pVideoDevice = media::VirtualVideoDeviceV2::create(serviceManager.getIoService(), *videoDeviceName, rfc6184::H264, uiFps,
                                                         media::parsePayload(ac.Video.PayloadType),
                                                         ac.Video.PayloadSizeString,
                                                         ac.Video.PayloadChar,
                                                         ac.Video.TotalSamples);
    }
    else
    {
      // we currently only support H.264 video
      pVideoDevice = media::VirtualVideoDeviceV2::create(serviceManager.getIoService(), *videoDeviceName, rfc6184::H264, uiFps);
    }
    if (pVideoDevice)
    {
      boost::system::error_code ec;

      // check if local interfaces have been specified on the command line
      boost::optional<std::vector<std::string> > localInterfaces = applicationParameters.getStringsParameter(ApplicationParameters::local_interfaces);
      // network manager
      MediaSessionNetworkManager mediaSessionNetworkManager(serviceManager.getIoService(), *localInterfaces);
      // SIP context
      SipContext context(*sipUser, sFqdn, sDomain, false, ac.SipClient.SipPort);
      // Offer answer
      bool bRapidSync = rapidSync ? true : false;
      std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswerModel = std::shared_ptr<rfc3264::OfferAnswerModel>(new rfc3264::OfferAnswerModel(context, mediaSessionNetworkManager,
                                                                                                                                              *rtcpMux, *enableMpRtp, bRapidSync,
                                                                                                                                              *rtcpRs, ac.RtpRtcp.RtcpRtpHeaderExt));

#if 1
      loadMediaConfigurationIntoOfferAnswerModel(mediaConfig, pOfferAnswerModel);
#else
      // hard-coding supported codecs for now
      // audio
      // AMR
      boost::optional<rfc4566::RtpMapping> rtpMapAmr(rfc4566::RtpMapping(rfc4867::AMR, rfc4867::AMR_CLOCK_RATE));
      boost::optional<rfc4566::FormatDescription> fmtpAmr(rfc4566::FormatDescription("octet-align=1"));
      pOfferAnswerModel->addFormat(rfc4566::AUDIO, rtpMapAmr, fmtpAmr, 13);
      // dummy opus
      boost::optional<rfc4566::RtpMapping> rtpMapOpus(rfc4566::RtpMapping("opus", "48000"));
      boost::optional<rfc4566::FormatDescription> fmtpOpus(rfc4566::FormatDescription("useinbandfec=1; usedtx=1"));;
      pOfferAnswerModel->addFormat(rfc4566::AUDIO, rtpMapOpus, fmtpOpus, 64);
      // dummy speex
      boost::optional<rfc4566::RtpMapping> rtpMapSpeex(rfc4566::RtpMapping("speex", "8000"));
      boost::optional<rfc4566::FormatDescription> fmtpSpeex(rfc4566::FormatDescription("vbr=on"));      
      pOfferAnswerModel->addFormat(rfc4566::AUDIO, rtpMapSpeex, fmtpSpeex, 64);
      // static payload formats
      const std::string PT_PCMU("0");
      const std::string PT_PCMA("8");
      pOfferAnswerModel->addFormat(rfc4566::AUDIO, PT_PCMU);
      pOfferAnswerModel->addFormat(rfc4566::AUDIO, PT_PCMA);

      // video
      boost::optional<rfc4566::RtpMapping> rtpMapH264(rfc4566::RtpMapping(rfc6184::H264, rfc6184::H264_CLOCK_RATE));
      rfc4566::FormatDescription h264_fmtp;
      boost::optional<rfc4566::FormatDescription> fmtpH264(h264_fmtp);
      std::string sProfileLevelId, sSPropParameterSets;
      ec = pVideoDevice->getParameter(rfc6184::PROFILE_LEVEL_ID, sProfileLevelId);
      if (ec)
      {
        LOG(WARNING) << "Failed to get video device profile level id";
        return -1;
      }

      ec = pVideoDevice->getParameter(rfc6184::SPROP_PARAMETER_SETS, sSPropParameterSets);
      if (ec)
      {
        LOG(WARNING) << "Failed to get video device parameter sets";
        return -1;
      }
      std::ostringstream ostr;
      ostr << rfc6184::PROFILE_LEVEL_ID << "=" << sProfileLevelId << ";";
      ostr << rfc6184::PACKETIZATION_MODE << "=1;";
#if 0
      // Adding this causes offer/answer to fail?
      ostr << rfc6184::SPROP_PARAMETER_SETS << "=" << sSPropParameterSets;
#endif
      fmtpH264->fmtp = ostr.str();
      pOfferAnswerModel->addFormat(rfc4566::VIDEO, rtpMapH264, fmtpH264, 500);
#endif

      // null audio device for testing
      std::shared_ptr<media::IVideoDevice> pAudioDevice;

      /// Multimedia service for simulation of consumption of media
      media::SimpleMultimediaServiceV2 multimediaService;

      // Note: SIP client and video device MUST use same IO service so that we can run everything from
      // one thread
      VLOG(2) << "RTCP-mux: " << *rtcpMux << " MPRTP: " << *enableMpRtp;
      rfc3261::SipAgent sipAgent(ec, serviceManager.getIoService(), applicationParameters, context, pOfferAnswerModel, pVideoDevice, pAudioDevice);
      if (ec)
      {
        LOG(WARNING) << "Failed to create SIP agent";
        return -1;
      }
      // Proxy
      if (sipProxy)
      {
        sipAgent.setRegisterServer(*sipProxy);
        bool bUseSipProxy = useSipProxy ? *useSipProxy : false;
        sipAgent.setUseSipProxy(bUseSipProxy);
        bool bRegisterWithProxy = registerWithProxy ? *registerWithProxy : false;
        sipAgent.setRegisterProxy(bRegisterWithProxy);
      }
      // INVITE handler
      sipAgent.setOnInviteHandler(boost::bind(&inviteHandler, _1, _2, boost::ref(sipAgent), boost::ref(serviceManager.getIoService())));
      // INVITE response handler
      sipAgent.setInviteResponseHandler(boost::bind(&inviteResponseHandler, _1, _2, _3));
      // REGISTER response handler
      sipAgent.setRegisterResponseHandler(boost::bind(&registerResponseHandler, _1, _2, _3));
      // Set handler to be called when call starts or fails
      sipAgent.setOnSessionStartHandler(boost::bind(&startApplicationHandler, _1, boost::ref(sipAgent), boost::ref(multimediaService)));
      // Set handler to be called when call finishes or fails
      sipAgent.setOnSessionEndHandler(boost::bind(&endCallHandler, _1, boost::ref(sipAgent), boost::ref(multimediaService)));

      // register SIP client with manager
      uint32_t uiServiceId;
      bool bSuccess = serviceManager.registerService(boost::bind(&rfc3261::SipAgent::start, boost::ref(sipAgent)),
                                                     boost::bind(&rfc3261::SipAgent::stop, boost::ref(sipAgent)),
                                                     uiServiceId);
      if (bSuccess)
      {
        if (sipCallee)
          sipAgent.call(*sipCallee);

        uint32_t uiDurationMs = duration ? *duration : 0;
        serviceManager.setDurationMs(uiDurationMs);
        startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
          boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));
      }
      else
      {
        LOG(WARNING) << "Failed to register service";
      }
    }
    else
    {
      LOG(WARNING) << "Invalid video device: " << *videoDeviceName;
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

