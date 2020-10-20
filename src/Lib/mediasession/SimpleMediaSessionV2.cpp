#include "CorePch.h"
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/IRtpSessionManager.h>
#include <rtp++/RtpSessionManager.h>

namespace rtp_plus_plus
{

using media::MediaSample;

SimpleMediaSessionV2::SimpleMediaSessionV2(boost::asio::io_service& ioService)
  :m_rIoService(ioService),
    m_eState(ST_READY),
    m_uiCompleteCount(0)
{
}

SimpleMediaSessionV2::~SimpleMediaSessionV2()
{
}

bool SimpleMediaSessionV2::hasVideo() const
{
  return m_pVideoManager != 0;
}

bool SimpleMediaSessionV2::hasAudio() const
{
  return m_pAudioManager != 0;
}

void SimpleMediaSessionV2::setVideoSessionManager(std::unique_ptr<IRtpSessionManager> pVideoManager)
{
  m_pVideoManager = std::move(pVideoManager);
  m_videoSource.setRtpSessionManager(m_pVideoManager.get());
  m_pVideoManager->setRtpSessionCompleteNotifier(boost::bind(&SimpleMediaSessionV2::handleRtpSessionComplete, this));
}

void SimpleMediaSessionV2::setAudioSessionManager(std::unique_ptr<IRtpSessionManager> pAudioManager)
{
  m_pAudioManager = std::move(pAudioManager);
  m_audioSource.setRtpSessionManager(m_pAudioManager.get());
  m_pAudioManager->setRtpSessionCompleteNotifier(boost::bind(&SimpleMediaSessionV2::handleRtpSessionComplete, this));
}

bool SimpleMediaSessionV2::sendVideo(const MediaSample& mediaSample)
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

bool SimpleMediaSessionV2::sendVideo(const std::vector<MediaSample>& vMediaSamples)
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

bool SimpleMediaSessionV2::isVideoAvailable() const
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

std::vector<MediaSample> SimpleMediaSessionV2::getVideoSample()
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

bool SimpleMediaSessionV2::sendAudio(const MediaSample& mediaSample)
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

bool SimpleMediaSessionV2::sendAudio(const std::vector<MediaSample>& vMediaSamples)
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

bool SimpleMediaSessionV2::isAudioAvailable() const
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

std::vector<MediaSample> SimpleMediaSessionV2::getAudioSample()
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

boost::system::error_code SimpleMediaSessionV2::start()
{
  if (m_pVideoManager)
  {
    boost::system::error_code ec = m_pVideoManager->start();
    if (ec)
    {
#if 0
      VLOG(2) << "HACK: ignoring failed media sessions for now in case of answer/offer rejections";
#else
      return ec;
#endif
    }
  }
  if (m_pAudioManager)
  {
    boost::system::error_code ec = m_pAudioManager->start();
    if (ec)
    {
#if 0
      VLOG(2) << "HACK: ignoring failed media sessions for now in case of answer/offer rejections";
#else
      return ec;
#endif
    }
  }

  m_eState = ST_STARTED;
  return boost::system::error_code();
}

boost::system::error_code SimpleMediaSessionV2::stop()
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

  m_eState = ST_READY;
  return boost::system::error_code();
}

SimpleMediaSessionV2::SimpleSourceAdapter::SimpleSourceAdapter()
  :m_pRtpSessionManager(NULL)
{

}

void SimpleMediaSessionV2::SimpleSourceAdapter::setRtpSessionManager(IRtpSessionManager* pRtpSessionManager)
{
  m_pRtpSessionManager = pRtpSessionManager;
  m_pRtpSessionManager->registerObserver(boost::bind(&SimpleSourceAdapter::notify, this));
}

bool SimpleMediaSessionV2::SimpleSourceAdapter::isSampleAvailable() const
{
  if (m_pRtpSessionManager)
    return m_pRtpSessionManager->isSampleAvailable();
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager is configured!";
    return false;
  }
}

std::vector<MediaSample> SimpleMediaSessionV2::SimpleSourceAdapter::getNextSample()
{
  if (m_pRtpSessionManager)
    return m_pRtpSessionManager->getNextSample();
  else
  {
    LOG_FIRST_N(WARNING, 1) << "No RTP session manager is configured!";
    return std::vector<MediaSample>();
  }
}

