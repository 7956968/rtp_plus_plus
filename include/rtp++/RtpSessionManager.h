#pragma once
#include <tuple>
#include <boost/date_time/posix_time/ptime.hpp>
#include <rtp++/IRtpJitterBuffer.h>
#include <rtp++/IRtpSessionManager.h>
#include <rtp++/TransmissionManager.h>
#include <rtp++/RtpSession.h>
#include <rtp++/rfc3611/RtcpXr.h>
#include <rtp++/rfc4585/RtcpFb.h>

namespace rtp_plus_plus
{

/// fwd
class TransmissionManager;
class RtpScheduler;

/// arrival time, presentation time, RTCP synced
typedef std::tuple<boost::posix_time::ptime, boost::posix_time::ptime, bool> RtpPacketStat;

/**
 * @brief The RtpSessionManager class manages a single RTP session.
 */
class RtpSessionManager : public IRtpSessionManager
{
public:
  /**
   * @brief Named constructor
   */
  static std::unique_ptr<RtpSessionManager> create(boost::asio::io_service& ioService,
                                                   const GenericParameters& applicationParameters);
  /**
   * @brief Constructor
   */
  RtpSessionManager(boost::asio::io_service& ioService,
                    const GenericParameters& applicationParameters);
  /**
   * @brief Destructor
   */
  virtual ~RtpSessionManager();
  /**
   * @brief Getter for managed RTP session
   */
  RtpSession::ptr getRtpSession() const { return m_pRtpSession; }
  /**
   * @brief Setter for managed RTP session
   */
  void setRtpSession(RtpSession::ptr pRtpSession);

  virtual std::vector<std::string> getMediaTypes() const;
  virtual std::string getPrimaryMediaType() const;
  virtual const RtpSessionParameters& getPrimaryRtpSessionParameters() const;
  virtual RtpSession& getPrimaryRtpSession();
  /**
   * @brief Getter for RTP packet stats
   */
  const std::vector<RtpPacketStat>& getPacketStats() const { return m_vPacketStats; }
  /**
   * @brief Starts RTP session
   */
  virtual boost::system::error_code start();
  /**
   * @brief Stops RTP session
   */
  virtual boost::system::error_code stop();

  virtual void send(const media::MediaSample& mediaSample);
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples);
  virtual bool isSampleAvailable() const;
  virtual std::vector<media::MediaSample> getNextSample();

  virtual void send(const media::MediaSample& mediaSample, const std::string& sMid)
  {
    // NOOP
    assert(false);
  }
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid)
  {
    // NOOP
    assert(false);
  }
  virtual bool isSampleAvailable(const std::string& sMid) const
  {
    // NOOP
    assert(false);
    return false;
  }
  virtual std::vector<media::MediaSample> getNextSample(const std::string& sMid)
  {
    // NOOP
    assert(false);
    return std::vector<media::MediaSample>();
  }

protected:
  /**
   * @brief onMemberUpdate This method is called when member updates occur
   * @param memberUpdate The updated session information
   */
  virtual void handleMemberUpdate(const MemberUpdate& memberUpdate);

private:
  void handleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                         bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation);

  void handleBeforeOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  void handleOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep);

  void handleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep);

  void handleOutgoingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep);

  void handleNacks(const std::vector<uint16_t>& nacks, const EndPoint &ep );
  void handleNacks(uint16_t uiFlowId, const std::vector<uint16_t>& nacks, const EndPoint &ep );
  void handleAcks(const std::vector<uint16_t>& acks, const EndPoint &ep );

  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                                   bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation);
  virtual void doHandleBeforeOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  virtual void doHandleOutgoingRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  virtual void doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep);

  virtual void doHandleXr(const rfc3611::RtcpXr& xr, const EndPoint& ep);
  virtual void doHandleFb(const rfc4585::RtcpFb& fb, const EndPoint& ep);

  virtual void onFeedbackGeneration(CompoundRtcpPacket& compoundRtcp);
  virtual void onMpFeedbackGeneration(uint16_t uiFlowId, CompoundRtcpPacket& compoundRtcp);

  void onPacketAssumedLost(uint16_t uiSN);
  void onPacketAssumedLostInFlow(uint16_t uiFlowId, uint16_t uiFSSN );
  /**
   * @brief This method is called when the sample exits the jitter buffer. These deadlines
   * are based on a relative offset to the time that the first RTP packet was received.
   * @param ec the boost asio timer error code
   * @param pTimer a pointer to the timer used for this timeout
   */
  void onScheduledSampleTimeout(const boost::system::error_code& ec,
                                boost::asio::deadline_timer* pTimer);
  /**
   * @brief prepareSampleForOutput should be called by subclasses once a media sample
   * or access unit is ready for output
   * @param vMediaSamples
   */
  void prepareSampleForOutput(const std::vector<media::MediaSample>& vMediaSamples);
  /**
   * Give subclasses opportunity to perform configuration on session start.
   */
  virtual void onSessionStarted();
  /**
   * Give subclasses opportunity to perform configuration on session stop.
   */
  virtual void onSessionStopped();

protected:
  /// asio service
  boost::asio::io_service& m_rIoService;
  /// RTP session object that is being managed
  RtpSession::ptr m_pRtpSession;
  /// Scheduler
  std::unique_ptr<RtpScheduler> m_pScheduler;
  /// RTP jitter buffer
  std::unique_ptr<IRtpJitterBuffer> m_pReceiverBuffer;
  /// RTX
  std::unique_ptr<TransmissionManager> m_pTxManager;
  /// FB management: TODO: rename to loss manager and abstract feedback out into feedback interface
  std::unique_ptr<IFeedbackManager> m_pFeedbackManager;
  /// Lock for timer map
  mutable boost::mutex m_timerLock;
  /// Timers for scheduled media sample delivery to application layer
  std::unordered_map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mPlayoutTimers;
  /// Lock for media sample queue
  mutable boost::mutex m_outgoingSampleLock;
  /// Outgoing media sample queue
  std::deque<std::vector<media::MediaSample> > m_qOutgoing;
  /// Codec
  std::string m_sCodec;
  /// Flag to exit on bye received
  bool m_bExitOnBye;
  /// analyse flag
  bool m_bAnalyse;
  /// vector to store received sequence numbers for session analysis
  std::vector<uint32_t> m_vSequenceNumbers;
  /// vector to store discarded sequence numbers for session analysis
  std::vector<uint32_t> m_vDiscards;
  /// vector to store time late in ms for session analysis 
  std::vector<uint32_t> m_vLateness;
  /// timing stats
  std::vector<RtpPacketStat> m_vPacketStats;
  /// Debug logging
  bool m_bFirstSyncedVideo;
  /// last received SN used to determine when to send more acks
  uint32_t m_uiLastReceived;
  /// ack enabled
  bool m_bAckEnabled;
  /// nack enabled
  bool m_bNackEnabled;

#if 0
  /// scream enabled
  bool m_bScreamEnabled;
  /// SN in last scream report
  uint32_t m_uiLastExtendedSNReported;
  /// loss
  uint32_t m_uiLastLoss;
#endif

  // Map to store RTX times
  std::unordered_map<uint16_t, boost::posix_time::ptime> m_mRtxDurationMap;
  std::unordered_map<uint32_t, boost::posix_time::ptime> m_mFlowSpecificRtxDurationMap;
};

} // rtp_plus_plus
