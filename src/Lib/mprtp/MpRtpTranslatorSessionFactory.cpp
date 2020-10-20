#include "CorePch.h"
#include <rtp++/mprtp/MpRtpTranslatorSessionFactory.h>
#include <rtp++/mprtp/MpRtpTranslator.h>

namespace rtp_plus_plus
{
namespace mprtp
{

MpRtpTranslatorSessionFactory::MpRtpTranslatorSessionFactory(RtpNotifier_t videoCallback, RtpNotifier_t audioCallback)
  :m_videoCallback(videoCallback),
    m_audioCallback(audioCallback)
{

}

std::unique_ptr<RtpSessionManager> MpRtpTranslatorSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                                                 const RtpSessionParameters& rtpParameters,
                                                                                                 const GenericParameters &applicationParameters)
{
  if (rtpParameters.getMediaType() == rfc4566::AUDIO)
  {
    return MpRtpTranslatorRtpSessionManager::create(simpleMediaSession.getIoService(), applicationParameters, m_audioCallback);
  }
  else if (rtpParameters.getMediaType() == rfc4566::VIDEO)
  {
    return MpRtpTranslatorRtpSessionManager::create(simpleMediaSession.getIoService(), applicationParameters, m_videoCallback);
  }
  else
  {
    LOG(ERROR) << "Unknown media type";
    return std::unique_ptr<RtpSessionManager>();
  }
}

}
}
