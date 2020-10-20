#include "CorePch.h"
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>
#include <boost/bind.hpp>
#include <rtp++/rfc4585/Rfc4585RtcpTransmissionTimer.h>
#include <rtp++/rfc5506/Rfc5506.h>

#define COMPONENT_LOG_LEVEL 10
// for now only handle point to point
#define POINT_TO_POINT true
#define DEBUG_RTCP_SCHEDULING

namespace rtp_plus_plus
{
namespace rfc4585
{

RtcpReportManager::ptr RtcpReportManager::create(boost::asio::io_service& ioService,
                                                 RtpSessionState& rtpSessionState,
                                                 const RtpSessionParameters &rtpParameters,
                                                 RtpReferenceClock& referenceClock,
                                                 rfc3550::SessionDatabase::ptr& pSessionDatabase,
                                                 const RtpSessionStatistics& rtpSessionStatistics)
{
  return std::unique_ptr<RtcpReportManager>(
        new RtcpReportManager(ioService, rtpSessionState, rtpParameters, referenceClock, pSessionDatabase, rtpSessionStatistics)
        );
}

RtcpReportManager::RtcpReportManager(boost::asio::io_service& ioService,
                                     RtpSessionState& rtpSessionState,
                                     const RtpSessionParameters& rtpParameters,
                                     RtpReferenceClock& referenceClock,
                                     rfc3550::SessionDatabase::ptr& pSessionDatabase,
                                     const RtpSessionStatistics &rtpSessionStatistics)
  :rfc3550::RtcpReportManager(ioService,
                              rtpSessionState,
                              rtpParameters,
                              referenceClock,
                              pSessionDatabase,
                              rtpSessionStatistics,
                              std::unique_ptr<RtcpTransmissionTimerBase>(new rfc4585::Rfc4585TransmissionTimer(false))),
//  m_bPointToPoint(POINT_TO_POINT),
  m_bAllowEarly(true),
  m_bEarlyScheduled(false),
  m_dT_dither_max(0),
  m_dT_rr_interval(5.0),
  m_dl(0.5), // according to RFC4585
  m_uiT_rr_ms(0),
  m_uiMaxFeedbackDelayMs(200), // TODO: make configurable
  m_bSupportsReducedSize(rfc5506::supportsReducedRtcpSize(rtpParameters))
{
  if (m_bSupportsReducedSize)
  {
    VLOG(2) << "Reduced size RTCP supported";
  }
}

bool RtcpReportManager::isFeedbackRelevant(RtcpPacketBase::ptr pFeedbackReport)
{
  // TODO
  return true;
}

bool RtcpReportManager::isFeedbackUseful(RtcpPacketBase::ptr pFeedbackReport, boost::posix_time::ptime& t0)
{
  return (m_tNext - t0).total_milliseconds() < m_uiMaxFeedbackDelayMs;
}

bool RtcpReportManager::scheduleEarlyFeedbackIfPossible(boost::posix_time::ptime& tScheduled, uint32_t &uiMs)
{
  boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::universal_time();

  uint32_t uiIntervalMs = static_cast<uint32_t>(m_dT_dither_max * 1000 + 0.5);
  if (t0 + boost::posix_time::milliseconds(uiIntervalMs) > m_tNext)
  {
#ifdef DEBUG_RTCP_SCHEDULING
      VLOG(COMPONENT_LOG_LEVEL) << "No early FB: Interval: " << uiIntervalMs << "ms m_dT_dither_max:" << m_dT_dither_max << " Next: " << m_tNext;
#endif
    return false;
  }
  else
  {
    if (m_bAllowEarly)
    {
      // cancel current timer and reschedule packet
      uint32_t uiIntervalMs = static_cast<uint32_t>((rfc3550::fRand(0.0, 1.0) * m_dT_dither_max) * 1000 + 0.5);
      // this will result in the handler being called with the operation_aborted error code
      m_rtcpTimer.expires_from_now(boost::posix_time::milliseconds(uiIntervalMs));
      m_tE = m_rtcpTimer.expires_at();
      uiMs = uiIntervalMs;
      tScheduled = m_tE;
#ifdef DEBUG_RTCP_SCHEDULING
      VLOG(COMPONENT_LOG_LEVEL) << "Early feedback allowed. Resetting timer to expire in "
              << uiIntervalMs << "ms m_tNext: " << m_tNext << " m_tE: " << m_tE;
#endif
      m_bAllowEarly = false;
      // set flag to allow us to restore state after early FB has been sent
      m_bEarlyScheduled = true;
      // launch new async task
      m_rtcpTimer.async_wait(boost::bind(&rfc4585::RtcpReportManager::onRtcpIntervalTimeout, this, _1));
      return true;
    }
    else
    {
#ifdef DEBUG_RTCP_SCHEDULING
      VLOG(COMPONENT_LOG_LEVEL) << "Early feedback not allowed";
#endif
      return false;
    }
  }
}

void RtcpReportManager::setNextRtcpTimeout()
{
  if (m_bEarlyScheduled)
  {
    m_bEarlyScheduled = false;
    // if we had scheduled an early one, we have to maintain the state
    boost::posix_time::ptime tPrevNext = m_tNext;
    m_tNext = m_tPrevious + boost::posix_time::milliseconds(2 * m_uiT_rr_ms);
    m_tPrevious = tPrevNext;
    m_rtcpTimer.expires_at(m_tNext);
#ifdef DEBUG_RTCP_SCHEDULING
    VLOG(COMPONENT_LOG_LEVEL) << "Early was scheduled: resetting state after early feedback m_tNext: " << m_tNext << " Previous tNext: " << tPrevNext;
#endif
    m_uiPreviousMembers = m_pSessionDatabase->getActiveMemberCount();
    m_rtcpTimer.async_wait(boost::bind(&rfc4585::RtcpReportManager::onRtcpIntervalTimeout, this, _1));
  }
  else
  {
#ifdef DEBUG_RTCP_SCHEDULING
    VLOG(COMPONENT_LOG_LEVEL) << "No early was scheduled during the last period, next period can allow early again";
#endif
    // call base class method
    rfc3550::RtcpReportManager::setNextRtcpTimeout();
    // if the early RTCP flag is not set meaning that
    // a regular RTCP report was sent, reset the flag 
    // to allow subsequent early RTCP reports
    m_bAllowEarly = true;
  }
}

void RtcpReportManager::onTimerSet()
{
  // This method is called by the base class every time
  // the interval is calculated: update local variables 
  // that depend on the interval
  double dT_rr = m_dTransmissionInterval;
  // store local copy of this value
  m_uiT_rr_ms = m_uiTransmissionIntervalMs;
  m_dT_dither_max = m_dl * dT_rr;
#ifdef DEBUG_RTCP_SCHEDULING
  VLOG(COMPONENT_LOG_LEVEL) << "dT_rr: " << dT_rr << " T_rr(ms): " << m_uiT_rr_ms << " m_dT_dither_max: " << m_dT_dither_max;
#endif
}

CompoundRtcpPacket RtcpReportManager::createAdditionalReports(const rfc3550::RtcpReportData_t& reportData, const CompoundRtcpPacket& compoundPacket)
{
  CompoundRtcpPacket rtcpPackets = rfc3550::RtcpReportManager::createAdditionalReports(reportData, compoundPacket);

  // copy feedback packets into compound vector: these have previously been retrieved with the callback
  rtcpPackets.insert(rtcpPackets.end(), m_vFeedback.begin(), m_vFeedback.end());
  m_vFeedback.clear();

  return rtcpPackets;
}

void RtcpReportManager::onRtcpIntervalTimeout(const boost::system::error_code& ec)
{
  if (m_bShuttingDown)
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Shutting down";
    finaliseSession();
  }
  else
  {
    if (!ec)
    {
      // cleanup member DB
      /*
      The participant MUST perform this check at least once per RTCP
      transmission interval.
      */
      m_pSessionDatabase->checkMemberDatabase();

      // get feedback from feedback callback
      // we need this to determine whether FB can be suppressed
      if (m_fnFeedback)
      {
        m_fnFeedback(m_vFeedback);
        VLOG_IF(COMPONENT_LOG_LEVEL, !m_vFeedback.empty()) << "Retrieved " << m_vFeedback.size() << " feedback messages for insertion into RTCP compound packet";
      }
      else
      {
        LOG_FIRST_N(WARNING, 1) << "No feedback callback configured";
      }

      // Do timer reconsideration according to RFC4585
      if (m_dT_rr_interval != 0.0 && !m_t_rr_last.is_not_a_date_time())
      {
        double dT_rr_current_interval = m_dT_rr_interval * rfc3550::fRand(0.5, 1.5);
        uint32_t uiT_rr_current_interval = static_cast<uint32_t>(dT_rr_current_interval * 1000 + 0.5);
        if (m_t_rr_last + boost::posix_time::milliseconds(uiT_rr_current_interval) <= m_tNext)
        {
  #ifdef DEBUG_RTCP_SCHEDULING
          VLOG(COMPONENT_LOG_LEVEL) << "Sending regular RTCP packet #1";
  #endif
          // schedule regular
          sendRtcpPacket();
          // store t_rr_last as in RFC4585
          m_t_rr_last = m_tNext;
        }
        else
        {
          // t_rr_last MUST remain unchanged for both these cases
          if (m_vFeedback.empty())
          {
            // suppress RTCP
  #ifdef DEBUG_RTCP_SCHEDULING
            VLOG(COMPONENT_LOG_LEVEL) << "Suppressing RTCP message";
  #endif
          }
          else
          {
  #ifdef DEBUG_RTCP_SCHEDULING
            VLOG(COMPONENT_LOG_LEVEL) << "Scheduling minimal or regular RTCP with " << m_vFeedback.size() << " feedback messages";
  #endif
            // schedule minimal or regular RTCP
            sendRtcpPacket(m_bSupportsReducedSize);
          }
        }
      }
      else
      {
  #ifdef DEBUG_RTCP_SCHEDULING
        VLOG(COMPONENT_LOG_LEVEL) << "Scheduling regular RTCP #2";
  #endif
        // schedule regular
        sendRtcpPacket();
        // store t_rr_last as in RFC4585
        m_t_rr_last = m_tNext;
      }

      // schedule next RTCP timeout
      setNextRtcpTimeout();
    }
    else
    {
      if (ec != boost::asio::error::operation_aborted)
      {
        DLOG(INFO) << "[" << this << "] Error: " << ec.message();
      }
    }
  }
}

} // rfc4585
} // rtp_plus_plus
