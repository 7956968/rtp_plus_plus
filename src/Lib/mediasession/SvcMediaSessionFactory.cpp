#include "CorePch.h"
#include <rtp++/mediasession/SvcMediaSessionFactory.h>
#include <rtp++/RtpSession.h>
#include <rtp++/SvcGroupedRtpSessionManager.h>
#include <rtp++/RtpSessionManager.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>

namespace rtp_plus_plus
{

SvcMediaSessionFactory::~SvcMediaSessionFactory()
{

}

std::unique_ptr<RtpSessionManager> SvcMediaSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters)
{
  // create SST SVC session
  return RtpSessionManager::create(simpleMediaSession.getIoService(), applicationParameters);
}

std::unique_ptr<GroupedRtpSessionManager> SvcMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender)
{
  // TODO:
  // - create RTP sessions for entire media description in group
  // - create grouped session manager instance that manages RTP sessions and schedules packets
  std::unique_ptr<GroupedRtpSessionManager> pGroupedSessionManager =
      std::unique_ptr<GroupedRtpSessionManager>(new SvcGroupedRtpSessionManager(simpleMediaSession.getIoService(), applicationParameters));

  return pGroupedSessionManager;
}

}
