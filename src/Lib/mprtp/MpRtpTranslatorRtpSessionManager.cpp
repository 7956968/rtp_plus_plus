#include "CorePch.h"
#include <rtp++/mprtp/MpRtpTranslatorRtpSessionManager.h>

namespace rtp_plus_plus
{
namespace mprtp
{

std::unique_ptr<RtpSessionManager> MpRtpTranslatorRtpSessionManager::create(boost::asio::io_service& ioService,
                                                 const GenericParameters& applicationParameters,
                                                 RtpNotifier_t onRtp)
{
  return std::unique_ptr<RtpSessionManager>(new MpRtpTranslatorRtpSessionManager(ioService, applicationParameters, onRtp));
}

MpRtpTranslatorRtpSessionManager::MpRtpTranslatorRtpSessionManager(boost::asio::io_service& ioService,
                                                                   const GenericParameters& applicationParameters,
                                                                   RtpNotifier_t onRtp)
  :RtpSessionManager(ioService, applicationParameters),
    m_onRtp(onRtp)
{

}

void MpRtpTranslatorRtpSessionManager::doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                                 bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation)
{
  // NOTE: could do slight buffering here on e.g. fragmentation units
  m_onRtp(rtpPacket, ep);
}

void MpRtpTranslatorRtpSessionManager::doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep)
{

}

}
}
