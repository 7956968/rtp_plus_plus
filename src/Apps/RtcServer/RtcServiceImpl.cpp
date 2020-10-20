#include "RtcServerPch.h"
#include "RtcServiceImpl.h"
#include <boost/bind.hpp>
#include <grpc++/status.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/application/ApplicationUtil.h>
#include <rtp++/experimental/GoogleRemb.h>
#include <rtp++/experimental/Nada.h>
#include <rtp++/experimental/Scream.h>
#include <rtp++/media/MediaModels.h>
#include <rtp++/media/VideoInputSource.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/rfc3264/OfferAnswerModel.h>
#include <rtp++/rfc4566/MediaDescription.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rfc4588/Rfc4588.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/scheduling/SchedulerFactory.h>

using grpc::Status;

using rtp_plus_plus::app::ApplicationParameters;
using rtp_plus_plus::MediaSessionDescription;
using rtp_plus_plus::SimpleMediaSessionV2;
using rtp_plus_plus::SimpleMediaSessionFactory;

namespace rtp_plus_plus {

void addFeedbackTypesRequiredForScheduler(uint32_t uiSchedulerType, std::vector<std::string>& RtcpFb)
{
  if (uiSchedulerType == SchedulerFactory::SCREAM_SP_SCHEDULER)
  {
    // add scream FB if not explicitly added
    // offerAnswer.enableScreamFeedback(true);
    auto it = std::find_if(RtcpFb.begin(), RtcpFb.end(), [](const std::string& sFb){ return sFb == experimental::SCREAM; });
    if (it == RtcpFb.end())
    {
      LOG(WARNING) << "Scream scheduler selected, but not scream fb configured. Adding scream FB";
      RtcpFb.push_back(experimental::SCREAM);
    }
  }
  else if (uiSchedulerType == SchedulerFactory::NADA_SP_SCHEDULER)
  {
    // add NADA FB if not explicitly added
    // offerAnswer.enableScreamFeedback(true);
    auto it = std::find_if(RtcpFb.begin(), RtcpFb.end(), [](const std::string& sFb){ return sFb == experimental::NADA; });
    if (it == RtcpFb.end())
    {
      LOG(WARNING) << "NADAscheduler selected, but not scream fb configured. Adding NADA FB";
      RtcpFb.push_back(experimental::NADA);
    }
  }
  else if (uiSchedulerType == SchedulerFactory::ACK_SP_SCHEDULER)
  {
    // add ACK FB if not explicitly added
    auto it = std::find_if(RtcpFb.begin(), RtcpFb.end(), [](const std::string& sFb){ return sFb == rfc4585::ACK; });
    if (it == RtcpFb.end())
    {
      LOG(WARNING) << "ACK scheduler selected, but not scream fb configured. Adding ACK FB";
      RtcpFb.push_back(rfc4585::ACK);
    }
    // add GOOG-REMB
    it = std::find_if(RtcpFb.begin(), RtcpFb.end(), [](const std::string& sFb){ return sFb == experimental::GOOG_REMB; });
    if (it == RtcpFb.end())
    {
      LOG(WARNING) << "ACK scheduler selected, but not REMB fb configured. Adding REMB FB";
      RtcpFb.push_back(experimental::GOOG_REMB);
    }
  }
}

RtcServiceImpl::RtcServiceImpl(const RtcConfig &rtcConfig)
  :m_rtcConfig(rtcConfig),
    m_pThread(nullptr),
    m_pModel(nullptr),
    m_pDataModel(nullptr),

