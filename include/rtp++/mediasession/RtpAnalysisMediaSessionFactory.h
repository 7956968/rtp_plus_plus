#pragma once
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>

namespace rtp_plus_plus
{

class RtpAnalysisMediaSessionFactory : public SimpleMediaSessionFactory
{
protected:

  /**
    * Override creation of media session so that we can analyse audio and video together
    */
  virtual boost::shared_ptr<SimpleMediaSession> doCreateMediaSession();


  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);
};

}
