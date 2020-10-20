#include "CorePch.h"
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/IRtpSessionManager.h>

namespace rtp_plus_plus
{

using media::MediaSample;

SimpleMediaSession::SimpleMediaSession()
{
}

SimpleMediaSession::~SimpleMediaSession()
{
}

bool SimpleMediaSession::hasVideo() const
{
  return m_pVideoManager != 0;
}

bool SimpleMediaSession::hasAudio() const
{
  return m_pAudioManager != 0;
}

void SimpleMediaSession::setVideoSessionManager(std::unique_ptr<IRtpSessionManager> pVideoManager)
{
  m_pVideoManager = std::move(pVideoManager);
  m_videoSource.setRtpSessionManager(m_pVideoManager.get());
}

void SimpleMediaSession::setAudioSessionManager(std::unique_ptr<IRtpSessionManager> pAudioManager)
{
  m_pAudioManager = std::move(pAudioManager);
  m_audioSource.setRtpSessionManager(m_pAudioManager.get());
}

bool SimpleMediaSession::sendVideo(const MediaSample& mediaSample)
{
  if (m_pVideoManager)
  {
    m_pVideoManager->send(mediaSample);
    return true;
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for video in simple media session";
    return false;
  }
}

bool SimpleMediaSession::sendVideo(const std::vector<MediaSample>& vMediaSamples)
{
  if (m_pVideoManager)
  {
    m_pVideoManager->send(vMediaSamples);
    return true;
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for video in simple media session";
    return false;
  }
}

bool SimpleMediaSession::isVideoAvailable() const
{
  if (m_pVideoManager)
  {
    return m_pVideoManager->isSampleAvailable();
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for video in simple media session";
    return false;
  }
}

std::vector<MediaSample> SimpleMediaSession::getVideoSample()
{
  if (m_pVideoManager)
  {
    return m_pVideoManager->getNextSample();
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for video in simple media session";
    return std::vector<MediaSample>();
  }
}

bool SimpleMediaSession::sendAudio(const MediaSample& mediaSample)
{
  if (m_pAudioManager)
  {
    m_pAudioManager->send(mediaSample);
    return true;
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for audio in simple media session";
    return false;
  }
}

bool SimpleMediaSession::sendAudio(const std::vector<MediaSample>& vMediaSamples)
{
  if (m_pAudioManager)
  {
    m_pAudioManager->send(vMediaSamples);
    return true;
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for audio in simple media session";
    return false;
  }
}

bool SimpleMediaSession::isAudioAvailable() const
{
  if (m_pAudioManager)
  {
    return m_pAudioManager->isSampleAvailable();
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for audio in simple media session";
    return false;
  }
}

std::vector<MediaSample> SimpleMediaSession::getAudioSample()
{
  if (m_pAudioManager)
  {
    return m_pAudioManager->getNextSample();
  }
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager configured for audio in simple media session";
    return std::vector<MediaSample>();
  }
}

boost::system::error_code SimpleMediaSession::doStart()
{
  if (m_pVideoManager)
  {
    boost::system::error_code ec = m_pVideoManager->start();
    if (ec) return ec;
  }
  if (m_pAudioManager)
  {
    boost::system::error_code ec = m_pAudioManager->start();
    if (ec) return ec;
  }
  return boost::system::error_code();
}

boost::system::error_code SimpleMediaSession::doStop()
{
  if (m_pVideoManager)
  {
    // ignore errors on stop
    m_pVideoManager->stop();
  }
  if (m_pAudioManager)
  {
    // ignore errors on stop
    m_pAudioManager->stop();
  }
  return boost::system::error_code();
}

SimpleMediaSession::SimpleSourceAdapter::SimpleSourceAdapter()
  :m_pRtpSessionManager(NULL)
{

}

void SimpleMediaSession::SimpleSourceAdapter::setRtpSessionManager(IRtpSessionManager* pRtpSessionManager)
{
  m_pRtpSessionManager = pRtpSessionManager;
  m_pRtpSessionManager->registerObserver(boost::bind(&SimpleSourceAdapter::notify, this));
}

bool SimpleMediaSession::SimpleSourceAdapter::isSampleAvailable() const
{
  if (m_pRtpSessionManager)
    return m_pRtpSessionManager->isSampleAvailable();
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager is configured!";
    return false;
  }
}

std::vector<MediaSample> SimpleMediaSession::SimpleSourceAdapter::getNextSample()
{
  if (m_pRtpSessionManager)
    return m_pRtpSessionManager->getNextSample();
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager is configured!";
    return std::vector<MediaSample>();
  }
}

void SimpleMediaSession::SimpleSourceAdapter::notify()
{
  // forwarder for observers
  triggerNotification();
}

} // rtp_plus_plus