    m_mediaSessionNetworkManager(m_serviceManager.getIoService(), rtcConfig.LocalInterfaces)
{
  // set default application parameters
  // Log RTP session loss statistics
  m_applicationParameters.setBoolParameter(ApplicationParameters::analyse, true);
  // RTP SSRC
  m_applicationParameters.setUintParameter(ApplicationParameters::rtp_ssrc, rtcConfig.RtpSsrc);
  // RTP SN
  m_applicationParameters.setUintParameter(ApplicationParameters::rtp_sn, rtcConfig.RtpSn);
  // RTP TS
  m_applicationParameters.setUintParameter(ApplicationParameters::rtp_ts, rtcConfig.RtpTs);
  // RTP rapid sync mode
  m_applicationParameters.setUintParameter(ApplicationParameters::rapid_sync_mode, rtcConfig.RapidSyncMode);
  m_applicationParameters.setBoolParameter(ApplicationParameters::extract_ntp_ts, rtcConfig.ExtractNtp);
  // receiver buffer latency
  m_applicationParameters.setUintParameter(ApplicationParameters::buf_lat, rtcConfig.BufferLatency);
  // scheduling
  if (rtcConfig.Scheduler != UINT32_MAX)
  {
    m_applicationParameters.setUintParameter(ApplicationParameters::scheduler, rtcConfig.Scheduler);
  }

  // add required feedback types for CC in case these haven't been specified
#if 1
  boost::optional<uint32_t> scheduler = m_applicationParameters.getUintParameter(ApplicationParameters::scheduler);
  uint32_t uiSchedulerType = (scheduler ? *scheduler : SchedulerFactory::BASE_SP_SCHEDULER);
  addFeedbackTypesRequiredForScheduler(uiSchedulerType, m_rtcConfig.RtcpFb);
#endif

  m_applicationParameters.setStringParameter(ApplicationParameters::scheduler_param, rtcConfig.SchedulerParam);
  m_applicationParameters.setBoolParameter(ApplicationParameters::rtp_summarise_stats, rtcConfig.SummariseStats);

  m_sipContext.User = "-";
  m_sipContext.FQDN = m_rtcConfig.PublicIp;

  if (boost::filesystem::exists(rtcConfig.VideoDevice))
  {
    // RTP FPS
    m_applicationParameters.setUintParameter(ApplicationParameters::video_fps, rtcConfig.Fps);
    // RTP MIN KBPS
    m_applicationParameters.setUintParameter(ApplicationParameters::video_min_kbps, rtcConfig.VideoMinKbps);
    // RTP MAX KBPS
    m_applicationParameters.setUintParameter(ApplicationParameters::video_max_kbps, rtcConfig.VideoMaxKbps);

    VLOG(2) << "Creating virtual video device for " << rtcConfig.VideoDevice;
    m_pVideoDevice = media::VirtualVideoDeviceV2::create(m_serviceManager.getIoService(), rtcConfig.VideoDevice, rtcConfig.VideoCodec,
                                                         rtcConfig.Fps, true, rtcConfig.LoopCount > 0, rtcConfig.LoopCount,
                                                         rtcConfig.Width, rtcConfig.Height, rtcConfig.VideoCodecImpl, rtcConfig.VideoCodecParams);
    assert (m_pVideoDevice);
    m_pVideoDevice->setCompletionHandler(boost::bind(&RtcServiceImpl::handleMediaSourceEofV2, this, _1));
  }
  else
  {
    m_pModel = new DynamicBitrateVideoModel(rtcConfig.Fps, rtcConfig.VideoKbps);
    if (rtcConfig.VideoCodec == rfc6184::H264 )
    {
      // generate H264-like data
      m_pDataModel = new H264DataModel();
    }
    else
    {
      // generate random data
      m_pDataModel = new IDataModel();
    }

    m_pVideoSource = VideoInputSource::create(m_serviceManager.getIoService(), rtcConfig.VideoMediaType, rtcConfig.Fps, m_pModel, m_pDataModel);
  }
  // io thread
  m_pWork = std::unique_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(m_serviceManager.getIoService()));
  m_pThread = std::unique_ptr<std::thread>(new std::thread(boost::bind(&boost::asio::io_service::run, boost::ref(m_serviceManager.getIoService()))));
}

