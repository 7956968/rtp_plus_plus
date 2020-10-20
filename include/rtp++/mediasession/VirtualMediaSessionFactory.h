#pragma once

#include <rtp++/mediasession/SimpleMediaSessionFactory.h>

namespace rtp_plus_plus
{

/**
  * Class to create a simple media session given a MediaSessionDescription object
  * This class takes the *first* audio and video session found in the media description
  * and creates a corresponding std::unique_ptr<IRtpSessionManager> object
  */
class VirtualMediaSessionFactory : public SimpleMediaSessionFactory
{
public:
  virtual ~VirtualMediaSessionFactory();

protected:
  virtual RtpSession::ptr doCreateRtpSession(SimpleMediaSession& simpleMediaSession, const RtpSessionParameters& rtpParameters, 
                                                 RtpReferenceClock& rtpReferenceClock, const GenericParameters &applicationParameters, bool bIsSender);
  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender);

};

}
