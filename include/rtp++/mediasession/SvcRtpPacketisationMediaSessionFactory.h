#pragma once
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>

namespace rtp_plus_plus
{

class SvcRtpPacketisationMediaSessionFactory : public SimpleMediaSessionFactory
{
public:

  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);

  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);

  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender);
  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSessionV2& simpleMediaSession, const RtpSessionParametersGroup_t& parameters, const GenericParameters &applicationParameters, bool bIsSender);
};

}