RtcServiceImpl::~RtcServiceImpl()
{
  LOG(INFO) << "Stopping media source";
  m_pWork.reset();
  // cancel io service events and then wait for thread
  if (m_pVideoSource)
  {
    LOG(INFO) << "Stopping video source";
    m_pVideoSource->stop();
    m_pVideoSource.reset();
  }
  if (m_pVideoDevice)
  {
    LOG(INFO) << "Stopping video device";
    m_pVideoDevice->stop();
    m_pVideoDevice.reset();
  }

  LOG(INFO) << "Stopping media session";
  if (m_pMediaSession)
  {
    m_pMediaSession->stop();
    m_pMediaSession.reset();
  }
  LOG(INFO) << "Waiting for media thread";
  m_pThread->join();
  LOG(INFO) << "Media thread done";

  if (m_pDataModel)
  {
    delete m_pDataModel;
    m_pDataModel = NULL;
  }

  if (m_pModel)
  {
    delete m_pModel;
    m_pModel = NULL;
  }
}

void RtcServiceImpl::handleMediaSourceEofV2(const boost::system::error_code& ec)
{
  VLOG(2) << "EOF of local media source";
}

::grpc::Status RtcServiceImpl::createOffer(::grpc::ServerContext* context,
                                           const ::peer_connection::OfferDescriptor* request,
                                           ::peer_connection::CreateSessionDescriptionResponse* response)
{
  LOG(INFO) << "RPC createOffer() invoked: " << request->content_type();
  response->set_response_code(200);
  response->set_response_description("OK");

  uint32_t uiMaxBandwidthKbps = 0;
  std::vector<uint32_t> vBitrates = StringTokenizer::tokenizeV2<uint32_t>(m_rtcConfig.VideoKbps, "|", true);
  assert(!vBitrates.empty());
  uiMaxBandwidthKbps = *std::max_element(vBitrates.begin(), vBitrates.end());

  bool bRapidSync = (m_rtcConfig.RapidSyncMode == 0)? false : true;
  rfc3264::OfferAnswerModel offerAnswer(m_sipContext, m_mediaSessionNetworkManager, m_rtcConfig.EnableMpRtp, m_rtcConfig.RtcpMux, bRapidSync, m_rtcConfig.RtcpRs, m_rtcConfig.RtcpRtpHeaderExt);

  offerAnswer.setPreferredProfile(m_rtcConfig.RtpProfile);
  offerAnswer.setPreferredProtocol(m_rtcConfig.Protocol);
  if (m_rtcConfig.RtpPort != 0)
    offerAnswer.setPreferredStartingPort(m_rtcConfig.RtpPort);

  std::string sDynamicFormat;
  offerAnswer.addFormat(sDynamicFormat, rfc4566::VIDEO, rfc4566::RtpMapping(m_rtcConfig.VideoCodec, "90000"), rfc4566::FormatDescription("packetization-mode=1"),
                        uiMaxBandwidthKbps, m_rtcConfig.RtcpFb, m_rtcConfig.RtcpXr);

  // RTX parameters
  if (m_rtcConfig.RtxTime > 0)
  {
    std::ostringstream ostr;
    ostr << "apt=" << sDynamicFormat << ";rtx-time=" << m_rtcConfig.RtxTime;
    std::string sRtxPayloadFormat;
    offerAnswer.addFormat(sRtxPayloadFormat, rfc4566::VIDEO, rfc4566::RtpMapping(rfc4588::RTX, "90000"), rfc4566::FormatDescription(ostr.str()), 0);
  }

  boost::optional<rfc4566::SessionDescription> offer = offerAnswer.generateOffer();
  assert(offer);
  m_localDescription = offer;

  // update c-lines before sending offer
  updateLocalConnectionDataInCaseOfNat(*offer);
  const std::string sOffer = offer->toString();
  VLOG(2) << "Generated offer: " << sOffer;
  peer_connection::SessionDescription* pSessionDescription = new peer_connection::SessionDescription();
  pSessionDescription->set_content_type("application/sdp");
  pSessionDescription->set_session_description(sOffer);
  response->set_allocated_session_description(pSessionDescription);
  return Status::OK;
}

