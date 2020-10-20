#pragma once
#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpSession.h>
#include <rtp++/GroupedRtpSessionManager.h>
#include <rtp++/RtpSessionManager.h>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/network/ExistingConnectionAdapter.h>

namespace rtp_plus_plus
{

/**
 * @brief Class to create a simple media session given a MediaSessionDescription object
 * This class takes the *first* audio and video session found in the media description
 * and creates a corresponding std::unique_ptr<IRtpSessionManager> object
 */
class SimpleMediaSessionFactory
{
public:
  virtual ~SimpleMediaSessionFactory();

  boost::shared_ptr<SimpleMediaSession> create(const MediaSessionDescription& mediaDescription,
                                               const GenericParameters &applicationParameters,
                                               std::shared_ptr<ICooperativeCodec> pCooperative = std::shared_ptr<ICooperativeCodec>());

  boost::shared_ptr<SimpleMediaSessionV2> create(boost::asio::io_service& ioService,
                                                 const MediaSessionDescription& mediaDescription,
                                                 const GenericParameters &applicationParameters,
                                                 std::shared_ptr<ICooperativeCodec> pCooperative = std::shared_ptr<ICooperativeCodec>());

  boost::shared_ptr<SimpleMediaSessionV2> create(boost::asio::io_service& ioService,
                                                 ExistingConnectionAdapter& adapter,
                                                 const MediaSessionDescription& mediaDescription,
                                                 const GenericParameters &applicationParameters,
                                                 std::shared_ptr<ICooperativeCodec> pCooperative = std::shared_ptr<ICooperativeCodec>());

protected:
  virtual boost::shared_ptr<SimpleMediaSession> doCreateMediaSession();

  virtual RtpSession::ptr doCreateRtpSession(SimpleMediaSession& simpleMediaSession,
                                             const RtpSessionParameters& rtpParameters,
                                             RtpReferenceClock& rtpReferenceClock,
                                             const GenericParameters &applicationParameters,
                                             bool bIsSender);

  virtual RtpSession::ptr doCreateGroupedRtpSession(SimpleMediaSession& simpleMediaSession,
                                                    const RtpSessionParameters& rtpParameters,
                                                    RtpReferenceClock& rtpReferenceClock,
                                                    const GenericParameters &applicationParameters,
                                                    bool bIsSender);

  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);

  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                                  const RtpSessionParametersGroup_t& parameters,
                                                                                  const GenericParameters &applicationParameters,
                                                                                  bool bIsSender);

  virtual boost::shared_ptr<SimpleMediaSessionV2> doCreateMediaSession(boost::asio::io_service& ioService);

  virtual RtpSession::ptr doCreateRtpSession(SimpleMediaSessionV2& simpleMediaSession,
                                             const RtpSessionParameters& rtpParameters,
                                             RtpReferenceClock& rtpReferenceClock,
                                             const GenericParameters &applicationParameters,
                                             bool bIsSender,
                                             ExistingConnectionAdapter adapter = ExistingConnectionAdapter());

  virtual RtpSession::ptr doCreateGroupedRtpSession(SimpleMediaSessionV2& simpleMediaSession,
                                                    const RtpSessionParameters& rtpParameters,
                                                    RtpReferenceClock& rtpReferenceClock,
                                                    const GenericParameters &applicationParameters,
                                                    bool bIsSender,
                                                    ExistingConnectionAdapter adapter = ExistingConnectionAdapter());

  virtual std::unique_ptr<RtpSessionManager> doCreateSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     const GenericParameters &applicationParameters);

  virtual std::unique_ptr<GroupedRtpSessionManager> doCreateGroupedSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                                                  const RtpSessionParametersGroup_t& parameters,
                                                                                  const GenericParameters &applicationParameters,
                                                                                  bool bIsSender);


private:

  // TODO: need to refactor all these methods!!!!!!!
  std::unique_ptr<IRtpSessionManager> createSessionManager(const MediaSessionDescription& mediaDescription,
                                                           SimpleMediaSession& simpleMediaSession,
                                                           boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                           const GenericParameters &applicationParameters,
                                                           std::shared_ptr<ICooperativeCodec> pCooperative);

  std::unique_ptr<IRtpSessionManager> createSessionManager(const MediaSessionDescription& mediaDescription,
                                                           SimpleMediaSessionV2& simpleMediaSession,
                                                           boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                           const GenericParameters &applicationParameters,
                                                           std::shared_ptr<ICooperativeCodec> pCooperative);

  std::unique_ptr<IRtpSessionManager> createSessionManager(const MediaSessionDescription& mediaDescription,
                                                           ExistingConnectionAdapter& adapter,
                                                           SimpleMediaSessionV2& simpleMediaSession,
                                                           boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                           const GenericParameters &applicationParameters,
                                                           std::shared_ptr<ICooperativeCodec> pCooperative);

};

} // rtp_plus_plus
