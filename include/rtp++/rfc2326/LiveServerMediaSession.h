#pragma once
#include <string>
#include <rtp++/rfc2326/ServerMediaSession.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/rfc3550/Rtcp.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief The LiveServerMediaSession is used to send live media to RTSP clients.
 * This class uses the SimpleMediaSessionV2 class to deliver media. This means
 * that at most one audio and one video RTP session are supported.
 */
class LiveServerMediaSession : public ServerMediaSession
{
public:
  /**
   * @brief Constructor
   */
  LiveServerMediaSession(RtspServer& rtspServer, const std::string& sSession,
                         const std::string& sSessionDescription, const std::string& sContentType,
                         uint32_t uiMaxTimeWithoutLivenessSeconds);
  /**
   * @brief Destructor
   */
  virtual ~LiveServerMediaSession();

protected:
  /**
   * @brief 
   */
  virtual ResponseCode handleSetup(const RtspMessage& setup, std::string& sSession,
                                   Transport& transport, const std::vector<std::string>& vSupported);
  /**
   * @brief
   */
  virtual ResponseCode doHandlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported);
  /**
   * @brief
   */
  virtual void doDeliver(const uint8_t uiPayloadType, const media::MediaSample& mediaSample);
  /**
   * @brief
   */
  virtual void doDeliver(const uint8_t uiPayloadType, const std::vector<media::MediaSample>& mediaSamples);
  /**
   * @brief
   */
  virtual void doShutdown();
  /**
   * @brief
   */
  virtual bool isSessionLive() const;

private:
  /**
   * @brief updates the time the last liveness indicator was received.
   */
  void updateLiveness();
  /**
   * @brief Called when an RTCP RR is received.
   */
  void onRr(const rfc3550::RtcpRr& rr, const EndPoint& ep);

private:
  /// Local session description
  rfc4566::SessionDescription m_sdp;
  /// Interface descriptions of client
  InterfaceDescriptions_t m_remoteInterfaces;
  /// Simple media session for media sample delivery
  boost::shared_ptr<SimpleMediaSessionV2> m_pMediaSession;
  /// Max time without liveness 
  uint32_t m_uiMaxTimeWithoutLivenessSeconds;
  /// Time last liveness indicator was received
  boost::posix_time::ptime m_tLastLivenessIndicator;
  /// audio payload type
  uint8_t m_uiAudioPayload;
  /// video payload type
  uint8_t m_uiVideoPayload;
};

} // rfc2326
} // rtp_plus_plus

