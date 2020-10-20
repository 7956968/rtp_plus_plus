#pragma once
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <numeric>
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <cpputil/Buffer.h>
#include <cpputil/RunningAverageQueue.h>
#include <cpputil/ServiceThread.h>
#include <rtp++/media/GeneratedMediaSource.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{

typedef boost::function<boost::system::error_code(const MediaSample&)> ReceiveMediaCb_t;

// -DDEBUG_SOURCE_JITTER

/**
 * @brief The MediaGenerator class generates periodic media samples
 */
class MediaGenerator :  public boost::enable_shared_from_this<MediaGenerator>, // for async callbacks
        private boost::noncopyable
{
public:
  typedef boost::shared_ptr<MediaGenerator> ptr;

public:

  static MediaGenerator::ptr create( unsigned uiIntervalMicrosecs, PayloadType ePayloadType,
                                     const std::string& sPacketSizes, uint8_t uiCharPayload,
                                     uint32_t uiTotalVideoSamples = 0,
                                     uint32_t uiDurationMilliSeconds = 0 );

  MediaGenerator(unsigned uiIntervalMicrosecs, PayloadType ePayloadType,
                 const std::string& sPacketSizes, uint8_t uiCharPayload,
                 uint32_t uiTotalVideoSamples, uint32_t uiDurationMilliSeconds);

  ~MediaGenerator();

  void setReceiveMediaCB(ReceiveMediaCb_t val);

  boost::system::error_code onStart();
  boost::system::error_code start();
  boost::system::error_code stop();
  boost::system::error_code onComplete();

private:

  void processNextSample();
  void scheduleNextSample();
  void handleStop();
  // asio event handler
  void handleTimeout(const boost::system::error_code& ec);
  bool shouldStop() const;
private:
  boost::asio::io_service m_ioService;
  //boost::asio::io_service::work m_work;
  std::unique_ptr<boost::asio::io_service::work> m_pWork;

  boost::asio::deadline_timer m_timer;

  GeneratedMediaSource m_source;

  // To be replaced by MediaSourceInterface so that we can use junk data, file data, etc
  ReceiveMediaCb_t m_receiveMediaCB;
  uint32_t m_uiIntervalMicrosecs;

  // for timer precision
  boost::posix_time::ptime m_timeBase; 
  boost::posix_time::ptime m_calculated;
  boost::posix_time::time_duration m_estimated, m_elapsed;

  // state
  unsigned m_uiTotalPacketSent;
  uint64_t m_uiTotalBytes;
  boost::posix_time::ptime m_tLastSample;
  boost::posix_time::ptime m_tSend;
  double m_dLastTime;

  RunningAverageQueue<uint32_t, double> m_jitter;
  uint32_t m_uiMinJitter;
  uint32_t m_uiMaxJitter;
  // Total milliseconds
  uint32_t m_uiDurationMilliSeconds;
};

} // media
} // rtp_plus_plus

template <>
struct ServiceTraits<rtp_plus_plus::media::MediaGenerator>
{
  static const char* name;
};

template <>
struct ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::MediaGenerator> >
{
  static const char* name;
};

typedef ServiceThread<rtp_plus_plus::media::MediaGenerator> MediaGeneratorService;
typedef boost::shared_ptr<ServiceThread< boost::shared_ptr<rtp_plus_plus::media::MediaGenerator> > > MediaGeneratorServicePtr;
