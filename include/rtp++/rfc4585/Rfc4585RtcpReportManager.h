#pragma once
#include <rtp++/rfc3550/RtcpReportManager.h>
#include <boost/date_time/posix_time/ptime.hpp>

// for now only handle point to point
#define POINT_TO_POINT true

/// @def DEBUG_RTCP_SCHEDULING logs debugging information abour report scheduling
// #define DEBUG_RTCP_SCHEDULING

namespace rtp_plus_plus
{
namespace rfc4585
{

typedef boost::function<void (CompoundRtcpPacket&) > FeedbackCallback_t;

class RtcpReportManager : public rfc3550::RtcpReportManager
{
public:
  typedef std::unique_ptr<RtcpReportManager> ptr;

  static ptr create(boost::asio::io_service& ioService,
                    RtpSessionState& rtpSessionState,
                    const RtpSessionParameters &rtpParameters,
                    RtpReferenceClock &referenceClock,
                    rfc3550::SessionDatabase::ptr& pSessionDatabase,
                    const RtpSessionStatistics& rtpSessionStatistics);

  RtcpReportManager(boost::asio::io_service& ioService,
                    RtpSessionState& rtpSessionState,
                    const RtpSessionParameters& rtpParameters,
                    RtpReferenceClock &referenceClock,
                    rfc3550::SessionDatabase::ptr& pSessionDatabase,
                    const RtpSessionStatistics& rtpSessionStatistics);

  void setFeedbackCallback(FeedbackCallback_t fnFeedback)
  {
    m_fnFeedback = fnFeedback;
  }

  // Note: T_rr_interval is in seconds
  double getT_rr_interval() const { return m_dT_rr_interval; }
  void setT_rr_interval(double dT_rr_interval) { m_dT_rr_interval = dT_rr_interval; }

  virtual bool isFeedbackRelevant(RtcpPacketBase::ptr pFeedbackReport);

  bool isFeedbackUseful(RtcpPacketBase::ptr pFeedbackReport, boost::posix_time::ptime& t0);
  /**
   * @brief scheduleEarlyFeedbackIfPossible can be called to schedule feedback if possible provided that
   * the caller has already verfied that the feebback is useful. This is the preferred
   * method to be used since it allows the feedback agent to collect and group data
   * until the time arrives where the feedback report is to be sent.
   * @param tScheduled
   * @param uiMs
   * @return
   */
  virtual bool scheduleEarlyFeedbackIfPossible(boost::posix_time::ptime &tScheduled, uint32_t& uiMs);

  void setNextRtcpTimeout();

protected:
  virtual void onTimerSet();
  /**
   * @brief createAdditionalReports should be overriden to send feedback packets
   * @param reportData
   * @return
   */
  virtual CompoundRtcpPacket createAdditionalReports(const rfc3550::RtcpReportData_t& reportData, const CompoundRtcpPacket& compoundPacket);
  /**
   * Timeout handler
   */
  virtual void onRtcpIntervalTimeout(const boost::system::error_code& ec);

private:
  // true for point to point sessions
  //    bool m_bPointToPoint;
  // if an early RTCP report is allowed
  bool m_bAllowEarly;
  // if an early RTCP report has been scheduled
  bool m_bEarlyScheduled;
  // time the last RR was sent
  boost::posix_time::ptime m_t_rr_last;
  // time the early RTCP report is to be sent
  boost::posix_time::ptime m_tE;

  // vector to store feedback messages to be sent
  CompoundRtcpPacket m_vFeedback;
  // callback to retrieve feedback messages
  FeedbackCallback_t m_fnFeedback;

  /// From RFC4585: for randomisation of early RTCP scheduling
  double m_dT_dither_max;
  /// From RFC4585: for supressing regular reports
  double m_dT_rr_interval;
  /// From RFC4585
  double m_dl;
  /// the last calculated RR interval in MS
  uint32_t m_uiT_rr_ms;
  /// Delay in milliseconds for which the feedback delay is useful
  uint32_t m_uiMaxFeedbackDelayMs;
  // supports reduced size
  bool m_bSupportsReducedSize;
};

} // rfc4585
} // rtp_plus_plus
