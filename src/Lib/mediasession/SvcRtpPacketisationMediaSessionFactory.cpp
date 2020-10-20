#include "CorePch.h"
#include <rtp++/mediasession/SvcRtpPacketisationMediaSessionFactory.h>
#include <rtp++/SvcRtpPacketisationSessionManager.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/sctp/SctpSvcRtpPacketisationSessionManager.h>
#include <rtp++/sctp/SctpH265RtpSessionManager.h>

namespace rtp_plus_plus
{

std::unique_ptr<RtpSessionManager>
SvcRtpPacketisationMediaSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                            const RtpSessionParameters& rtpParameters,
                                                            const GenericParameters &applicationParameters)
{
  return std::unique_ptr<RtpSessionManager>();
}

std::unique_ptr<RtpSessionManager>
SvcRtpPacketisationMediaSessionFactory::doCreateSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                            const RtpSessionParameters& rtpParameters,
                                                            const GenericParameters &applicationParameters)
{
  return std::unique_ptr<RtpSessionManager>();
}

std::unique_ptr<GroupedRtpSessionManager> SvcRtpPacketisationMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender)
{
  // TODO:
  // - create RTP sessions for entire media description in group
  // - create grouped session manager instance that manages RTP sessions and schedules packets

  SvcRtpPacketisationSessionManager* pSvcManager = new SvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters);

  return std::unique_ptr<GroupedRtpSessionManager>(pSvcManager);
}

std::unique_ptr<GroupedRtpSessionManager> SvcRtpPacketisationMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSessionV2& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender)
{
  // TODO:
  // - create RTP sessions for entire media description in group
  // - create grouped session manager instance that manages RTP sessions and schedules packets
  std::unique_ptr<GroupedRtpSessionManager> pGroupedSessionManager =
      std::unique_ptr<GroupedRtpSessionManager>(new SvcRtpPacketisationSessionManager(simpleMediaSession.getIoService(), applicationParameters));

  return pGroupedSessionManager;
}

}
