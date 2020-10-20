#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/experimental/Scream.h>
#include <rtp++/scheduling/RtpScheduler.h>
#include <rtp++/TransmissionManager.h>

// fwd
class ScreamTx;
class RtpQueue;

namespace rtp_plus_plus
{

static float SCREAM_FIFTY_KBPS  = 50000.0f;
static float SCREAM_FIVE_MBPS   = 5000000.0f;
static float SCREAM_30_FPS      = 30.0f;

#ifdef ENABLE_SCREAM
/**
 * @brief The ScreamScheduler class
 */
class ScreamScheduler : public RtpScheduler
{
public:
  /**
   * @brief ScreamScheduler
   * @param pRtpSession
   * @param transmissionManager
   * @param pCooperative
   * @param ioService
   * @param fFps
   * @param fMinBps
   * @param fMaxBps
   */
  ScreamScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                  ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                  float fFps = SCREAM_30_FPS, float fMinBps = SCREAM_FIFTY_KBPS, float fMaxBps = SCREAM_FIVE_MBPS);
  /**
   * @brief Destructor
   */
  ~ScreamScheduler();
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
   * @brief shutdown
   */
  virtual void shutdown();
  /**
   * @brief onScheduleTimeout handler for timer timeouts
   * @param ec
   */
  void onScheduleTimeout(const boost::system::error_code& ec );
  /**
   * @brief processFeedback
   * @param fb
   * @param ep
   */
  virtual void processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep);
  /**
   * @brief retrieveFeedback
   * @return
   */
  virtual std::vector<rfc4585::RtcpFb::ptr> retrieveFeedback();

private:
  /**
   * @brief updateScreamParameters
   * @param uiSSRC
   * @param uiJiffy
   * @param uiHighestSNReceived
   * @param uiCumuLoss
   */
  void updateScreamParameters(uint32_t uiSSRC, uint32_t uiJiffy, uint32_t uiHighestSNReceived, uint32_t uiCumuLoss);

private:
  TransmissionManager& m_transmissionManager;
  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer;

  std::deque<RtpPacket> m_qRtpPackets;
  std::map<uint16_t, RtpPacket> m_mRtpPackets;

  boost::posix_time::ptime m_tStart;
  uint32_t m_uiSsrc;
  RtpQueue* m_pRtpQueue;
  ScreamTx* m_pScreamTx;

  ICooperativeCodec* m_pCooperative;
  uint32_t m_uiPreviousKbps;

  /// SN in last scream report
  uint32_t m_uiLastExtendedSNReported;
  /// loss
  uint32_t m_uiLastLoss;
};

#endif
} // rtp_plus_plus
