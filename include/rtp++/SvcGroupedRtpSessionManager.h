#pragma once
#include <unordered_map>
#include <rtp++/GroupedRtpSessionManager.h>
#include <rtp++/PtsBasedJitterBuffer.h>

namespace rtp_plus_plus
{

/**
  * Base class for grouped RTP sessions. The implementation is responsible for sending media samples
  * in the target RTP sessions, and for e.g. remultiplexing media
  */
class SvcGroupedRtpSessionManager : public GroupedRtpSessionManager
{
public:
  SvcGroupedRtpSessionManager(boost::asio::io_service& ioService,
                              const GenericParameters& applicationParameters);

protected:

  virtual void doSend(const media::MediaSample& mediaSample);
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples);
  virtual void doSend(const media::MediaSample& mediaSample, const std::string& sMid);
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid);
  virtual bool doIsSampleAvailable() const;
  virtual bool doIsSampleAvailable(const std::string& sMid) const
  {
    // not applicable to SVC use case
    return false;
  }

  virtual std::vector<media::MediaSample> doGetNextSample();
  virtual std::vector<media::MediaSample> doGetNextSample(const std::string& sMid)
  {
    // not applicable to SVC use case
    return std::vector<media::MediaSample>();
  }

  virtual void onRtpSessionAdded(const std::string& sMid,
                                 const RtpSessionParameters& rtpParameters,
                                 RtpSession::ptr pRtpSession);

  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                   bool bSSRCValidated, bool bRtcpSynchronised,
                                   const boost::posix_time::ptime& tPresentation,
                                   const std::string& sMid);

  virtual void doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep,
                          const std::string& sMid);
  
  virtual void onSessionsStarted();
  virtual void onSessionsStopped();

  void onScheduledSampleTimeout(const boost::system::error_code& ec,
                                boost::asio::deadline_timer* pTimer);

private:

  /// asio service
  boost::asio::io_service& m_rIoService;
  /// RTP jitter buffer
  PtsBasedJitterBuffer m_receiverBuffer;
  /// Timers for scheduled media sample delivery to application layer
  std::unordered_map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mPlayoutTimers;
  /// Lock for media sample queue
  mutable boost::mutex m_outgoingSampleLock;
  /// Outgoing media sample queue
  // MediaSampleQueue_t m_qOutgoing;
  std::deque<std::vector<media::MediaSample> > m_qOutgoing;
};

}
