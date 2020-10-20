#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <rtp++/experimental/ExperimentalRtcp.h>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/experimental/Nada.h>
#include <rtp++/scheduling/RtpScheduler.h>
#include <rtp++/TransmissionManager.h>

namespace rtp_plus_plus
{

static float NADA_FIFTY_KBPS  = 50000.0f;
static float NADA_FIVE_MBPS   = 5000000.0f;
static float NADA_30_FPS      = 30.0f;

/**
 * @brief The NadaScheduler class
 */
class NadaScheduler : public RtpScheduler
{
public:
  /**
   * @brief NadaScheduler
   * @param pRtpSession
   * @param transmissionManager
   * @param pCooperative
   * @param ioService
   * @param fFps
   * @param fMinBps
   * @param fMaxBps
   */
  NadaScheduler(RtpSession::ptr pRtpSession, TransmissionManager& transmissionManager,
                  ICooperativeCodec* pCooperative, boost::asio::io_service& ioService,
                  float fFps = NADA_30_FPS, float fMinBps = NADA_FIFTY_KBPS, float fMaxBps = NADA_FIVE_MBPS);
  /**
   * @brief Destructor
   */
  ~NadaScheduler();
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
   * @brief processFeedback
   * @param fb
   * @param ep
   */
  void processFeedback(const rfc4585::RtcpFb& fb, const EndPoint& ep);
  /**
   * @brief onIncomingRtp
   * @param rtpPacket
   * @param ep
   * @param bSSRCValidated
   * @param bRtcpSynchronised
   * @param tPresentation
   */
  virtual void onIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                            bool bSSRCValidated, bool bRtcpSynchronised,
                                            const boost::posix_time::ptime& tPresentation);
  /**
   * @brief onIncomingRtcp
   * @param compoundRtcp
   * @param ep
   */
  virtual void onIncomingRtcp(const CompoundRtcpPacket& compoundRtcp,
                                             const EndPoint& ep);
  /**
   * @brief retrieveFeedback
   * @return
   */
  std::vector<rfc4585::RtcpFb::ptr> retrieveFeedback();

private:
  /**
   * @brief updateNadaParameters
   * @param fb
   */
  void updateNadaParameters(const experimental::RtcpNadaFb& fb);
  /**
   * @brief sendPacketsIfPossible
   */
  void sendPacketsIfPossible();
  /**
   * @brief updateOutgoingRateQueue
   */
  void updateOutgoingRateQueue(const boost::posix_time::ptime& tNow);
  /**
   * @brief calculateSendRateKbps
   * @param tNow
   * @param uiTotalBytesSentInWindow
   * @param uiDurationMs
   * @return
   */
  uint32_t calculateSendRateKbps(const boost::posix_time::ptime& tNow, uint32_t& uiTotalBytesSentInWindow, uint32_t& uiDurationMs);
  /**
   * @brief canSendPacket
   * @param rtpPacket
   * @param uiOutgoingRateKbps
   * @param uiTotalBytesSentInWindow
   * @param uiDurationMs
   * @return
   */
  bool canSendPacket(const RtpPacket& rtpPacket, uint32_t uiOutgoingRateKbps, uint32_t uiTotalBytesSentInWindow, uint32_t uiDurationMs);
private:
  TransmissionManager& m_transmissionManager;
  experimental::NadaSender m_nadaTx;
  experimental::NadaReceiver m_nadaRx;
  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_timer;

  std::deque<RtpPacket> m_qRtpPackets;
  std::map<uint16_t, RtpPacket> m_mRtpPackets;
  // deque for storing recently sent packets to maintain 500ms LOGWIN
  std::deque<RtpPacket> m_qRecentlySendPackets;

  boost::posix_time::ptime m_tStart;
  uint32_t m_uiSsrc;

  ICooperativeCodec* m_pCooperative;
  uint32_t m_uiPreviousKbps;

  uint32_t m_uiMaxOvershootKbps;
  uint32_t m_uiBufferLengthBytes;
  uint32_t m_uiEncoderRateKbps;
  uint32_t m_uiNetworkRateKbps;
};

} // rtp_plus_plus
