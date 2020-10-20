#pragma once
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/RtpSessionState.h>
#include <rtp++/RtpSessionStatistics.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc3550/Rfc3550RtcpTransmissionTimer.h>
#include <rtp++/rfc3550/SessionDatabase.h>

/// @def DEBUG_RTT Log debug info about RTTs
// #define DEBUG_RTT

/// @def DEBUG_RTCP_SCHEDULING Logs debugging information abour report scheduling
// #define DEBUG_RTCP_SCHEDULING

/// @def DEBUG_RTP_STATISTICS Print out RTP statistics objects
// #define DEBUG_RTP_STATISTICS

/**
 * RTCP report manager abstraction: the report manager is responsible for
 * the generation of RTCP reports according to the reporting interval.
 * The report manager has to interact with the session database for this
 * purpose.
 * One report manager is responsible for one flow.
 *
 * TODO:

- The own SSRC is timing out
- With MPRTCP there's an issue when sending the BYE on different flows: the criteria if was a sender in the last session can evaluate to false since both
  managers are using the same session db, and the call to gatherRTCPReportDataAndBeginNewInterval resets the member state. Bye reconsideration might change this
  if packets are still being sent. Perhaps we can multiply the number of intervals ( sender = if sent in last 2 intervals) with the number of flows?
 */

namespace rtp_plus_plus
{

/// Callback functor for when RTCP reports are generated
typedef boost::function<void (const CompoundRtcpPacket& compoundRtcp)> fnRtcpCb_t;

namespace rfc3550
{

class RtcpReportManager : public boost::noncopyable
{
public:
  typedef std::unique_ptr<RtcpReportManager> ptr;

  static ptr create(boost::asio::io_service& ioService,
                    RtpSessionState& rtpSessionState,
                    const RtpSessionParameters &rtpParameters,
                    RtpReferenceClock& referenceClock,
                    SessionDatabase::ptr& pSessionDatabase, const RtpSessionStatistics& rtpSessionStatistics,
                    RtcpTransmissionTimerBase::ptr pTransmissionTimer = RtcpTransmissionTimerBase::ptr
      (new rfc3550::Rfc3550RtcpTransmissionTimer(false)));

  RtcpReportManager(boost::asio::io_service& ioService,
                    RtpSessionState& rtpSessionState,
                    const RtpSessionParameters &rtpParameters,
                    RtpReferenceClock& referenceClock,
                    std::unique_ptr<SessionDatabase>& pSessionDatabase,
                    const RtpSessionStatistics& rtpSessionStatistics,
                    std::unique_ptr<RtcpTransmissionTimerBase> pTransmissionTimer =
      std::unique_ptr<RtcpTransmissionTimerBase>(new rfc3550::Rfc3550RtcpTransmissionTimer(false)));

  virtual ~RtcpReportManager();

  /**
   * configure handler for when RTCP is generated.
   * The calling class MUST set the handler since the asynchronous
   * nature of the implementation relies on feedback notification
   * that packets have been sent.
   */
  void setRtcpHandler(fnRtcpCb_t rtcpHandler);
  /**
   * Starts the RTCP interval calculation
   */
  void startReporting();
  /**
   * This method should be called to shutdown the component cleanly.
   * This triggers the sending of an RTCP BYE. Once a BYE has been
   * sent, the RtpSession can shutdown.
   */
  void scheduleFinalReportAndShutdown();

  virtual void scheduleFeedbackReport(RtcpPacketBase::ptr pFeedbackReport)
  {
    LOG_FIRST_N(WARNING, 1) << "RFC3550: scheduleFeedbackReport not applicable";
  }

  virtual void scheduleFeedbackReports(const std::vector<RtcpPacketBase::ptr>& vReports)
  {
    LOG_FIRST_N(WARNING, 1) << "RFC3550: scheduleFeedbackReports not applicable";
  }

