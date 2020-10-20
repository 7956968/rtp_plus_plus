#include "CorePch.h"
#include <rtp++/media/MediaConsumerService.h>
#include <rtp++/media/MediaGeneratorService.h>
#include <rtp++/media/StreamMediaSource.h>
#include <cpputil/Conversion.h>

const char* ServiceTraits<rtp_plus_plus::media::MediaGenerator>::name = "Media generator simulation";
const char* ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::MediaGenerator> >::name = "Media generator simulation";

const char* ServiceTraits<rtp_plus_plus::media::StreamMediaSource>::name = "Stream media source";
const char* ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::StreamMediaSource> >::name = "Stream media source";

const char* ServiceTraits<rtp_plus_plus::media::MediaConsumer>::name = "Media consumer service";
const char* ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::MediaConsumer> >::name = "Media consumer service";

namespace rtp_plus_plus
{
namespace media
{

MediaGenerator::ptr MediaGenerator::create( unsigned uiIntervalMicrosecs, PayloadType ePayloadType,
                                            const std::string& sPacketSizes, uint8_t uiCharPayload,
                                            uint32_t uiTotalVideoSamples, uint32_t uiDurationMilliSeconds )
{
  return boost::make_shared<MediaGenerator>(uiIntervalMicrosecs, ePayloadType, sPacketSizes, uiCharPayload, uiTotalVideoSamples, uiDurationMilliSeconds);
}

MediaGenerator::MediaGenerator(unsigned uiIntervalMicrosecs, PayloadType ePayloadType,
                               const std::string& sPacketSizes, uint8_t uiCharPayload,
                               uint32_t uiTotalVideoSamples, uint32_t uiDurationMilliSeconds)
  :m_pWork(new boost::asio::io_service::work(m_ioService)),
    m_timer(m_ioService),
    m_uiIntervalMicrosecs(uiIntervalMicrosecs),
    m_source(ePayloadType, sPacketSizes, uiCharPayload, uiTotalVideoSamples),
    m_uiTotalPacketSent(0),
    m_uiTotalBytes(0),
    m_dLastTime(0.0),
    m_jitter(250),
    m_uiMinJitter(UINT_MAX),
    m_uiMaxJitter(0),
    m_uiDurationMilliSeconds(uiDurationMilliSeconds)
{

}

MediaGenerator::~MediaGenerator()
{
  // print out statistics
  //unsigned uiTotalBytes = m_uiTotalPacketSent * m_uiPacketSize;
  //unsigned uiTotalBytes = m_uiTotalPacketSent * m_uiPacketSize;
  boost::posix_time::time_duration duration = m_tLastSample - m_timeBase;
  DLOG(INFO) << "[" << this << "] Summary: Total packets sent: " << m_uiTotalPacketSent << " Total bytes: " << m_uiTotalBytes << " Total Duration (ms): " << duration.total_milliseconds();

  // TODO: using variable packet sizes now: can not calculate it like this anymore
  /*
  double dNumberOfSendPerSecond = 1000/static_cast<double>(m_uiIntervalMs);
  /// one kbps = 1000 bits per second (not 1024!!)
  double dCalcRate = (dNumberOfSendPerSecond * m_uiPacketSize * 8)/1000.0;
  double dMeasuredRate = (uiTotalBytes * 8)/static_cast<double>(duration.total_milliseconds());
  double dAccuracyPercentage = ((dCalcRate - dMeasuredRate)/dCalcRate)*100;
  DLOG(INFO) << "Duration: " << duration.total_seconds() << " Calculated bitrate: " << dCalcRate << "kbps Measured bitrate: " << dMeasuredRate << "kbps Accuracy: " << dAccuracyPercentage << "%";
  */
}

void MediaGenerator::setReceiveMediaCB(ReceiveMediaCb_t val) { m_receiveMediaCB = val; }

boost::system::error_code MediaGenerator::onStart()
{
  return boost::system::error_code();
}

boost::system::error_code MediaGenerator::start()
{
  processNextSample();
  scheduleNextSample();
  m_ioService.run();
  return boost::system::error_code();
}

boost::system::error_code MediaGenerator::stop()
{
  m_ioService.post(boost::bind(&MediaGenerator::handleStop, this));
  return boost::system::error_code();
}

boost::system::error_code MediaGenerator::onComplete()
{
  return boost::system::error_code();
}

void MediaGenerator::processNextSample()
{
  if (m_timeBase.is_not_a_date_time())
  {
    m_timeBase = boost::posix_time::microsec_clock::local_time();
    m_tLastSample = m_timeBase;
    m_dLastTime = 0.0;
  }
  else
  {
    // for measuring send delay
    m_tSend = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = m_tSend - m_tLastSample;
    uint64_t uiTotalMicroseconds = diff.total_microseconds();
    uint32_t uiSeconds = (uint32_t)(uiTotalMicroseconds/1000000);
    uint32_t uiMicroseconds = (uint32_t)(uiTotalMicroseconds%1000000);
    double dElapsed = uiSeconds + (uiMicroseconds/1000000.0);
    m_dLastTime += dElapsed;
    m_tLastSample = m_tSend;

    // jitter calculation: casting to uint32_t should be ok here since we are expecting the value to be
    // reasonably close together
    uint32_t uiJitter = static_cast<uint32_t>(std::abs((m_tLastSample - m_calculated).total_microseconds()));

    m_jitter.insert(uiJitter);
    m_uiMinJitter = (uiJitter < m_uiMinJitter) ? uiJitter : m_uiMinJitter;
    m_uiMaxJitter = (uiJitter > m_uiMaxJitter) ? uiJitter : m_uiMaxJitter;

    VLOG(15) << "Elapsed: " << dElapsed << "s (" << uiTotalMicroseconds << " mics) Sample time: " << m_dLastTime
             << " Jitter: " << uiJitter << " Min: " << m_uiMinJitter << " Max: " << m_uiMaxJitter
             << " Run Avg: " << m_jitter.getAverage();
#ifdef DEBUG_SOURCE_JITTER
    LOG_EVERY_N(INFO, 500) << "Elapsed: " << dElapsed << "s (" << uiTotalMicroseconds << " mics) Sample time: " << m_dLastTime
                           << " Jitter: " << uiJitter << " Min: " << m_uiMinJitter << " Max: " << m_uiMaxJitter
                           << " Run Avg: " << m_jitter.getAverage();
#endif
  }

  ++m_uiTotalPacketSent;

  boost::optional<MediaSample> mediaSample = m_source.getNextMediaSample();

  if (mediaSample)
  {
    mediaSample->setStartTime(m_dLastTime);
    if (m_receiveMediaCB)
      m_receiveMediaCB(*mediaSample);

    m_uiTotalBytes += mediaSample->getDataBuffer().getSize();
  }
}

void MediaGenerator::scheduleNextSample()
{
  // calculate how much time has actually elapsed using the time base to maintain timer precision
  m_elapsed = boost::posix_time::microsec_clock::local_time() - m_timeBase;
  m_timer.expires_from_now(m_estimated - m_elapsed +
                           boost::posix_time::microseconds(m_uiIntervalMicrosecs));

  // calculate what the time should be when we get the next sample so that we can calculate the jitter
  m_calculated = m_timeBase + boost::posix_time::microseconds(m_uiTotalPacketSent * m_uiIntervalMicrosecs);

  m_timer.async_wait(boost::bind(&MediaGenerator::handleTimeout, this, boost::asio::placeholders::error));
}

void MediaGenerator::handleStop()
{
  m_timer.cancel();
  m_ioService.stop();
}

bool MediaGenerator::shouldStop() const
{
  if (!m_source.isGood())
    return true;
  else if (m_uiDurationMilliSeconds > 0 &&
           ((m_tLastSample - m_timeBase).total_milliseconds() >= m_uiDurationMilliSeconds)
           )
  {
    VLOG(2) << "Media generator should stop. "
            << "Target duration: " << m_uiDurationMilliSeconds
            << "ms elapsed: " << (m_tLastSample - m_timeBase).total_milliseconds() << "ms";
    return true;
  }
  return false;
}

// asio event handler
void MediaGenerator::handleTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    processNextSample();

    // end condition
    if (shouldStop())
    {
      VLOG(2) << "Stopping media source, max samples sent: " << m_uiTotalPacketSent;
      m_pWork.reset();
      return;
    }
    m_estimated += boost::posix_time::microseconds(m_uiIntervalMicrosecs);
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
      LOG(INFO) << "Timer cancelled, shutdown imminent?";
    }
  }
}

} // media
} // rtp_plus_plus
