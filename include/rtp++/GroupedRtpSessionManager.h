#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <rtp++/IRtpSessionManager.h>
#include <rtp++/RtpSession.h>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus
{

/**
 * Base class for grouped RTP sessions. The implementation is responsible for sending media samples
 * in the target RTP sessions, and for e.g. remultiplexing media
 */
class GroupedRtpSessionManager : public IRtpSessionManager
{
public:

  virtual ~GroupedRtpSessionManager();

  void addRtpSession(const RtpSessionParameters& rtpParameters, RtpSession::ptr pRtpSession);

  /**
   * @brief setCooperative
   * @param pCooperative
   */
  void setCooperative(std::shared_ptr<ICooperativeCodec> pCooperative) { m_pCooperative = pCooperative; }

  virtual std::vector<std::string> getMediaTypes() const;
  virtual std::string getPrimaryMediaType() const;
  virtual const RtpSessionParameters& getPrimaryRtpSessionParameters() const;
  virtual RtpSession& getPrimaryRtpSession();
  virtual boost::system::error_code start();
  virtual boost::system::error_code stop();

  virtual void send(const media::MediaSample& mediaSample);
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples);

  virtual void send(const media::MediaSample& mediaSample, const std::string& sMid);
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid);
  /**
   * @brief isSampleAvailable for use cases where one media sample is generated from multiple grouped sessions
   * @return
   */
  virtual bool isSampleAvailable() const;
  /**
   * @brief isSampleAvailable for use cases where there are multiple independently retrieved media samples
   * @param sMid
   * @return
   */
  virtual bool isSampleAvailable(const std::string& sMid) const;
  /**
   * @brief getNextSample for use cases where one media sample is generated from multiple grouped sessions
   * e.g in scenarios with FEC or SVC MST
   * @return
   */
  virtual std::vector<media::MediaSample> getNextSample();
  /**
   * @brief getNextSample for use cases where there are multiple independently retrieved media samples
   * e.g. audio and video bundles
   * @param sMid
   * @return
   */
  virtual std::vector<media::MediaSample> getNextSample(const std::string& sMid);
  /**
   * This method can be used to get an RTP session identified by a MID line
   * It is assumed that the subclass implements some kind of logic
   * to allow it to identify which MID an RTP sample should be delivered by
   */
  RtpSession::ptr getRtpSession(const std::string& sMid);
  /**
   * This method allows subclasses to get MID information
   */
  const std::string& getMid(uint32_t uiIndex) const;
  /**
   * Allows subclasses to retrieve number of sessions managed by grouped session manager
   */
  const uint32_t getSessionCount() const;

  void handleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                         bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation,
                         const std::string& sMid);

  void handleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep,
                          const std::string& sMid);

  void handleGroupedMemberUpdate(const MemberUpdate& memberUpdate, const std::string &sMid);

protected:

  GroupedRtpSessionManager(const GenericParameters& applicationParameters);

  virtual void doSend(const media::MediaSample& mediaSample) = 0;
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples) = 0;
  virtual bool doIsSampleAvailable() const = 0;
  virtual std::vector<media::MediaSample> doGetNextSample() = 0;

  virtual void doSend(const media::MediaSample& mediaSample, const std::string& sMid) = 0;
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid) = 0;
  virtual bool doIsSampleAvailable(const std::string& sMid) const = 0;
  virtual std::vector<media::MediaSample> doGetNextSample(const std::string& sMid) = 0;
  /**
   * Gives subclasses opportunity to store/map info about the RTP session
   */
  virtual void onRtpSessionAdded(const std::string& sMid, const RtpSessionParameters& rtpParameters, RtpSession::ptr pRtpSession);

  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                   bool bSSRCValidated, bool bRtcpSynchronised,
                                   const boost::posix_time::ptime& tPresentation,
                                   const std::string& sMid) = 0;

  virtual void doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep,
                                    const std::string& sMid) = 0;

  /**
   * Give subclasses opportunity to perform configuration etc.
   */
  virtual void onSessionsStarted() = 0;
  virtual void onSessionsStopped() = 0;

private:

  RtpSessionParametersGroup_t m_vRtpParameters;
  std::unordered_map<std::string, RtpSession::ptr> m_mRtpSessions;

  std::vector<std::string> m_vMids;

  enum SessionState
  {
    SS_STOPPED,
    SS_STARTED,       ///< set if start is called successfully
    SS_SHUTTING_DOWN  ///< set when shutting down to prevent new asynchronous operations from being launched
  };
  SessionState m_eState;

  /// Number of byes received used for shutting down application
  uint32_t m_uiByeReceivedCount;
  /// Flag to exit on bye received
  bool m_bExitOnBye;
  /// analyse flag
  bool m_bAnalyse;
  /// vector to store received sequence numbers for post-session analysis
  std::unordered_map<std::string, std::vector<uint32_t> > m_mLostSequenceNumbers;

  std::weak_ptr<ICooperativeCodec> m_pCooperative;
};

} // rtp_plus_plus
