#include "CorePch.h"
#include <boost/bind.hpp>
#include <rtp++/util/Base64.h>
#include <rtp++/media/MediaModels.h>
#include <rtp++/media/MediaSink.h>
#include <rtp++/media/MediaTypes.h>
#include <rtp++/media/SimpleMultimediaServiceV2.h>
#include <rtp++/media/h264/H264FormatDescription.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>

namespace rtp_plus_plus
{
namespace media
{

using app::ApplicationUtil;

SimpleMultimediaServiceV2::SimpleMultimediaServiceV2()
  :m_eState(ST_READY),
    m_uiRunningServiceCount(0),
    m_pAudioConsumer(nullptr),
    m_pVideoConsumer(nullptr),
    m_pAudioService(nullptr),
    m_pVideoService(nullptr),
    m_pStreamVideoService(nullptr),
    m_pVideoDevice(nullptr),
    m_pModel(nullptr),
    m_pDataModel(nullptr),
    m_pVideoSource(nullptr)

{

}

SimpleMultimediaServiceV2::~SimpleMultimediaServiceV2()
{
  if (m_eState == ST_RUNNING)
  {
    LOG(WARNING) << "Services have not been stopped!!! Stopping services";
    stopServices();
  }

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

boost::system::error_code SimpleMultimediaServiceV2::createAudioConsumer(IMediaSampleSource& audioSource, const std::string& sOutput,
                                                                       const RtpSessionParameters& rtpParameters, unsigned uiIntervalMicrosecs)
{
  std::string sMediaType = rtpParameters.getEncodingName();
  m_pAudioConsumer = makeService<media::MediaConsumer>(uiIntervalMicrosecs, boost::ref(audioSource));

  VLOG(5) << "Writing media output to " << sOutput;
  media::MediaSink* pMediaSink = new media::MediaSink(sOutput);
  m_vMediaSinks.push_back(pMediaSink);
  m_pAudioConsumer->get()->setSampleCallback(boost::bind(&media::MediaSink::write, pMediaSink, _1));
  return boost::system::error_code();
}

boost::system::error_code SimpleMultimediaServiceV2::createVideoConsumer(IMediaSampleSource& videoSource, const std::string& sOutput,
                                                                       const RtpSessionParameters& rtpParameters, unsigned uiIntervalMicrosecs)
{
  std::string sMediaType = rtpParameters.getEncodingName();
  // TODO: add SPS and PPS if cmd flag is set to every IDR AU:
  // This could be accomplished by:
  // creating a MediaSink consumer class or by maintaining state
  // in this class.
  bool bPrependParameterSets = false;
  std::string sSps, sPps;
  if (sMediaType == rfc6184::H264 )
  {
    std::vector<std::string> fmtp = rtpParameters.getAttributeValues("fmtp");
    if (fmtp.size() > 0)
    {
      std::string sFmtp = fmtp[0];
      media::h264::H264FormatDescription description(sFmtp);
      if (description.Sps != "")
      {
        sSps = base64_decode(description.Sps);
        bPrependParameterSets = true;
      }

      if (description.Pps != "")
      {
        sPps = base64_decode(description.Pps);
      }
    }
  }

  m_pVideoConsumer = makeService<media::MediaConsumer>(uiIntervalMicrosecs, boost::ref(videoSource));

  media::MediaSink* pMediaSink = 0;
  if ( sMediaType == rfc6184::H264 )
  {
    media::h264::H264AnnexBStreamWriter* pH264MediaSink = new media::h264::H264AnnexBStreamWriter(sOutput, bPrependParameterSets);
    pH264MediaSink->setParameterSets(sSps, sPps);
    pMediaSink = pH264MediaSink;
  }
  else if ( sMediaType == rfchevc::H265 )
  {
    pMediaSink = new media::h265::H265AnnexBStreamWriter(sOutput, false);
  }
  else
  {
    pMediaSink = new media::MediaSink(sOutput);
  }
  m_pVideoConsumer->get()->setAccessUnitCallback(boost::bind(&media::MediaSink::writeAu, pMediaSink, _1));
  return boost::system::error_code();
}

boost::system::error_code
SimpleMultimediaServiceV2::createVideoProducer(boost::shared_ptr<SimpleMediaSessionV2> pMediaSession,
                                               const std::string& sSource,
                                               uint32_t uiFps, bool bLimitRateToFramerate,
                                               const std::string& sPayloadType,
                                               const std::string& sPayloadSizeString,
                                               const uint8_t uiPayloadChar, uint32_t uiTotalSamples,
                                               bool bLoop, uint32_t uiLoopCount,
                                               uint32_t uiDurationMilliSeconds,
                                               const std::string& sVideoBitrates,
                                               const uint32_t uiWidth,
                                               const uint32_t uiHeight,
                                               const std::string& sVideoCodecImpl,
                                               const std::vector<std::string>& videoCodecParameters)
{
  VLOG(2) << "FPS: " << uiFps;
  const std::string sMediaType = pMediaSession->getVideoSessionManager()->getPrimaryMediaType();
  // Only a single stream can be read from stdin
  bool bGen = false, bStdin = false;
  if (sSource == "stdin")
  {
    bStdin = true;
  }
  else if (sSource == "gen")
  {
    bGen = true;
  }

  if (bStdin || !bGen)
  {
    // Only supporting H.264 at the moment
    if (sMediaType != rfc6184::H264 && sMediaType != rfc6190::H264_SVC &&
        sMediaType != rfchevc::H265 && sMediaType != media::YUV420P)
    {
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
    }
  }

  if (bStdin)
  {
    VLOG(2) << "Creating virtual input stream video device for " << sSource;
    m_pStreamVideoService = makeService<media::StreamMediaSource>(boost::ref(std::cin), sMediaType, 10000, false, 0, uiFps, bLimitRateToFramerate, StreamMediaSource::AccessUnit);
    m_pStreamVideoService->get()->setReceiveAccessUnitCB(boost::bind(&ApplicationUtil::handleVideoAccessUnitV2, _1, pMediaSession));
  }
  else
  {
    if (bGen)
    {
      VLOG(2) << "Creating virtual dynamic bitrate model video device for " << sSource;
#if 1
      // new video service with bitrate and data models
      // TODO: add this as a parameter of the method, hard-coding for now
      // std::string sVideoKbps = "1000";
      m_pModel = new DynamicBitrateVideoModel(uiFps, sVideoBitrates);
      if (sMediaType == rfc6184::H264 )
      {
        // generate H264-like data
        m_pDataModel = new H264DataModel();
      }
      else
      {
        // generate random data
        m_pDataModel = new IDataModel();
      }

      m_pVideoSource = VideoInputSource::create(pMediaSession->getIoService(), sMediaType, uiFps, m_pModel, m_pDataModel);
      m_pVideoSource->setReceiveMediaCB(boost::bind(&app::ApplicationUtil::handleVideoSampleV2, _1, pMediaSession));

#else
      // OLD video service
      // calculate video parameters
      // framerate
      uint32_t uiIntervalMicrosecs = static_cast<uint32_t>(1000000.0/uiFps + 0.5);
      // payload
      media::PayloadType eType = media::parsePayload(sPayloadType);

      LOG(INFO) << "Video source: " << uiIntervalMicrosecs/1000.0
                << "ms Payload: " << sPayloadType
                << " Sizes: " << sPayloadSizeString
                << " Char Filler: " << uiPayloadChar;

      // start media simulation service in new thread
      m_pVideoService = makeService<media::MediaGenerator>(uiIntervalMicrosecs, eType, sPayloadSizeString, uiPayloadChar, uiTotalSamples, uiDurationMilliSeconds);
      m_pVideoService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleVideoSampleV2, _1, pMediaSession));
      m_pVideoService->setCompletionHandler(boost::bind(&ApplicationUtil::handleMediaSourceEofV2, _1, pMediaSession));
#endif
    }
    else
    {
      // error: we shouldn't use model here: file streaming!
#if 0
      // new video service with bitrate and data models
      // TODO: add this as a parameter of the method, hard-coding for now
      std::string sVideoKbps = "1000";
      m_pModel = new DynamicBitrateVideoModel(uiFps, sVideoKbps);
      if (sMediaType == rfc6184::H264 )
      {
        // generate H264-like data
        m_pDataModel = new H264DataModel();
      }
      else
      {
        // generate random data
        m_pDataModel = new IDataModel();
      }

      m_pVideoSource = VideoInputSource::create(pMediaSession->getIoService(), sMediaType, uiFps, m_pModel, m_pDataModel);
      m_pVideoSource->setReceiveMediaCB(boost::bind(&app::ApplicationUtil::handleVideoSampleV2, _1, pMediaSession));

#else
      VLOG(2) << "Creating virtual video device for " << sSource;
      m_pVideoDevice = media::VirtualVideoDeviceV2::create(pMediaSession->getIoService(), sSource, sMediaType, uiFps, bLimitRateToFramerate, bLoop, uiLoopCount, uiWidth, uiHeight, sVideoCodecImpl, videoCodecParameters);
      if (m_pVideoDevice)
      {
        m_pVideoDevice->setReceiveAccessUnitCB(boost::bind(&ApplicationUtil::handleVideoAccessUnitV2, _1, pMediaSession));
        m_pVideoDevice->setCompletionHandler(boost::bind(&ApplicationUtil::handleMediaSourceEofV2, _1, pMediaSession));
      }
      else
      {
        return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
      }
#endif
    }
  }
  return boost::system::error_code();
}

boost::system::error_code SimpleMultimediaServiceV2::createAudioProducer(boost::shared_ptr<SimpleMediaSessionV2> pMediaSession, uint32_t uiRateMs,
                                                                       const std::string& sPayloadType, const std::string& sPayloadSizeString,
                                                                       const uint8_t uiPayloadChar, uint32_t uiTotalSamples,
                                                                       uint32_t uiDurationMilliSeconds)
{
  // calculate audio parameters
  // payload
  media::PayloadType eType = media::parsePayload(sPayloadType);
  // size
  std::vector<uint32_t> sizes = ApplicationUtil::parseMediaSampleSizeString(sPayloadSizeString);

  LOG(INFO) << "Audio source: " << uiRateMs << "ms Payload: " << sPayloadType
            << " Sizes: " << sPayloadSizeString
            << " Char Filler: " << uiPayloadChar;

  m_pAudioService = makeService<media::MediaGenerator>(uiRateMs * 1000, eType, sPayloadSizeString, uiPayloadChar,uiTotalSamples, uiDurationMilliSeconds);
  m_pAudioService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleAudioSampleV2, _1, pMediaSession));
  return boost::system::error_code();
}

