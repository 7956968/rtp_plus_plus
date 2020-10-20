#pragma once
#include <string>
#include <vector>
#include <boost/system/error_code.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/MemberUpdate.h>
#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpSession.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/media/IMediaSampleSink.h>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc3550/Rtcp.h>

namespace rtp_plus_plus
{

typedef boost::function<void (const rfc3550::RtcpSr&, const EndPoint&)> SrNotifier_t;
typedef boost::function<void (const rfc3550::RtcpRr&, const EndPoint&)> RrNotifier_t;
typedef boost::function<void (const rfc3550::RtcpSdes&, const EndPoint&)> SdesNotifier_t;
typedef boost::function<void (const rfc3550::RtcpApp&, const EndPoint&)> AppNotifier_t;
typedef boost::function<void (const rfc3550::RtcpBye&, const EndPoint&)> ByeNotifier_t;
typedef boost::function<void ()> RtpSessionCompleteNotifier_t;

/**
 * @brief The IRtpSessionManager class abstraction exists purely so that we can have
 * grouped and non-grouped RTP session managers
 */
class IRtpSessionManager : public media::IMediaSampleSource,
                           public media::IMediaSampleSink
{
public:
  virtual ~IRtpSessionManager()
  {

  }

  RtpReferenceClock& getRtpReferenceClock() 
  {
    return m_referenceClock;
  }

  void setSrNotifier(SrNotifier_t onSr) { m_onSr = onSr; }
  void setRrNotifier(RrNotifier_t onRr) { m_onRr = onRr; }
  void setSdesNotifier(SdesNotifier_t onSdes) { m_onSdes = onSdes; }
  void setAppNotifier(AppNotifier_t onApp) { m_onApp = onApp; }
  void setByeNotifier(ByeNotifier_t onBye) { m_onBye = onBye; }
  void setRtpSessionCompleteNotifier(RtpSessionCompleteNotifier_t onComplete) { m_onRtpSessionComplete = onComplete; }
  /**
   * @brief getMediaTypes The derived class must return the media types of the RTP session(s)
   * @return a vector of media types
   */
  virtual std::vector<std::string> getMediaTypes() const = 0;
  /**
   * @brief getPrimaryMediaType The derived class must return the primary media type of the RTP session.
   * This will be the actual media type of the media, and not the FEC or retransmission media type.
   * @return the primary media type
   */
  virtual std::string getPrimaryMediaType() const = 0;
  /**
   * @brief getPrimaryRtpSessionParameters The derived class must return the primary RTP session parameters
   * of the RTP session. In a grouped session, this will be the RTP session parameters of the actual
   * media, and not the FEC or retransmission session parameters.
   * @return the primary media type
   */
  virtual const RtpSessionParameters& getPrimaryRtpSessionParameters() const = 0;
  /**
   * @brief getPrimaryRtpSession The derived class must return the primary RTP session
   * @return the primary RTP session
   */
  virtual RtpSession& getPrimaryRtpSession() = 0;
  /**
   * @brief setCooperative
   * @param pCooperative
   */
  void setCooperative(std::shared_ptr<ICooperativeCodec> pCooperative) { m_pCooperative = pCooperative; }
  /**
   * @brief onMemberUpdate This method is called when member updates occur
   * @param memberUpdate The updated session information
   */
  virtual void handleMemberUpdate(const MemberUpdate& memberUpdate)
  {
    VLOG(15) << "Member update: " << memberUpdate;
  }
  virtual void handleRtpSessionComplete()
  {
    if (m_onRtpSessionComplete) m_onRtpSessionComplete();
  }

  virtual boost::system::error_code start() = 0;
  virtual boost::system::error_code stop() = 0;

protected:

  IRtpSessionManager(const GenericParameters& applicationParameters)
    :m_applicationParameters(applicationParameters)
  {

  }

  GenericParameters m_applicationParameters;
  RtpReferenceClock m_referenceClock;

  // notifiers
  SrNotifier_t m_onSr;
  RrNotifier_t m_onRr;
  SdesNotifier_t m_onSdes;
  AppNotifier_t m_onApp;
  ByeNotifier_t m_onBye;
  RtpSessionCompleteNotifier_t m_onRtpSessionComplete;

  std::weak_ptr<ICooperativeCodec> m_pCooperative;
};

} // rtp_plus_plus


