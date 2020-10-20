#include "CorePch.h"
#include <rtp++/mediasession/VirtualMediaSessionFactory.h>
#include <rtp++/RtpSessionManager.h>
#include <rtp++/rto/VirtualRtpSession.h>
#include <rtp++/mediasession/MediaSessionDescription.h>

namespace rtp_plus_plus
{

VirtualMediaSessionFactory::~VirtualMediaSessionFactory()
{

}

RtpSession::ptr VirtualMediaSessionFactory::doCreateRtpSession(SimpleMediaSession& simpleMediaSession, const RtpSessionParameters& rtpParameters, 
                                                               RtpReferenceClock& rtpReferenceClock, const GenericParameters &applicationParameters, bool bIsSender)
{
  rto::VirtualRtpSession::ptr pRtpSession = rto::VirtualRtpSession::create(simpleMediaSession.getIoService(), rtpParameters, rtpReferenceClock, applicationParameters, bIsSender, false);
  return pRtpSession;
}

std::unique_ptr<GroupedRtpSessionManager> VirtualMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender)
{
  // Subclasses can override this to create a grouped session manager that manages multiple RTP sessions, one for each object in the group
  return std::unique_ptr<GroupedRtpSessionManager>();
}

}