void SimpleMediaSessionV2::SimpleSourceAdapter::notify()
{
  // forwarder for observers
  triggerNotification();
}

void SimpleMediaSessionV2::handleRtpSessionComplete()
{
  ++m_uiCompleteCount;
  if (m_pVideoManager && m_pAudioManager)
  {
    if (m_uiCompleteCount == 2)
    {
      VLOG(2) << "All RTP sessions complete.";
      // HACK to access session stats
      boost::posix_time::ptime tFirstSyncedVideoArrival;
      boost::posix_time::ptime tFirstSyncedVideoPts;
      RtpSessionManager* pRtpSessionManager = dynamic_cast<RtpSessionManager*>(m_pVideoManager.get());
      if (pRtpSessionManager)
      {
        const std::vector<RtpPacketStat>& rtpStats = pRtpSessionManager->getPacketStats();
        if (!rtpStats.empty())
        {
          auto it = std::find_if(rtpStats.begin(), rtpStats.end(), [](const RtpPacketStat& stat)
          {
            return std::get<2>(stat);
          });
          if (it != rtpStats.end())
          {
            const RtpPacketStat& stat = *it;
            tFirstSyncedVideoArrival = std::get<0>(stat);
            tFirstSyncedVideoPts = std::get<1>(stat);;
            boost::posix_time::ptime tPrevArrival = tFirstSyncedVideoArrival;
            boost::posix_time::ptime tPrevPts = tFirstSyncedVideoPts;
            LOG(INFO) << "First Video RTCP synced packet stats: Pts: " << tFirstSyncedVideoPts << " arrival: " << tFirstSyncedVideoArrival;
            ++it;
            std::vector<uint64_t> vDiffs;
            for (; it != rtpStats.end(); ++it)
            {
              const RtpPacketStat& s = *it;
              boost::posix_time::ptime tArrival = std::get<0>(s);
              boost::posix_time::ptime tPts = std::get<1>(s);
              uint64_t uiDiffArrivalMs = (tArrival - tPrevArrival).total_milliseconds();
              uint64_t uiDiffPtsMs = (tPts - tPrevPts).total_milliseconds();
              vDiffs.push_back(uiDiffPtsMs - uiDiffArrivalMs);
              tPrevArrival = tArrival;
              tPrevPts = tPts;
            }
          }
        }
      }

      boost::posix_time::ptime tFirstSyncedAudioArrival;
      boost::posix_time::ptime tFirstSyncedAudioPts;
      RtpSessionManager* pAudioRtpSessionManager = dynamic_cast<RtpSessionManager*>(m_pAudioManager.get());
      if (pAudioRtpSessionManager)
      {
        const std::vector<RtpPacketStat>& rtpStats = pAudioRtpSessionManager->getPacketStats();
        if (!rtpStats.empty())
        {
          auto it = std::find_if(rtpStats.begin(), rtpStats.end(), [](const RtpPacketStat& stat)
          {
            return std::get<2>(stat);
          });
          if (it != rtpStats.end())
          {
            const RtpPacketStat& stat = *it;
            tFirstSyncedAudioArrival = std::get<0>(stat);
            tFirstSyncedAudioPts = std::get<1>(stat);;
            LOG(INFO) << "First Audio RTCP synced packet stats: Pts: " << tFirstSyncedAudioPts << " arrival: " << tFirstSyncedAudioArrival;
          }
        }
      }

      if (!tFirstSyncedAudioPts.is_not_a_date_time() && !tFirstSyncedVideoPts.is_not_a_date_time())
      {
        // calc differences
        LOG(INFO) << "Diff in A/V PTS: " << (tFirstSyncedVideoPts - tFirstSyncedAudioPts).total_milliseconds() << "ms"
                  << " arrival time: " << (tFirstSyncedVideoArrival - tFirstSyncedAudioArrival).total_milliseconds() << "ms";
      }
      if (m_mediaSessionCompleteHandler) m_mediaSessionCompleteHandler();
    }
  }
  else if (m_pVideoManager || m_pAudioManager)
  {
    if (m_uiCompleteCount == 1)
    {
      VLOG(2) << "All RTP sessions complete.";
      if (m_mediaSessionCompleteHandler) m_mediaSessionCompleteHandler();
    }
  }
}

} // rtp_plus_plus
