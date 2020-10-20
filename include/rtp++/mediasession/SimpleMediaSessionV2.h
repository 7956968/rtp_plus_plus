#pragma once
#include <boost/asio/io_service.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{

// Fwd
class IRtpSessionManager;

/**
 * @brief A simple media session consists of exactly one video and one audio stream
 * handles the event loop for the RTP sessions. This class is more of a manager class
 * for one audio and one video RTP session together.
 */
class SimpleMediaSessionV2
{
  enum State
  {
    ST_READY,
    ST_STARTED
  };

  typedef boost::function<void ()> MediaSessionCompleteHandler_t;

public:

  /**
   * @brief Constructor
   */
  SimpleMediaSessionV2(boost::asio::io_service& ioService);
  /**
   * @brief Destructor
   */
  virtual ~SimpleMediaSessionV2();
  /**
   * @brief Getter for io service
   */
  boost::asio::io_service& getIoService()
  {
    return m_rIoService;
  }
  /**
   * @brief Setter for media session completion callback.
   */
  void setMediaSessionCompleteHandler(MediaSessionCompleteHandler_t handler) { m_mediaSessionCompleteHandler = handler; }
  /**
   * @brief Returns true if media session is in the running state.
   */
  bool isRunning() const { return m_eState == ST_STARTED; }
  /**
   * @brief Getter for video session manager. This should only be called if hasVideo() == true
   */
  std::unique_ptr<IRtpSessionManager>& getVideoSessionManager() { return m_pVideoManager; }
  /**
   * @brief Setter for video session manager. NOTE: this class takes ownership.
   */
  void setVideoSessionManager(std::unique_ptr<IRtpSessionManager> pVideoManager);
  /**
   * @brief Returns true if media session has a video component.
   */
  bool hasVideo() const;
  /**
   * @brief Sends a video sample if the session has a video session manager.
   * @return true if the sample can be sent using the video session manager, false if this has not been configured previously.
   */
  bool sendVideo(const media::MediaSample& mediaSample);
  /**
   * @brief Sends a vector of video samples if the session has a video session manager.
   * @return true if the samples can be sent using the video session manager, false if this has not been configured previously.
   */
  bool sendVideo(const std::vector<media::MediaSample>& vMediaSamples);
  bool isVideoAvailable() const;
  std::vector<media::MediaSample> getVideoSample();
  media::IMediaSampleSource& getVideoSource()
  {
    return m_videoSource;
  }
  /**
   * @brief Getter for audio session manager. This should only be called if hasAudio() == true.
   */
  std::unique_ptr<IRtpSessionManager>& getAudioSessionManager() { return m_pAudioManager; }
  /**
   * @brief Setter for audio session manager. NOTE: this class takes ownership.
   */
  void setAudioSessionManager(std::unique_ptr<IRtpSessionManager> pAudioManager);
  /**
   * @brief Returns true if media session has a audio component.
   */
  bool hasAudio() const;
  /**
   * @brief Sends a audio sample if the session has a audio session manager.
   * @return true if the sample can be sent using the audio session manager, false if this has not been configured previously.
   */
  bool sendAudio(const media::MediaSample& mediaSample);
  /**
   * @brief Sends a vector of audio samples if the session has a audio session manager.
   * @return true if the samples can be sent using the audio session manager, false if this has not been configured previously.
   */
  bool sendAudio(const std::vector<media::MediaSample>& vMediaSamples);
  bool isAudioAvailable() const;
  std::vector<media::MediaSample> getAudioSample();
  media::IMediaSampleSource& getAudioSource()
  {
    return m_audioSource;
  }
  /**
   * @brief starts the media session
   */
  boost::system::error_code start();
  /**
   * @brief stops the media session
   */
  boost::system::error_code stop();

private:

  void handleRtpSessionComplete();

private:
  /// io service
  boost::asio::io_service& m_rIoService;
  /// state of media session
  State m_eState;
  /// video session manager
  std::unique_ptr<IRtpSessionManager> m_pVideoManager;
  /// audio session manager
  std::unique_ptr<IRtpSessionManager> m_pAudioManager;
  /**
   * @brief Adapter class to avoid passing out references to the pointer
   */
  class SimpleSourceAdapter : public media::IMediaSampleSource
  {
  public:
    SimpleSourceAdapter();
    void setRtpSessionManager(IRtpSessionManager* pIRtpSessionManager);
    virtual bool isSampleAvailable() const;
    virtual bool isSampleAvailable(const std::string& sMid) const { return false; }
    virtual std::vector<media::MediaSample> getNextSample();
    virtual std::vector<media::MediaSample> getNextSample(const std::string& sMid) { return std::vector<media::MediaSample>(); }
    void notify();
  private:
    IRtpSessionManager* m_pRtpSessionManager;
  };
  /// adapter for audio source
  SimpleSourceAdapter m_audioSource;
  /// adapter for video source
  SimpleSourceAdapter m_videoSource;
  /// count of RTP sessions complete.
  uint32_t m_uiCompleteCount;
  /// media session completion callback
  MediaSessionCompleteHandler_t m_mediaSessionCompleteHandler;
};

} // rtp_plus_plus