::grpc::Status RtcServiceImpl::createAnswer(::grpc::ServerContext* context,
                                      const ::peer_connection::AnswerDescriptor* request,
                                      ::peer_connection::CreateSessionDescriptionResponse* response)
{
  LOG(INFO) << "RPC createAnswer() invoked: " << request->session_description().content_type();
  response->set_response_code(200);
  response->set_response_description("OK");
  const std::string sOffer = request->session_description().session_description();
  VLOG(2) << "Received offer: " << std::endl << sOffer;

  boost::optional<rfc4566::SessionDescription> offer = rfc4566::SdpParser::parse(sOffer);
  assert(offer);

  uint32_t uiMaxBandwidthKbps = 0;
  std::vector<uint32_t> vBitrates = StringTokenizer::tokenizeV2<uint32_t>(m_rtcConfig.VideoKbps, "|", true);
  assert(!vBitrates.empty());
  uiMaxBandwidthKbps = *std::max_element(vBitrates.begin(), vBitrates.end());

  bool bRapidSync = (m_rtcConfig.RapidSyncMode == 0)? false : true;
  rfc3264::OfferAnswerModel offerAnswer(m_sipContext, m_mediaSessionNetworkManager, m_rtcConfig.EnableMpRtp, m_rtcConfig.RtcpMux, bRapidSync, m_rtcConfig.RtcpRs, m_rtcConfig.RtcpRtpHeaderExt);
  // add the format specified to the offer/answer model
  std::string sPayloadType;
  offerAnswer.addFormat(sPayloadType, rfc4566::VIDEO, rfc4566::RtpMapping(m_rtcConfig.VideoCodec, "90000"), rfc4566::FormatDescription("packetization-mode=1"),
                        uiMaxBandwidthKbps, m_rtcConfig.RtcpFb, m_rtcConfig.RtcpXr);
  if (m_rtcConfig.RtpPort != 0)
    offerAnswer.setPreferredStartingPort(m_rtcConfig.RtpPort);
  // add RTX format here to support RTX: search for nack support in video
  bool bNack = false;
  std::string sApt;
  for (size_t i = 0; i < offer->getMediaDescriptionCount(); ++i)
  {
    rfc4566::MediaDescription& media = offer->getMediaDescription(i);
    if (media.getMediaType() == rfc4566::VIDEO)
    {
      const std::map<std::string, rfc4566::FeedbackDescription>& fb = media.getFeedbackMap();
      // check for RTX
      for (auto& pair : fb)
      {
        std::vector<std::string> vFbTypes = pair.second.FeedbackTypes;
        for (const std::string& sFb : vFbTypes)
        {
          if (sFb == rfc4585::NACK)
          {
            sApt = pair.first;
            bNack = true;
            break;
          }
        }
        if (bNack)
          break;
      }
    }
  }
  if (bNack)
  {
    std::ostringstream ostr;
    ostr << "apt=" << sApt << ";rtx-time=" << m_rtcConfig.RtxTime;
    std::string sRtxPayloadFormat;
    offerAnswer.addFormat(sRtxPayloadFormat, rfc4566::VIDEO, rfc4566::RtpMapping(rfc4588::RTX, "90000"), rfc4566::FormatDescription(ostr.str()), 0);
    VLOG(2) << "Adding RTX PT: " << sRtxPayloadFormat << " " << ostr.str();
  }
  boost::optional<rfc4566::SessionDescription> answer = offerAnswer.generateAnswer(*offer);
  assert(answer);
  m_localDescription = answer;

  // update c-lines before sending offer
  updateLocalConnectionDataInCaseOfNat(*answer);

  m_remoteDescription = offer;
  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
                                                                            *m_localDescription, *m_remoteDescription);
  // TODO: change application parameters into struct storing SimpleMediaSession-specific parameters
  SimpleMediaSessionFactory factory;
  ExistingConnectionAdapter adapter(&m_mediaSessionNetworkManager.getPortAllocationManager());
  m_pMediaSession = factory.create(m_serviceManager.getIoService(), adapter, mediaSessionDescription, m_applicationParameters, m_pVideoDevice);
  // connect source device
  if (m_pVideoSource)
  {
    m_pVideoSource->setReceiveMediaCB(boost::bind(&app::ApplicationUtil::handleVideoSampleV2, _1, m_pMediaSession));
  }
  else
  {
    assert(m_pVideoDevice);
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&app::ApplicationUtil::handleVideoAccessUnitV2, _1, m_pMediaSession));
  }
  // connect video observer
  m_pMediaSession->getVideoSource().registerObserver(boost::bind(&RtcServiceImpl::onVideoReceived, this));

  const std::string sAnswer = answer->toString();
  peer_connection::SessionDescription* pSessionDescription = new peer_connection::SessionDescription();
  pSessionDescription->set_content_type("application/sdp");
  pSessionDescription->set_session_description(sAnswer);
  response->set_allocated_session_description(pSessionDescription);
  return Status::OK;
}

