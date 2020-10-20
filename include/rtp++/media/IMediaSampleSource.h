#pragma once
#include <vector>
#include <boost/function.hpp>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * @brief The IMediaSampleSource class provides a pull interface and a push interface
 * for media sample retrieval. The pull interface should be used when a program has its
 * own event loop. In this case isSampleAvailable and getNextSample can be used to retrieve
 * available media samples. To use the push mode, the media callback should be registered
 * which should be triggered when media becomes available.
 */
class IMediaSampleSource
{
public:
  typedef boost::function<void ()> Notifier_t;
  typedef boost::function<void (const std::vector<MediaSample>&)> MediaCallback_t;

  /**
   * @brief Destructor
   */
  virtual ~IMediaSampleSource()
  {

  }
  /**
   * @brief Registers observer for new media samples. Observer must then still retrieve samples from source
   */
  void registerObserver(Notifier_t notifier) { m_notifier = notifier; }
  /**
   * @brief Triggers notification callback.
   */
  void triggerNotification()
  {
    if (m_notifier) m_notifier();
  }
  /**
   * @brief Registers media callback: callback is executed when new media samples are available.
   */
  void registerMediaCallback(MediaCallback_t mediaCallback) { m_mediaCallback = mediaCallback; }
  /**
   * @brief isSampleAvailable Pull interface. This method can be called to see if there are any media samples available
   * @return if media samples are available
   */
  virtual bool isSampleAvailable() const = 0;
  /**
   * @brief isSampleAvailable Pull interface. This method can be called to see if there are any media samples available
   * for a specific MID
   * @return if media samples are available
   */
  virtual bool isSampleAvailable(const std::string& sMid) const = 0;
  /**
   * @brief getNextSample Pull interface. This method retrieves the next media sample in the queue
   * @return The next available samples or an empty vector
   */
  virtual std::vector<MediaSample> getNextSample() = 0;
  /**
   * @brief getNextSample Pull interface. This method retrieves the next media sample in the queue
   * @return The next available samples or an empty vector
   */
  virtual std::vector<MediaSample> getNextSample(const std::string& sMid) = 0;

protected:
  /// part of push interface
  MediaCallback_t m_mediaCallback;
  /// For event notification
  Notifier_t m_notifier;
};

} // media
} // rtp_plus_plus

