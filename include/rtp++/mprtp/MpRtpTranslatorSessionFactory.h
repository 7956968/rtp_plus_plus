#pragma once
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/mprtp/MpRtpTranslatorRtpSessionManager.h>

namespace rtp_plus_plus
{
namespace mprtp {

/**
 * @brief The MpRtpTranslatorSessionFactory class Implementation for creating MpRtpTranslatorRtpSessionManager
 * This class has to retain state to set the audio and video callbacks correctly
 */
class MpRtpTranslatorSessionFactory : public SimpleMediaSessionFactory
{
public:

  MpRtpTranslatorSessionFactory(RtpNotifier_t videoCallback, RtpNotifier_t audioCallback);

  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);

private:
  RtpNotifier_t m_videoCallback;
  RtpNotifier_t m_audioCallback;

};

}
}
