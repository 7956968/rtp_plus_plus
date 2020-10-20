#include "CorePch.h"
#include <rtp++/mediasession/RtpPacketisationMediaSessionFactory.h>
#include <rtp++/RtpPacketisationSessionManager.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/sctp/SctpSvcRtpPacketisationSessionManager.h>
#include <rtp++/sctp/SctpShvcRtpPacketisationSessionManager.h>
// #include <rtp++/sctp/SctpH265RtpSessionManager.h>

namespace rtp_plus_plus
{

std::unique_ptr<RtpSessionManager>
RtpPacketisationMediaSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                            const RtpSessionParameters& rtpParameters,
                                                            const GenericParameters &applicationParameters)
{
  if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6190::H264_SVC)
  {
    sctp::SctpSvcRtpPacketisationSessionManager* pRtpSessionManager =
        new sctp::SctpSvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfchevc::H265)
  {
    sctp::SctpShvcRtpPacketisationSessionManager* pRtpSessionManager =
      new sctp::SctpShvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }

  return RtpPacketisationSessionManager::create(simpleMediaSession.getIoService(), applicationParameters);
}

std::unique_ptr<RtpSessionManager>
RtpPacketisationMediaSessionFactory::doCreateSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                            const RtpSessionParameters& rtpParameters,
                                                            const GenericParameters &applicationParameters)
{
  if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6190::H264_SVC)
  {
    sctp::SctpSvcRtpPacketisationSessionManager* pRtpSessionManager =
        new sctp::SctpSvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfchevc::H265)
  {
    sctp::SctpShvcRtpPacketisationSessionManager* pRtpSessionManager =
      new sctp::SctpShvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }

  return RtpPacketisationSessionManager::create(simpleMediaSession.getIoService(), applicationParameters);
}

}