  virtual bool scheduleEarlyFeedbackIfPossible(boost::posix_time::ptime &tScheduled, uint32_t& uiMs)
  {
    LOG_FIRST_N(WARNING, 1) << "RFC3550: scheduleEarlyFeedbackIfPossible not applicable";
    return false;
  }

protected:
  /**
   * Triggers the next RTCP timeout
   */
  virtual void setNextRtcpTimeout();
  /**
   * This method can be overridden to allow subclasses to update state
   */
  virtual void onTimerSet()
  {

  }
  /**
   * Timeout handler
   */
  virtual void onRtcpIntervalTimeout(const boost::system::error_code& ec);
  /**
   * Timeout handler for Bye reconsideration
   */
  void onScheduleByeTimeout(const boost::system::error_code& ec, const CompoundRtcpPacket& bye);
  /**
   * creates a compound RTCP report
   * This code should generate a compound RTCP packet containing an SR if
   * the participant sent media during the last interval, otherwise an RR.
   * Receiver report blocks should also be appended to the packet
   * With a maximum value of 31 per RR. If there are more than 31 active
   * senders, this component should send RRs for the next x senders using
   * a round robin scheduling algorithm.
   */
  CompoundRtcpPacket generatePeriodicCompoundRtcpPacket(const RtcpReportData_t& reportData, bool bMinimalRtcpPacket);
  /**
   * generates RTCP sender report
   */
  std::vector<RtcpSr::ptr> generateSenderReports(const ChannelStatistics& rtpStatistic);

  void scheduleBye(double dTransmissionInterval, const CompoundRtcpPacket& bye);
  /**
   * Helper method that generates a periodic compound RTCP packet
   * @param bWithBye if true, an RTCP BYE report is appended to the compound RTCP packet
   */
  CompoundRtcpPacket generateCompoundRtcpPacket(bool bWithBye = false, bool bMinimalRtcpPacket = false);
  /**
   * This method allows subclasses to add additional RTCP reports to the compound packet
   * @param reportData containts the report data retrieved from the session database
   * @param compoundPacket The compound RTCP packets assembled so far. These might be useful to determine what additional data to add.
   * @return a compound packet containing the additional reports.
   */
  virtual CompoundRtcpPacket createAdditionalReports(const RtcpReportData_t &reportData, const CompoundRtcpPacket& compoundPacket);
  /**
   * This method finalises the RTP session and sends the bye
   * using reconsideration if necessary
   */
  virtual void finaliseSession();
  /**
   * Helper method that builds a compound RTCP packet and sends it using the configured RTCP handler
   */
  void sendRtcpPacket(bool bIsMinimalPacketAllowed = false);
  /**
   * Helper method that determines if the local participant has sent any retransmissions
   * @param rtpStatistic The ChannelStatistics object needed to determine whether any retransmissions were sent
   */
  bool isRetransmissionSender(const ChannelStatistics &rtpStatistic) const;

protected:
  /// asio service
  boost::asio::io_service& m_ioService;
  /// RTP Info manager
  RtpSessionState& m_rtpSessionState;
  /// rtcp timer
  boost::asio::deadline_timer m_rtcpTimer;
  /// Session parameters
  const RtpSessionParameters& m_parameters;
  // RTP reference clock
  RtpReferenceClock& m_referenceClock;
  /// session database
  std::unique_ptr<SessionDatabase>& m_pSessionDatabase;
  /// Timer for RTCP reporting
  std::unique_ptr<RtcpTransmissionTimerBase> m_pTransmissionTimer;
  /// Shutdown flag
  bool m_bShuttingDown;
  /// Callback for RTCP reports
  fnRtcpCb_t m_onRtcp;
  /// Stores whether the first RTCP packet has been sent
  bool m_bInitial;
  /// fraction of bandwidth used by RTCP
  uint32_t m_uiRtcpBandwidthFraction;
  /// RTCP bandwidth
  double m_dRtcpBw;
  // configuration whether reduced min is allowed
  bool m_bUseReducedMinimum;
  /// RTP and RTCP statistics
  const RtpSessionStatistics& m_rtpSessionStatistics;
  /// Store the time the last interval was calculated
  boost::posix_time::ptime m_tPrevious;
  /// Store the time the next calculated interval
  boost::posix_time::ptime m_tNext;
  /// Members at the time where m_tPrevious was calculated
  uint32_t m_uiPreviousMembers;
  /// Stores RTCP interval
  double m_dTransmissionInterval;
  uint32_t m_uiTransmissionIntervalMs;
};

} // rfc3550
} // rtp_plus_plus