void RtcServiceImpl::updateLocalConnectionDataInCaseOfNat(rfc4566::SessionDescription& localSdp)
{
  // go through connection data on session and media level and update to FQDN
  VLOG(2) << "Updating session level connection from " << localSdp.getConnection().ConnectionAddress << " to " << m_sipContext.FQDN;
  localSdp.getConnection().ConnectionAddress = m_sipContext.FQDN;
#if 0
  std::string sAddress = localSdp.getConnection().ConnectionAddress;
  if (sAddress != "0.0.0.0")
  {
    // make sure address is in local interface list, else replace with 0.0.0.0
    if (!m_mediaSessionNetworkManager.hasInterface(sAddress))
    {
      LOG(WARNING) << "Local session interface does not exist: " << sAddress << ". Replacing with 0.0.0.0";
      localSdp.getConnection().ConnectionAddress = "0.0.0.0";
    }
  }
#endif
  for (size_t i = 0; i < localSdp.getMediaDescriptionCount(); ++i)
  {
    rfc4566::MediaDescription& media = localSdp.getMediaDescription(i);
//    LOG(WARNING) << "Local media interface does not exist: " << sAddress << ". Replacing with 0.0.0.0";
    VLOG(2) << "Updating media level connection from " << media.getConnection().ConnectionAddress << " to " << m_sipContext.FQDN;
    media.getConnection().ConnectionAddress = m_sipContext.FQDN;
#if 0
    std::string sAddress = media.getConnection().ConnectionAddress;
    if (sAddress != "0.0.0.0")
    {
      // make sure address is in local interface list, else replace with 0.0.0.0
      if (!m_mediaSessionNetworkManager.hasInterface(sAddress))
      {
        LOG(WARNING) << "Local media interface does not exist: " << sAddress << ". Replacing with 0.0.0.0";
        media.getConnection().ConnectionAddress = "0.0.0.0";
      }
    }
#endif
  }
}

::grpc::Status RtcServiceImpl::setRemoteDescription(::grpc::ServerContext* context,
                                                    const ::peer_connection::SessionDescription* request,
                                                    ::peer_connection::SetSessionDescriptionResponse* response)
{
  LOG(INFO) << "RPC setRemoteDescription() invoked: " << request->session_description();
  response->set_response_code(200);
  response->set_response_description("OK");
  const std::string sRemoteSdp = request->session_description();
  boost::optional<rfc4566::SessionDescription> remoteSdp = rfc4566::SdpParser::parse(sRemoteSdp);
  assert(remoteSdp);
  m_remoteDescription = remoteSdp;
  assert(m_localDescription);

  // perform SDP to RtpSessionParameter translation
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(),
                                                                            *m_localDescription, *m_remoteDescription);

  SimpleMediaSessionFactory factory;
  ExistingConnectionAdapter adapter(&m_mediaSessionNetworkManager.getPortAllocationManager());
  m_pMediaSession = factory.create(m_serviceManager.getIoService(), adapter, mediaSessionDescription, m_applicationParameters, m_pVideoDevice);
  // connect video observer
  m_pMediaSession->getVideoSource().registerObserver(boost::bind(&RtcServiceImpl::onVideoReceived, this));

  if (m_pVideoSource)
    m_pVideoSource->setReceiveMediaCB(boost::bind(&app::ApplicationUtil::handleVideoSampleV2, _1, m_pMediaSession));
  if (m_pVideoDevice)
  {
    m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&app::ApplicationUtil::handleVideoAccessUnitV2, _1, m_pMediaSession));
  }
  return Status::OK;
}