boost::system::error_code SimpleMultimediaServiceV2::startServices()
{
  if (m_eState == ST_RUNNING)
  {
    LOG(WARNING) << "Services are running already";
    return boost::system::error_code(boost::system::errc::operation_not_permitted, boost::system::get_generic_category());
  }
  VLOG(10) << "Starting services";
  startConsumers();
  startProducers();
  if (m_uiRunningServiceCount == 0)
  {
    LOG(WARNING) << "No services configured";
    return boost::system::error_code(boost::system::errc::operation_not_permitted, boost::system::get_generic_category());
  }
  else
  {
    m_eState = ST_RUNNING;
    VLOG(10) << m_uiRunningServiceCount << " services started";
    return boost::system::error_code();
  }
}

boost::system::error_code SimpleMultimediaServiceV2::stopServices()
{
  if (m_eState == ST_READY)
  {
    VLOG(5) << "Services are stopped already";
    return boost::system::error_code(boost::system::errc::operation_not_permitted, boost::system::get_generic_category());
  }
  stopConsumers();
  stopProducers();

  m_eState = ST_READY;
  m_uiRunningServiceCount = 0;
  return boost::system::error_code();
}

void SimpleMultimediaServiceV2::startConsumers()
{
  if (m_pAudioConsumer)
  {
    m_pAudioConsumer->start();
    ++m_uiRunningServiceCount;
  }

  if (m_pVideoConsumer)
  {
    m_pVideoConsumer->start();
    ++m_uiRunningServiceCount;
  }
}

