#pragma once
#include <deque>
#include <map>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/scheduling/RtpScheduler.h>
#include <rtp++/TransmissionManager.h>

namespace rtp_plus_plus
{

/**
 * @brief The AckBasedRtpScheduler class
 */
class AckBasedRtpScheduler : public RtpScheduler
{
public:
  /**
   * @brief AckBasedRtpScheduler
   * @param pRtpSession
   */
  AckBasedRtpScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                       ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                       double dFps, uint32_t uiMinKbps, uint32_t uiMaxKps, const std::string& sSchedulingParameter);
  /**
   * @brief ~AckBasedRtpScheduler destructor
   */
  ~AckBasedRtpScheduler();
  /**
   * @brief scheduleRtpPackets paces the sending of packets if there is more than one RTP packet.
   * @param vRtpPackets
   */
  virtual void scheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets);
  /**
   * @brief scheduleRtxPacket
   * @param rtpPacket
   */
  virtual void scheduleRtxPacket(const RtpPacket& rtpPacket);
  /**
   * @brief processFeedback
   * @param fb
   * @param ep
   */
  void processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep);
  /**
   * @brief retrieveFeedback
   * @return
   */
  virtual std::vector<rfc4585::RtcpFb::ptr> retrieveFeedback();
  /**
   * @brief shutdown
   */
  virtual void shutdown();

protected:

  /**
   * @brief ack
   * @param acks
   */
  void ack(const std::vector<uint16_t>& acks);
  /**
   * @brief nack
   * @param nacks
   */
  void nack(const std::vector<uint16_t>& nacks);
  /**
   * @brief doScheduleRtpPackets
   * @param vRtpPackets
   *
   * This code assumes that the calling code and this code are using the same IO service
   * Hence no locking of deque
   */
  virtual void doScheduleRtpPackets(const std::vector<RtpPacket>& vRtpPackets);
  /**
   * @brief sendPacketsIfPossible
   */
  void sendPacketsIfPossible();
  /**
   * @brief onScheduleTimeout handler for timer timeout
   * @param ec
   */
  void onScheduleTimeout(const boost::system::error_code& ec );
  /**
   * @brief onScheduleTimeout handler for timer timeout
   * @param ec
   */
  void onCcbEvalTimeout(const boost::system::error_code& ec);

private:
  TransmissionManager& m_transmissionManager;
  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer; // for pacing

  ICooperativeCodec* m_pCooperative;

  double m_dFps;
  uint32_t m_uiMinKbps;
  uint32_t m_uiMaxKps;
  bool m_bRembEnabled;

  // outgoing packet queue
  std::deque<RtpPacket> m_qRtpPackets;
  boost::posix_time::ptime m_tLastTx;
  uint32_t m_uiUnsentBytes;
  uint32_t m_uiBytesUnacked;
  uint32_t m_uiBytesNewlyAcked;
  uint32_t m_uiHighestSNAcked;
  uint32_t m_uiAckCount;

  // used to measure incoming/outgoing rate of CCB
  uint32_t m_uiMinIntervalBetweenMeasurementsMs;
  boost::asio::deadline_timer m_timerCcb;
  boost::posix_time::ptime m_tLastCcbMeasurement;
  uint32_t m_uiBytesIncomingSinceLastMeasurement; 
  uint32_t m_uiBytesOutgoingSinceLastMeasurement;
  uint32_t m_uiRtxBytesOutgoingSinceLastMeasurement;
  double m_dIncomingRateCcb; // might become immediate, medium, start value
  double m_dOutgoingRateCcb; // might become immediate, medium, start value
  double m_dOutgoingRtxRateCcb; // might become immediate, medium, start value
  // boost::posix_time::ptime m_tLastCcbEval;
  
  std::deque<uint16_t> m_qUnacked;
  std::deque<uint16_t> m_qLost;
  enum TransmissionMode
  {
    TM_FAST_START = 0,
    TM_STEADY_STATE = 1
  };
  TransmissionMode m_eMode;

  double owd_target;
  double owd_fraction_avg;
  boost::circular_buffer<double> owd_fraction_hist;
  double owd_trend;
  boost::circular_buffer<double> owd_norm_hist;
  uint32_t mss;
  uint32_t min_cwnd;
  uint32_t cwnd;
  uint32_t cwnd_i;
  uint32_t bytes_newly_acked;
  uint32_t send_wnd;
  uint32_t t_pace_ms;
  boost::circular_buffer<double> age_vec;
  uint32_t target_bitrate;
  uint32_t target_bitrate_i;
  double rate_transmit;
  double rate_acked;
  double s_rtt;
  double beta;

  // SN -> time RTX sent
  std::map<uint16_t, boost::posix_time::ptime> m_mRtxInfo;

  // time last receive rate measurement
  boost::posix_time::ptime m_tPreviousEstimate;
};

} // rtp_plus_plus