void RtcServiceImpl::onVideoReceived()
{
  std::vector<media::MediaSample> mediaSamples = m_pMediaSession->getVideoSource().getNextSample();
  VLOG(2) << "Received " << mediaSamples.size() << " video sample(s)";
  // assuming H.264 for now
  for (media::MediaSample& mediaSample : mediaSamples)
  {
    media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
    VLOG(2) << "NALU Size: " << mediaSample.getPayloadSize() << " Type info: " << media::h264::toString(eType);
#if 0
    for (int i = 0; i < 5; ++i)
    {
      VLOG(2) << "NALU[" << i << "]: " << (int)mediaSample.getDataBuffer().data()[i];
    }
#endif
  }
}

void RtcServiceImpl::doStartStreaming()
{
  LOG(INFO) << "RtcServiceImpl::doStartStreaming";
  boost::system::error_code ec;

  // start media session first, other packetisation will fail
  ec = m_pMediaSession->start();
  assert(!ec);

  if (m_pVideoSource)
    ec = m_pVideoSource->start();
  else
  {
    assert(m_pVideoDevice);
    ec = m_pVideoDevice->start();
  }
  assert(!ec);

}

::grpc::Status RtcServiceImpl::startStreaming(::grpc::ServerContext* context,
                                        const ::peer_connection::StartStreamingRequest* request,
                                        ::peer_connection::StartStreamingResponse* response)
{
  LOG(INFO) << "RPC startStreaming() invoked";
  response->set_response_code(200);
  response->set_response_description("OK");


#if 0
  boost::system::error_code ec;
  ec = m_pVideoSource->start();
  assert(!ec);

  // start media session first, other packetisation will fail
  ec = m_pMediaSession->start();
  assert(!ec);
#else
  // do this in io_service thread to avoid blocking with SCTP
  m_serviceManager.getIoService().post(boost::bind(&RtcServiceImpl::doStartStreaming, this));
#endif
  return Status::OK;
}

::grpc::Status RtcServiceImpl::stopStreaming(::grpc::ServerContext* context,
                                     const ::peer_connection::StopStreamingRequest* request,
                                     ::peer_connection::StopStreamingResponse* response)
{
  LOG(INFO) << "RPC stopStreaming() invoked";
  response->set_response_code(200);
  response->set_response_description("OK");

  boost::system::error_code ec;

  if (m_pVideoSource)
    ec = m_pVideoSource->stop();
  else
  {
    assert(m_pVideoDevice);
    ec = m_pVideoDevice->stop();
  }
  assert(!ec);

  ec = m_pMediaSession->stop();
  assert(!ec);

  return Status::OK;
}

::grpc::Status RtcServiceImpl::shutdown(::grpc::ServerContext* context,
                                const ::peer_connection::ShutdownRequest* request,
                                ::peer_connection::ShutdownResponse* response)
{
  LOG(INFO) << "RPC shutdown() invoked";
  response->set_response_code(200);
  response->set_response_description("OK");

  LOG(INFO) << "Generating Ctrl-C";
  kill(getpid(), SIGINT);

  return Status::OK;
}

} // rtp_plus_plus