void SimpleMultimediaServiceV2::startProducers()
{
  if (m_pVideoService)
  {
    m_pVideoService->start();
    ++m_uiRunningServiceCount;
  }
  if (m_pStreamVideoService)
  {
    m_pStreamVideoService->start();
    ++m_uiRunningServiceCount;
  }
  if (m_pAudioService)
  {
    m_pAudioService->start();
    ++m_uiRunningServiceCount;
  }
  if (m_pVideoDevice)
  {
    m_pVideoDevice->start();
    ++m_uiRunningServiceCount;
  }

  if (m_pVideoSource)
  {
    m_pVideoSource->start();
    ++m_uiRunningServiceCount;
  }
}

void SimpleMultimediaServiceV2::stopConsumers()
{
  if (m_pAudioConsumer)
    m_pAudioConsumer->stop();

  if (m_pVideoConsumer)
    m_pVideoConsumer->stop();

  // close out files
  for (size_t i = 0; i < m_vMediaSinks.size(); ++i)
  {
    delete m_vMediaSinks[i];
  }
  m_vMediaSinks.clear();
}

void SimpleMultimediaServiceV2::stopProducers()
{
  VLOG(10) << "Stopping video service";
  if (m_pVideoService && m_pVideoService->isRunning())
    m_pVideoService->stop();

  if (m_pStreamVideoService && m_pStreamVideoService->isRunning())
  {
    m_pStreamVideoService->stop();

    if (m_in1.is_open())
      m_in1.close();
  }

  if (m_pVideoDevice)
  {
    m_pVideoDevice->stop();
  }

  if (m_pVideoSource)
    m_pVideoSource->stop();

  VLOG(10) << "Stopping audio service";
  if (m_pAudioService && m_pAudioService->isRunning())
    m_pAudioService->stop();

  VLOG(10) << "Producers stopped";
}

}
}
