#include "CorePch.h"
#include <boost/bind.hpp>
#include <rtp++/util/Base64.h>
#include <rtp++/media/MediaSink.h>
#include <rtp++/media/SimpleMultimediaService.h>
#include <rtp++/media/h264/H264FormatDescription.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>

namespace rtp_plus_plus
{
namespace media
{

using app::ApplicationUtil;

SimpleMultimediaService::SimpleMultimediaService()
  :m_eState(ST_READY),
    m_uiRunningServiceCount(0)
{

}

SimpleMultimediaService::~SimpleMultimediaService()
{
  if (m_eState == ST_RUNNING)
  {
    LOG(WARNING) << "Services have not been stopped!!! Stopping services";
    stopServices();
  }
}

boost::system::error_code SimpleMultimediaService::createAudioConsumer(IMediaSampleSource& audioSource, const std::string& sOutput,
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

boost::system::error_code SimpleMultimediaService::createVideoConsumer(IMediaSampleSource& videoSource, const std::string& sOutput,
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
SimpleMultimediaService::createVideoProducer(boost::shared_ptr<SimpleMediaSession> pMediaSession,
                                             const std::string& sSource,
                                             uint32_t uiFps, bool bLimitRateToFramerate,
                                             const std::string& sPayloadType,
                                             const std::string& sPayloadSizeString,
                                             const uint8_t uiPayloadChar, uint32_t uiTotalSamples,
                                             bool bLoop, uint32_t uiLoopCount,
                                             uint32_t uiDurationMilliSeconds)
{
  const std::string sMediaType = pMediaSession->getVideoSessionManager()->getPrimaryMediaType();
  // Only a single stream can be read from stdin
  bool bGen = false, bStdin = false;
  if (!ApplicationUtil::checkAndInitialiseSource(sSource, bGen, bStdin, m_in1))
  {
    return boost::system::error_code(boost::system::errc::bad_file_descriptor, boost::system::get_generic_category());
  }

  if (bStdin || !bGen)
  {
    // Only supporting H.264 at the moment
    if (sMediaType != rfc6184::H264 && sMediaType != rfc6190::H264_SVC &&
        sMediaType != rfchevc::H265 )
    {
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
    }
  }

  if (bStdin)
  {
    m_pStreamVideoService = makeService<media::StreamMediaSource>(boost::ref(std::cin), sMediaType, 10000, false, 0, uiFps, bLimitRateToFramerate, StreamMediaSource::AccessUnit);
    m_pStreamVideoService->get()->setReceiveAccessUnitCB(boost::bind(&ApplicationUtil::handleVideoAccessUnit, _1, pMediaSession));
  }
  else
  {
    if (bGen)
    {
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
      m_pVideoService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleVideoSample, _1, pMediaSession));
      m_pVideoService->setCompletionHandler(boost::bind(&ApplicationUtil::handleMediaSourceEof, _1, pMediaSession));
    }
    else
    {
      m_pStreamVideoService = makeService<media::StreamMediaSource>(boost::ref(m_in1), sMediaType, 10000, bLoop, uiLoopCount, uiFps, bLimitRateToFramerate, StreamMediaSource::AccessUnit);
      m_pStreamVideoService->get()->setReceiveAccessUnitCB(boost::bind(&ApplicationUtil::handleVideoAccessUnit, _1, pMediaSession));
      m_pStreamVideoService->setCompletionHandler(boost::bind(&ApplicationUtil::handleMediaSourceEof, _1, pMediaSession));
    }
  }
  return boost::system::error_code();
}

boost::system::error_code SimpleMultimediaService::createAudioProducer(boost::shared_ptr<SimpleMediaSession> pMediaSession, uint32_t uiRateMs,
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
  m_pAudioService->get()->setReceiveMediaCB(boost::bind(&ApplicationUtil::handleAudioSample, _1, pMediaSession));
  return boost::system::error_code();
}

boost::system::error_code SimpleMultimediaService::startServices()
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
    VLOG(5) << m_uiRunningServiceCount << " services started";
    return boost::system::error_code();
  }
}

boost::system::error_code SimpleMultimediaService::stopServices()
{
  if (m_eState == ST_READY)
  {
    LOG(WARNING) << "Services are stopped already";
    return boost::system::error_code(boost::system::errc::operation_not_permitted, boost::system::get_generic_category());
  }
  stopConsumers();
  stopProducers();

  m_eState = ST_READY;
  m_uiRunningServiceCount = 0;
  return boost::system::error_code();
}

void SimpleMultimediaService::startConsumers()
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

void SimpleMultimediaService::startProducers()
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
}

void SimpleMultimediaService::stopConsumers()
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

void SimpleMultimediaService::stopProducers()
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

  VLOG(10) << "Stopping audio service";
  if (m_pAudioService && m_pAudioService->isRunning())
    m_pAudioService->stop();

  VLOG(10) << "Producers stopped";
}

}
}
