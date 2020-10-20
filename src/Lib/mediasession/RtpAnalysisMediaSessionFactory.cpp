#include "CorePch.h"
#include <boost/bind.hpp>
#include <rtp++/mediasession/RtpAnalysisMediaSessionFactory.h>
#include <rtp++/ForwardIncomingRtpSessionManager.h>
#include <rtp++/mediasession/RtpAnalysisMediaSession.h>

namespace rtp_plus_plus
{

boost::shared_ptr<SimpleMediaSession> RtpAnalysisMediaSessionFactory::doCreateMediaSession()
{
  return boost::make_shared<RtpAnalysisMediaSession>();
}

std::unique_ptr<RtpSessionManager>
RtpAnalysisMediaSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                            const RtpSessionParameters& rtpParameters,
                                                            const GenericParameters &applicationParameters)
{
  ForwardIncomingRtpSessionManager* pManager = new ForwardIncomingRtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

  // we know the media session is a RtpAnalysisMediaSession from doCreateMediaSession
  RtpAnalysisMediaSession& analysisMediaSession = dynamic_cast<RtpAnalysisMediaSession&>(simpleMediaSession);

  // set callback depending on audio or video
  if (rtpParameters.getMediaType() == rfc4566::AUDIO)
  {
    pManager->setRtpForward(boost::bind(&RtpAnalysisMediaSession::handleIncomingAudio,
                                        boost::ref(analysisMediaSession),
                                        _1, _2, _3, _4, _5));
    analysisMediaSession.setAudioClockFrequency(rtpParameters.getRtpTimestampFrequency());
  }
  else
  {
    pManager->setRtpForward(boost::bind(&RtpAnalysisMediaSession::handleIncomingVideo,
                                        boost::ref(analysisMediaSession),
                                        _1, _2, _3, _4, _5));
    analysisMediaSession.setVideoClockFrequency(rtpParameters.getRtpTimestampFrequency());
  }
  return std::unique_ptr<RtpSessionManager>(pManager);
}

}
