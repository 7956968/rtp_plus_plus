#pragma once
#include <iostream>
#include <fstream>
#include <boost/system/error_code.hpp>
#include <cpputil/MakeService.h>
#include <rtp++/IRtpSessionManager.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/media/MediaConsumerService.h>
#include <rtp++/media/MediaGeneratorService.h>
#include <rtp++/media/StreamMediaSource.h>
#include <rtp++/media/VideoInputSource.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/application/ApplicationUtil.h>

namespace rtp_plus_plus
{
// fwd
class IVideoModel;
class IDataModel;

namespace media
{


/**
 * @brief The SimpleMultimediaServiceV2 class supports producing/consuming single multimedia sources
 * such as a single video and/or audio stream
 */
class SimpleMultimediaServiceV2
{
public:

  /**
   * @brief SimpleMultimediaServiceV2 Constructor
   */
  SimpleMultimediaServiceV2();

  /**
   * @brief SimpleMultimediaServiceV2 Destructor
   * Stops services if they have not been stopped yet.
   */
  ~SimpleMultimediaServiceV2();

  /**
   * @brief createAudioConsumer creates an audio consumer that writes received samples to file or std out
   * WARNING: cout should only be used if there is one consumer in the multimedia service.
   * @param audioSource Audio source object that samples are to retrieved from
   * @param sOutput Output file name or "cout" to write to standard out.
   * @param uiIntervalMicrosecs The interval in which samples will be retrieved from the source
   * @return boost::system::error_code()
   */
  boost::system::error_code createAudioConsumer(IMediaSampleSource& audioSource, const std::string& sOutput,
                                                const RtpSessionParameters& rtpParameters, unsigned uiIntervalMicrosecs = 10000);

  /**
   * @brief createVideoConsumer creates an H.264 video consumer that writes received samples to file or std out
   * HACK: only currently supporting H264 in callback.
   * WARNING: cout should only be used if there is one consumer in the multimedia service.
   * @param videoSource Video source object that samples are to retrieved from
   * @param sOutput Output file name or "cout" to write to standard out.
   * @param uiIntervalMicrosecs The interval in which samples will be retrieved from the source
   * @return boost::system::error_code()
   */
  boost::system::error_code createVideoConsumer(IMediaSampleSource& videoSource, const std::string& sOutput,
                                                const RtpSessionParameters& rtpParameters, unsigned uiIntervalMicrosecs = 10000);

  /**
   * @brief createVideoProducer
   * @param pMediaSession
   * @param sSource Filename, 'stdin' or 'gen'
   * @param uiFps Framerate of video
   * @param bLimitRateToFramerate If using a file source, whether the rate of reading samples should be limited to uiFps
   * @param sPayloadType [[if sSource = 'gen']] Video payload type for generated media [rand,char,ntp]
   * @param sPayloadSizeString [[if sSource = 'gen']] Video payload sizes to be generated
   * @param uiPayloadChar [[if sSource = 'gen' && sPayloadType='char' ]] Video Payload char
   * @param uiTotalSamples [[if sSource = 'gen']] Total samples to be generated before EOF
   * @param bLoop [[if sSource = file ]] if the source should be looped on EOF
   * @param uiLoopCount [[if sSource = file ]] How many repetitions of the file should be played
   * @param uiDurationMilliSeconds [[if sSource = gen ]] Duration in seconds that should be played back
   * @return boost::system::error_code() on success, bad_file_descriptor or not_supported on failure
   */
  boost::system::error_code createVideoProducer(boost::shared_ptr<SimpleMediaSessionV2> pMediaSession,
                                                const std::string& sSource, uint32_t uiFps, bool bLimitRateToFramerate,
                                                const std::string& sPayloadType, const std::string& sPayloadSizeString,
                                                const uint8_t uiPayloadChar, uint32_t uiTotalSamples,
                                                bool bLoop, uint32_t uiLoopCount = 0,
                                                uint32_t uiDurationMilliSeconds = 0,
                                                const std::string &sVideoBitrates = "1000",
                                                const uint32_t uiWidth = 0,
                                                const uint32_t uiHeight = 0,
                                                const std::string& sVideoCodecImpl = "",
                                                const std::vector<std::string>& videoCodecParameters = std::vector<std::string>());

  /**
   * @brief createAudioProducer creates audio source
   * @param pMediaSession The simple media sesssion to deliver media
   * @param uiRateMs Audio rate in ms
   * @param sPayloadType Audio payload type for generated media [rand,char,ntp]
   * @param sPayloadSizeString Audio payload sizes to be generated
   * @param uiPayloadChar [[ if sPayloadType='char' ]] Payload char
   * @param uiTotalSamples Total samples to be generated before eof
   * @param uiDurationMilliSeconds [[if sSource = gen ]] Duration in seconds that should be played back
   * @return boost::system::error_code();
   */
  boost::system::error_code createAudioProducer(boost::shared_ptr<SimpleMediaSessionV2> pMediaSession, uint32_t uiRateMs,
                                                const std::string& sPayloadType, const std::string& sPayloadSizeString,
                                                const uint8_t uiPayloadChar, uint32_t uiTotalSamples,
                                                uint32_t uiDurationMilliSeconds = 0);

  /**
   * @brief startServices starts all configured multimedia services
   * The service must be ready and there must be at least one configured service
   * @return boost::system::error_code() on success, boost::system::errc::operation_not_permitted on failure
   */
  boost::system::error_code startServices();

  /**
   * @brief stopServices stops all configured multimedia services.
   * The service must be running
   * @return boost::system::error_code() on success, boost::system::errc::operation_not_permitted on failure
   */
  boost::system::error_code stopServices();

private:
  void startConsumers();
  void startProducers();
  void stopConsumers();
  void stopProducers();

  private:
  enum State
  {
    ST_READY,
    ST_RUNNING
  };
  State m_eState;

  uint32_t m_uiRunningServiceCount;

  // start consumer service that requests samples every 10 ms
  MediaConsumerServicePtr m_pAudioConsumer;
  MediaConsumerServicePtr m_pVideoConsumer;
  std::vector<media::MediaSink*> m_vMediaSinks;

  MediaGeneratorServicePtr m_pAudioService;
  MediaGeneratorServicePtr m_pVideoService;
  StreamMediaSourceServicePtr m_pStreamVideoService;
  std::shared_ptr<media::VirtualVideoDeviceV2> m_pVideoDevice;

  IVideoModel* m_pModel;
  IDataModel* m_pDataModel;
  std::shared_ptr<VideoInputSource> m_pVideoSource;

  std::ifstream m_in1;
};

} // media
} // rtp_plus_plus
