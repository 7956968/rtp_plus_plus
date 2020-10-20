#include "CorePch.h"
#include <rtp++/rto/BasicRtoEstimator.h>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <cpputil/Conversion.h>

/// the maximum number of consecutive losses before the estimator stops trying to estimate the next arrival
#define MAX_CONSECUTIVE_LOSSES 500
#define MIN_SAMPLES_FOR_PREDICTION 30
#define MAX_TIME_BETWEEN_PACKETS_MS 3000
#define MAX_EXPIRED_TIMERS 10

namespace rtp_plus_plus
{
namespace rto
{

BasicRtoEstimator::BasicRtoEstimator(boost::asio::io_service& rIoService)
  :m_rIoService(rIoService),
  m_timer(m_rIoService),
  m_bShuttingdown(false),
  m_bResetting(false),
  m_bTimerActive(false),
  m_uiSNCurrentlyExpected(0),
  m_uiPreviouslyReceivedMaxSN(0),
  m_bFirstTimerScheduled(false),
  m_uiPrevEstimateUs(0),
  m_uiConsecutiveLosses(0)
{

}

BasicRtoEstimator::~BasicRtoEstimator()
{
  double dError = m_pPredictor->getErrorStandardDeviation();
  VLOG(2) << "Prediction error: " << dError;
}

void BasicRtoEstimator::reset()
{
  LOG(WARNING) << "Resetting estimator!";
  if (m_bTimerActive)
  {
    m_bResetting = true;
    // cancel outstanding operations
    m_timer.cancel();
  }

  // for now it should be sufficient to remove all items from the queue
  // since this starts the data collection process
  m_pPredictor->reset();
  m_uiConsecutiveLosses = 0;
  m_uiPreviouslyReceivedMaxSN = 0;
  m_tPrev = boost::date_time::not_a_date_time;
  // this will cause a new timeout to be scheduled once there is sufficient data
  m_bFirstTimerScheduled = false;
}

bool BasicRtoEstimator::isGreaterThanPreviouslyReceivedSN( const uint32_t uiSN) const
{
  uint32_t uiDiff = uiSN - m_uiPreviouslyReceivedMaxSN;
  return (uiDiff < (USHRT_MAX >> 1));
}

bool BasicRtoEstimator::hasMaximumTimeBetweenPacketsBeenExceeded(const boost::posix_time::ptime &tArrival)
{
  if (!m_tPrev.is_not_a_date_time())
  {
    int iDiffMs = static_cast<uint32_t>((tArrival - m_tPrev).total_milliseconds());
    // max to not use too large intervals
    // where there are no packets in estimation
    return (iDiffMs > MAX_TIME_BETWEEN_PACKETS_MS);
  }
  else
  {
    return false;
  }
}

bool BasicRtoEstimator::insertNewPredictionEntryIfPossible(const boost::posix_time::ptime &tArrival, const uint32_t uiSN)
{
  if (!m_tPrev.is_not_a_date_time())
  {
    // only insert if it is "greater"
    if (isGreaterThanPreviouslyReceivedSN(uiSN))
    {
      int diffUs = static_cast<int>((tArrival - m_tPrev).total_microseconds());
      // in the case of SN losses there are gaps in the sequence numbers
      // The large gaps in between time also skew the values
      // we therefore divide by the sequence number difference to get the average
      // and not skew the std deviation values
      uint32_t uiDiffSN = uiSN - m_uiPreviouslyReceivedMaxSN;
#if 1
      int iDiffUs2 = (uiDiffSN > 1) ? diffUs/uiDiffSN : diffUs;
      VLOG(15) << "Interrarrival time: " << iDiffUs2 << "us Diff: " << diffUs << "us Diff SN: "
              << uiDiffSN << "(" << uiSN << "-" << m_uiPreviouslyReceivedMaxSN << ")";
#endif
      if (uiDiffSN > 1)
      {
        // here we also know that the packets with sequence numbers in between were lost
        diffUs /= uiDiffSN; // this will truncate the value but should be negligible

        // report missing SNs
        for (size_t i = 1; i < uiDiffSN; ++i)
        {
          // the timed detection might already have detected this so check if the packet
          // is already lost.
          // This scenario could occur if the predictions are inaccurate and the packet after the loss
          // arrives before the timer for the lost packet has expired
          if (!m_statisticsManager.isPacketAssumedLost(m_uiPreviouslyReceivedMaxSN + i))
          {
            m_statisticsManager.assumePacketLost(tArrival, m_uiPreviouslyReceivedMaxSN + i);
            m_onLost(m_uiPreviouslyReceivedMaxSN + i);
          }
        }
      }
      m_pPredictor->insert(diffUs);
      m_tPrev = tArrival;
      m_uiPreviouslyReceivedMaxSN = uiSN;
      return true;
    }
    else
    {
      // older entry: too complex to insert at right position and calc differences
      // we keep it simple by just calculating difference to last received sample with max SN
      // In the mean time we have received further RTP packets with bigger sequence numbers
      LOG(WARNING) << "Received old RTP packet with SN " << uiSN << ". Current max SN: " << m_uiPreviouslyReceivedMaxSN;
      return false;
    }
  }
  else
  {
    m_tPrev = tArrival;
    m_uiPreviouslyReceivedMaxSN = uiSN;
    LOG(WARNING) << "First RTP packet with SN " << uiSN;
    return false;
  }
}

void BasicRtoEstimator::predictRtoAndScheduleTimer(uint32_t uiNextSN)
{
  if (m_pPredictor->isReady())
  {
    double dNextInterval = m_pPredictor->getLastPrediction();
    uint32_t uiEstimateUs = static_cast<uint32_t>(dNextInterval + 0.5);
    // store the previous estimate so that we can re-use it when estimating
    // the RTO for the next SN in the case that the packet is lost
    m_uiPrevEstimateUs = uiEstimateUs;
    VLOG(15) << "Expecting packet " << uiNextSN << " in " << uiEstimateUs << " us";
    scheduleTimer(uiNextSN, uiEstimateUs);
  }
}

void BasicRtoEstimator::doOnPacketArrival(const boost::posix_time::ptime& tArrival, const uint32_t uiSN)
{
  // state check
  if (m_bShuttingdown || m_bResetting)
  {
    LOG(WARNING) << "Resetting predictor";
    return;
  }

  // Removing for DEBUGGING
#if 0
  if (hasMaximumTimeBetweenPacketsBeenExceeded(tArrival))
  {
    LOG(WARNING) << "Maximum time between packets exceeded - resetting estimator";
    reset();
  }
#endif
  VLOG(10) << "Received RTP packet with SN " << uiSN;

  m_uiConsecutiveLosses = 0;
  // FIXME: the following code uses different members so might just work.
  // even though the packet is received and then checked for los
  bool bFalsePositive = m_statisticsManager.isPacketAssumedLost(uiSN);
  // m_statisticsManager.receivePacket(tArrival, uiSN);
  if (bFalsePositive)
  {
    // notify outer class of late arrival so that we can not send RTX of the RTCP FB has not been sent yet
    if (m_onFalsePositive)
    {
      bool bRemovedOnTime = m_onFalsePositive(uiSN);
      if (bRemovedOnTime)
        m_statisticsManager.falsePositiveRemovedBeforeRetransmission(uiSN);
    }
  }

  if (insertNewPredictionEntryIfPossible(tArrival, uiSN))
  {
    // only schedule first timeout: sunsequent ones will be managed by the timeout handler
    if (!m_bFirstTimerScheduled)
    {
      predictRtoAndScheduleTimer(uiSN + 1);
    }
    else
    {
       // cancel timer: the handler will schedule the next packet
       m_timer.cancel();
    }
  }
  else
  {
    // This is an old received packet
    // In the mean time we have received further RTP packets with bigger sequence numbers
    // In this case we don't cancel the timer since the higher sequence number since these have already been received
  }
}

void BasicRtoEstimator::scheduleTimer(uint32_t uiNextSN, uint32_t uiOffsetUs)
{
  if (!m_bShuttingdown)
  {
    if (!m_bFirstTimerScheduled)
    {
      m_bFirstTimerScheduled = true;
    }
    VLOG(10) << "Starting timeout for " << uiNextSN << " expected in " << uiOffsetUs << "us";
    m_uiSNCurrentlyExpected = uiNextSN;
    // explicitly calculate the time from the previous sample to not introduce drift in the prediction
    boost::posix_time::ptime tExpiry = m_tPrev + boost::posix_time::microseconds(uiOffsetUs);
    m_timer.expires_at(tExpiry);
    m_timer.async_wait(boost::bind(&BasicRtoEstimator::onEstimationTimeout, this, _1, uiNextSN) );
    m_bTimerActive = true;
    m_statisticsManager.estimatePacketArrivalTime(m_timer.expires_at(), uiNextSN);

    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    VLOG(15) << LOG_MODIFY_WITH_CARE
            << " PS packet estimation"
            << " now: " << tNow
            << " ID: " << m_id
            << " SN: " << uiNextSN
            << " at: " << m_timer.expires_at() << " (in " << static_cast<uint32_t>(uiOffsetUs/1000.0 + 0.5) << " ms)";

  }
}

void BasicRtoEstimator::scheduleTimerFromNow(uint32_t uiNextSN, uint32_t uiOffsetUs)
{
  if (!m_bShuttingdown)
  {
    if (!m_bFirstTimerScheduled)
    {
      m_bFirstTimerScheduled = true;
    }
    VLOG(10) << "Starting timeout for " << uiNextSN << " expected in " << uiOffsetUs << "us";
    m_uiSNCurrentlyExpected = uiNextSN;
    m_timer.expires_from_now(boost::posix_time::microseconds(uiOffsetUs));
    m_timer.async_wait(boost::bind(&BasicRtoEstimator::onEstimationTimeout, this, _1, uiNextSN) );
    m_bTimerActive = true;
    m_statisticsManager.estimatePacketArrivalTime(m_timer.expires_at(), uiNextSN);

    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    VLOG(15) << LOG_MODIFY_WITH_CARE
            << " PS packet estimation"
            << " now: " << tNow
            << " ID: " << m_id
            << " SN: " << uiNextSN
            << " at: " << m_timer.expires_at() << " (in " << static_cast<uint32_t>(uiOffsetUs/1000.0 + 0.5) << " ms)";
  }
}

void BasicRtoEstimator::doStop()
{
  // set flag so that cancel doesn't respawn next timer
  m_bShuttingdown = true;
  m_timer.cancel();
}

void BasicRtoEstimator::onEstimationTimeout(const boost::system::error_code& ec, uint32_t uiSN)
{
  VLOG(10) << "onRtpPacketAssumedLostTimeout SN: " << uiSN <<  " ec: " << ec.message();
  m_bTimerActive = false;

  if (!m_bShuttingdown && !m_bResetting)
  {
    if (!ec)
    {
      boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#ifdef DEBUG_TIMEOUT_LATENCY
      VLOG(2) << "!!!### Difference: " << (tNow - m_timer.expires_at()).total_microseconds() << "us";
#endif
      // Lost timeout: not cancelled: there could still have been a cancel attempt which might have failed
      if (!m_statisticsManager.hasPacketBeenReceivedRecently(uiSN))
      {
        ++m_uiConsecutiveLosses;
        VLOG(10) << "Packet with SN " << uiSN << " assumed lost";
        m_statisticsManager.assumePacketLost(tNow, uiSN);
        // packet was not received before timeout
        if (m_onLost) m_onLost(uiSN);

        if (m_uiConsecutiveLosses >= MAX_CONSECUTIVE_LOSSES)
        {
          LOG(WARNING) << "Max number (" << MAX_CONSECUTIVE_LOSSES << ") of consecutive losses reached. No further RTOs will be set.";
          // clear queue to reset getInterarrivalTimeEstimateUs() state
          reset();
          return;
        }

        // check if the lost SN is is greater than the max received SN
        if (uiSN - m_uiPreviouslyReceivedMaxSN < MAX_CONSECUTIVE_LOSSES)
        {
          VLOG(10) << "Consecutive loss? Scheduling timer for loss + 1: " << uiSN + 1;
          // schedule timer for next packet in case there are multiple losses
          scheduleTimerFromNow(uiSN + 1, m_uiPrevEstimateUs);
        }
        else
        {
          VLOG(10) << "Loss? Scheduling timer for previous max + 1: " << m_uiPreviouslyReceivedMaxSN + 1;
          // even though SN was lost: a packet with a higher SN has arrived in the interim
          predictRtoAndScheduleTimer(m_uiPreviouslyReceivedMaxSN + 1);
        }
      }
      else
      {
        VLOG(10) << "Packet received. Scheduling timer for previous max + 1: " << m_uiPreviouslyReceivedMaxSN + 1;
        // start estimation of next highest SN
        predictRtoAndScheduleTimer(m_uiPreviouslyReceivedMaxSN + 1);
      }
    }
    else
    {
      // cancelled: reschedule next sample
      VLOG(10) << "Packet received. Scheduling timer for previous max + 1: " << m_uiPreviouslyReceivedMaxSN + 1;
      // start estimation of next highest SN
      predictRtoAndScheduleTimer(m_uiPreviouslyReceivedMaxSN + 1);
    }
  }

  if (m_bResetting)
  {
    VLOG(2) << "RTO estimator reset complete and ready to accept new packets";
    m_bResetting = false;
    m_statisticsManager.reset();
  }
}

} // rto
} // rtp_plus_plus
