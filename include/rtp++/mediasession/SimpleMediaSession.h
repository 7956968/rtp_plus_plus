#pragma once
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceController.h>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{

// Fwd
class IRtpSessionManager;

/**
  * A simple media session consists of exactly one video and one audio stream
  * handles the event loop for the RTP sessions
  */
class SimpleMediaSession : public ServiceController
{
public:

  SimpleMediaSession();

  virtual ~SimpleMediaSession();

  /**
    * This should only be called if hasVideo() == true
    */
  std::unique_ptr<IRtpSessionManager>& getVideoSessionManager() { return m_pVideoManager; }

  void setVideoSessionManager(std::unique_ptr<IRtpSessionManager> pVideoManager);
  bool hasVideo() const;
  bool sendVideo(const media::MediaSample& mediaSample);
  bool sendVideo(const std::vector<media::MediaSample>& vMediaSamples);
  bool isVideoAvailable() const;
  std::vector<media::MediaSample> getVideoSample();
  media::IMediaSampleSource& getVideoSource()
  {
    return m_videoSource;
  }

  /**
    * This should only be called if hasAudio() == true
    */
  std::unique_ptr<IRtpSessionManager>& getAudioSessionManager() { return m_pAudioManager; }

  void setAudioSessionManager(std::unique_ptr<IRtpSessionManager> pAudioManager);
  bool hasAudio() const;
  bool sendAudio(const media::MediaSample& mediaSample);
  bool sendAudio(const std::vector<media::MediaSample>& vMediaSamples);
  bool isAudioAvailable() const;
  std::vector<media::MediaSample> getAudioSample();
  media::IMediaSampleSource& getAudioSource()
  {
    return m_audioSource;
  }

protected:
  /**
   * @fn  virtual boost::system::error_code MediaSessionManager::doStart()
   *
   * @brief Starts all media sessions managed by this component
   *
   * @return  .
   */
  virtual boost::system::error_code doStart();
  /**
   * @fn  virtual boost::system::error_code MediaSessionManager::doStop()
   *
   * @brief Stops all media sessions managed by this component
   *
   * @return  .
   */
  virtual boost::system::error_code doStop();

private:

  std::unique_ptr<IRtpSessionManager> m_pVideoManager;
  std::unique_ptr<IRtpSessionManager> m_pAudioManager;

  /**
    * Adapter class to avoid passing out references to the pointer
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

  SimpleSourceAdapter m_audioSource;
  SimpleSourceAdapter m_videoSource;
};


}
