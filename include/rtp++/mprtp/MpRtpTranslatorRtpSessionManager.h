#pragma once
#include <rtp++/RtpSessionManager.h>

namespace rtp_plus_plus
{
namespace mprtp
{

/**
 * @brief The MpRtpTranslatorRtpSessionManager class translates RTP packets to MPRTP,
 * and vice versa
 */
typedef boost::function<void (const RtpPacket& rtpPacket, const EndPoint& ep)> RtpNotifier_t;

class MpRtpTranslatorRtpSessionManager : public RtpSessionManager
{
public:

  static std::unique_ptr<RtpSessionManager> create(boost::asio::io_service& ioService,
                                                   const GenericParameters& applicationParameters,
                                                   RtpNotifier_t onRtp);

  MpRtpTranslatorRtpSessionManager(boost::asio::io_service& ioService,
                                   const GenericParameters& applicationParameters,
                                   RtpNotifier_t onRtp);

  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                                   bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation);

  virtual void doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep);

  // For event notification
  RtpNotifier_t m_onRtp;
};

}
}
