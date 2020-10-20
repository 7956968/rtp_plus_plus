#pragma once

#include <rtp++/mediasession/SimpleMediaSessionFactory.h>

namespace rtp_plus_plus
{

/**
  * Class to create a simple media session given a MediaSessionDescription object
  * This class takes the *first* audio and video session found in the media description
  * and creates a corresponding std::unique_ptr<IRtpSessionManager> object
  */
class SvcMediaSessionFactory : public SimpleMediaSessionFactory
{
public:
  virtual ~SvcMediaSessionFactory();

protected:
  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters);
  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender);

};

}
