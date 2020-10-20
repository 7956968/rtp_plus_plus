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
#include <cpputil/Conversion.h>
#include <cpputil/ServiceThread.h>
#include <rtp++/RtpTime.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/IMediaSampleSource.h>

namespace rtp_plus_plus
{
namespace media
{

typedef boost::function<void (const MediaSample&)> MediaCb_t;
typedef boost::function<void (const std::vector<MediaSample>&)> MediaAccessUnitCb_t;

/**
 * @brief The MediaConsumer class: Service that periodically consumes samples from IMediaSampleSource
 * It shows how media data can be accessed by other frameworks, applications with their own event loop
 */

class MediaConsumer :  public boost::enable_shared_from_this<MediaConsumer>, // for async callbacks
    private boost::noncopyable
{
public:
  typedef boost::shared_ptr<MediaConsumer> ptr;

public:
  static MediaConsumer::ptr create( unsigned uiIntervalMicrosecs, IMediaSampleSource& mediaSource )
  {
    return boost::make_shared<MediaConsumer>(uiIntervalMicrosecs, mediaSource);
  }

  MediaConsumer(unsigned uiIntervalMicrosecs, IMediaSampleSource& mediaSource)
    : m_timer(m_ioService),
      m_uiIntervalMicrosecs(uiIntervalMicrosecs),
      m_rMediaSource(mediaSource)
  {
  }

  ~MediaConsumer()
  {
  }

  void setSampleCallback(MediaCb_t sampleCB) { m_sampleCB = sampleCB; }
  void setAccessUnitCallback(MediaAccessUnitCb_t accessUnitCB) { m_accessUnitCB = accessUnitCB; }

  boost::system::error_code onStart()
  {
    return boost::system::error_code();
  }

  boost::system::error_code start()
  {
    scheduleNextSample();
    m_ioService.run();
    return boost::system::error_code();
  }

  boost::system::error_code stop()
  {
    m_ioService.post(boost::bind(&MediaConsumer::handleStop, this));
    return boost::system::error_code();
  }

  boost::system::error_code onComplete()
  {
    return boost::system::error_code();
  }

private:

  void scheduleNextSample()
  {
    m_timer.expires_from_now(boost::posix_time::microseconds(m_uiIntervalMicrosecs) );
    m_timer.async_wait(boost::bind(&MediaConsumer::handleTimeout, this, boost::asio::placeholders::error));
  }

  void handleStop()
  {
    m_timer.cancel();
  }

  // asio event handler
  void handleTimeout(const boost::system::error_code& ec)
  {
    if (!ec)
    {
      uint32_t uiSamples = 0;
      std::vector<MediaSample> vSamples = m_rMediaSource.getNextSample();

      if (m_sampleCB)
      {
        for (MediaSample& mediaSample: vSamples)
        {
          VLOG(10) << "Got sample with TS: " << mediaSample.getStartTime();
          m_sampleCB(mediaSample);
          ++uiSamples;
        }
      }
      else if (m_accessUnitCB)
      {
        if (!vSamples.empty())
        {
          m_accessUnitCB(vSamples);
          uiSamples += vSamples.size();
        }
      }
      else
      {
        LOG_FIRST_N(WARNING, 1) << "No callback configured for consumer";
      }
//      while (pSample)
//      {
//        VLOG(10) << "Got sample with TS: " << pSample->getStartTime();
//        if (m_sampleCB)  m_sampleCB(*pSample);
//        ++uiSamples;
//        pSample = m_rMediaSource.getNextSample();
//      }
      if (uiSamples > 0)
      {
        VLOG(8) << "Got " << uiSamples << " from media session manager";
      }
      else
      {
        VLOG(15) << "Got 0 from media session manager";
      }
      scheduleNextSample();
    }
    else
    {
      // log error
      if (ec != boost::asio::error::operation_aborted)
      {
        LOG(WARNING) << "Timed out error: %1%" << ec.message();
      }
      else
      {
        VLOG(10) << "Timer cancelled, shutdown imminent?";
      }
    }
  }

private:
  boost::asio::io_service m_ioService;
  boost::asio::deadline_timer m_timer;

  uint32_t m_uiIntervalMicrosecs;
  IMediaSampleSource& m_rMediaSource;

  MediaCb_t m_sampleCB;
  MediaAccessUnitCb_t m_accessUnitCB;
};

}
}

template <>
struct ServiceTraits<rtp_plus_plus::media::MediaConsumer>
{
  static const char* name;
};

template <>
struct ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::MediaConsumer> >
{
  static const char* name;
};

typedef ServiceThread<rtp_plus_plus::media::MediaConsumer> MediaConsumerService;
typedef boost::shared_ptr<ServiceThread< boost::shared_ptr<rtp_plus_plus::media::MediaConsumer> > > MediaConsumerServicePtr;
