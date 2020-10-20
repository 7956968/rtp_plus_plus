#include "CorePch.h"
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <boost/bind.hpp>
#include <functional>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/GroupedRtpSessionManager.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/RtpSessionManager.h>
#include <rtp++/sctp/SctpH264RtpSessionManager.h>
#include <rtp++/sctp/SctpH265RtpSessionManager.h>
#include <rtp++/sctp/SctpSvcRtpSessionManager.h>
#include <rtp++/sctp/SctpShvcRtpSessionManager.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{

using app::ApplicationParameters;

SimpleMediaSessionFactory::~SimpleMediaSessionFactory()
{

}

boost::shared_ptr<SimpleMediaSession> SimpleMediaSessionFactory::create(const MediaSessionDescription& mediaDescription,
                                                                        const GenericParameters& applicationParameters,
                                                                        std::shared_ptr<ICooperativeCodec> pCooperative)
{
  auto pSimpleMediaSession = doCreateMediaSession();

  bool bVideo = false, bAudio = false;

  // add video session
  boost::optional<MediaSessionDescription::MediaIndex_t> index = mediaDescription.find(rfc4566::VIDEO);

  if (index)
  {
    std::unique_ptr<IRtpSessionManager> pVideoManager = createSessionManager(mediaDescription, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);
    if (pVideoManager)
    {
      bVideo = true;
      pSimpleMediaSession->setVideoSessionManager(std::move(pVideoManager));
    }
    else
    {
      LOG(WARNING) << "No video session manager";
    }
  }
  else
  {
    LOG(INFO) << "No video in SDP";
  }

  // add audio session
  index = mediaDescription.find(rfc4566::AUDIO);
  if (index)
  {
    std::unique_ptr<IRtpSessionManager> pAudioManager = createSessionManager(mediaDescription, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);

    if (pAudioManager)
    {
      pSimpleMediaSession->setAudioSessionManager(std::move(pAudioManager));
      bAudio = true;
    }
    else
    {
      LOG(INFO) << "No audio session manager";
    }
  }
  else
  {
    LOG(INFO) << "No audio in SDP";
  }

  // make sure we were at least able to create a video or audio session
  if (bVideo || bAudio)
    return pSimpleMediaSession;
  else
  {
    return boost::shared_ptr<SimpleMediaSession>();
  }
}

boost::shared_ptr<SimpleMediaSessionV2> SimpleMediaSessionFactory::create(boost::asio::io_service& ioService,
  ExistingConnectionAdapter& adapter,
  const MediaSessionDescription& mediaDescription,
  const GenericParameters& applicationParameters,
  std::shared_ptr<ICooperativeCodec> pCooperative)
{
  VLOG(10) << "SimpleMediaSessionFactory::create";
  auto pSimpleMediaSession = doCreateMediaSession(ioService);

  bool bVideo = false, bAudio = false;

  // add video session
  VLOG(6) << "SimpleMediaSessionFactory::create - creating video session manager";
  boost::optional<MediaSessionDescription::MediaIndex_t> index = mediaDescription.find(rfc4566::VIDEO);
  std::unique_ptr<IRtpSessionManager> pVideoManager = createSessionManager(mediaDescription, adapter, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);
  if (pVideoManager)
  {
    bVideo = true;
    pSimpleMediaSession->setVideoSessionManager(std::move(pVideoManager));
  }
  else
  {
    VLOG(5) << "No video session manager";
  }

  // add audio session
  VLOG(6) << "SimpleMediaSessionFactory::create - creating audio session manager";
  index = mediaDescription.find(rfc4566::AUDIO);
  std::unique_ptr<IRtpSessionManager> pAudioManager = createSessionManager(mediaDescription, adapter, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);

  if (pAudioManager)
  {
    pSimpleMediaSession->setAudioSessionManager(std::move(pAudioManager));
    bAudio = true;
  }
  else
  {
    VLOG(5) << "No audio session manager";
  }

  // make sure we were at least able to create a video or audio session
  if (bVideo || bAudio)
    return pSimpleMediaSession;
  else
  {
    return boost::shared_ptr<SimpleMediaSessionV2>();
  }
}

/// V2
boost::shared_ptr<SimpleMediaSessionV2> SimpleMediaSessionFactory::create(boost::asio::io_service& ioService,
  const MediaSessionDescription& mediaDescription,
  const GenericParameters& applicationParameters,
  std::shared_ptr<ICooperativeCodec> pCooperative)
{
  auto pSimpleMediaSession = doCreateMediaSession(ioService);

  bool bVideo = false, bAudio = false;

  // add video session
  boost::optional<MediaSessionDescription::MediaIndex_t> index = mediaDescription.find(rfc4566::VIDEO);
  std::unique_ptr<IRtpSessionManager> pVideoManager = createSessionManager(mediaDescription, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);
  if (pVideoManager)
  {
    bVideo = true;
    pSimpleMediaSession->setVideoSessionManager(std::move(pVideoManager));
  }
  else
  {
    VLOG(5) << "No video session manager";
  }

  // add audio session
  index = mediaDescription.find(rfc4566::AUDIO);
  std::unique_ptr<IRtpSessionManager> pAudioManager = createSessionManager(mediaDescription, *pSimpleMediaSession.get(), index, applicationParameters, pCooperative);

  if (pAudioManager)
  {
    pSimpleMediaSession->setAudioSessionManager(std::move(pAudioManager));
    bAudio = true;
  }
  else
  {
    VLOG(5) << "No audio session manager";
  }

  // make sure we were at least able to create a video or audio session
  if (bVideo || bAudio)
    return pSimpleMediaSession;
  else
  {
    return boost::shared_ptr<SimpleMediaSessionV2>();
  }
}

boost::shared_ptr<SimpleMediaSession> SimpleMediaSessionFactory::doCreateMediaSession()
{
  return boost::make_shared<SimpleMediaSession>();
}

RtpSession::ptr SimpleMediaSessionFactory::doCreateRtpSession(SimpleMediaSession& simpleMediaSession,
                                                              const RtpSessionParameters& rtpParameters,
                                                              RtpReferenceClock& rtpReferenceClock,
                                                              const GenericParameters &applicationParameters,
                                                              bool bIsSender)
{
  return RtpSession::create(simpleMediaSession.getIoService(), rtpParameters, rtpReferenceClock, applicationParameters, bIsSender);
}

RtpSession::ptr SimpleMediaSessionFactory::doCreateGroupedRtpSession(SimpleMediaSession& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     RtpReferenceClock& rtpReferenceClock,
                                                                     const GenericParameters &applicationParameters,
                                                                     bool bIsSender)
{
  return RtpSession::create(simpleMediaSession.getIoService(), rtpParameters, rtpReferenceClock, applicationParameters, bIsSender);
}

std::unique_ptr<RtpSessionManager> SimpleMediaSessionFactory::doCreateSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                                      const RtpSessionParameters& rtpParameters,
                                                                                      const GenericParameters &applicationParameters)
{
  if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6190::H264_SVC)
  {
    sctp::SctpSvcRtpSessionManager* pRtpSessionManager =
      new sctp::SctpSvcRtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6184::H264)
  {
    sctp::SctpH264RtpSessionManager* pRtpSessionManager =
      new sctp::SctpH264RtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfchevc::H265)
  {
    sctp::SctpH265RtpSessionManager* pRtpSessionManager =
      new sctp::SctpH265RtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }

  // default
  return RtpSessionManager::create(simpleMediaSession.getIoService(), applicationParameters);
}

std::unique_ptr<GroupedRtpSessionManager> SimpleMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSession& simpleMediaSession,
                                                                                                   const RtpSessionParametersGroup_t& parameters,
                                                                                                   const GenericParameters &applicationParameters,
                                                                                                   bool bIsSender)
{
  // Subclasses can override this to create a grouped session manager that manages multiple RTP sessions, one for each object in the group
  return std::unique_ptr<GroupedRtpSessionManager>();
}

boost::shared_ptr<SimpleMediaSessionV2> SimpleMediaSessionFactory::doCreateMediaSession(boost::asio::io_service& ioService)
{
  return boost::make_shared<SimpleMediaSessionV2>(ioService);
}

RtpSession::ptr SimpleMediaSessionFactory::doCreateRtpSession(SimpleMediaSessionV2& simpleMediaSession,
                                                              const RtpSessionParameters& rtpParameters,
                                                              RtpReferenceClock& rtpReferenceClock,
                                                              const GenericParameters &applicationParameters,
                                                              bool bIsSender,
                                                              ExistingConnectionAdapter adapter)
{
  VLOG(10) << "SimpleMediaSessionFactory::doCreateRtpSession";
  return RtpSession::create(simpleMediaSession.getIoService(), adapter, rtpParameters, rtpReferenceClock, applicationParameters, bIsSender);
}

RtpSession::ptr SimpleMediaSessionFactory::doCreateGroupedRtpSession(SimpleMediaSessionV2& simpleMediaSession,
                                                                     const RtpSessionParameters& rtpParameters,
                                                                     RtpReferenceClock& rtpReferenceClock,
                                                                     const GenericParameters &applicationParameters,
                                                                     bool bIsSender,
                                                                     ExistingConnectionAdapter adapter)
{
  VLOG(10) << "SimpleMediaSessionFactory::doCreateGroupedRtpSession";
  return RtpSession::create(simpleMediaSession.getIoService(), adapter, rtpParameters, rtpReferenceClock, applicationParameters, bIsSender);
}

std::unique_ptr<RtpSessionManager> SimpleMediaSessionFactory::doCreateSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                                                      const RtpSessionParameters& rtpParameters,
                                                                                      const GenericParameters &applicationParameters)
{
  VLOG(10) << "SimpleMediaSessionFactory::doCreateSessionManager";
  if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6190::H264_SVC)
  {
    sctp::SctpSvcRtpSessionManager* pRtpSessionManager =
      new sctp::SctpSvcRtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfc6184::H264)
  {
    sctp::SctpH264RtpSessionManager* pRtpSessionManager =
      new sctp::SctpH264RtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }
  else if (rtpParameters.useRtpOverSctp() && rtpParameters.getEncodingName() == rfchevc::H265)
  {
    sctp::SctpH265RtpSessionManager* pRtpSessionManager =
      new sctp::SctpH265RtpSessionManager(simpleMediaSession.getIoService(), applicationParameters);

    return std::unique_ptr<RtpSessionManager>(pRtpSessionManager);
  }

  // default
  return RtpSessionManager::create(simpleMediaSession.getIoService(), applicationParameters);
}

std::unique_ptr<GroupedRtpSessionManager> SimpleMediaSessionFactory::doCreateGroupedSessionManager(SimpleMediaSessionV2& simpleMediaSession,
                                                                                                   const RtpSessionParametersGroup_t& parameters,
                                                                                                   const GenericParameters &applicationParameters,
                                                                                                   bool bIsSender)
{
  // Subclasses can override this to create a grouped session manager that manages multiple RTP sessions, one for each object in the group
  return std::unique_ptr<GroupedRtpSessionManager>();
}

std::unique_ptr<IRtpSessionManager> SimpleMediaSessionFactory::createSessionManager(const MediaSessionDescription& mediaDescription,
                                                                                    SimpleMediaSession& simpleMediaSession,
                                                                                    boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                                                    const GenericParameters &applicationParameters,
                                                                                    std::shared_ptr<ICooperativeCodec> pCooperative)
{
  if (index)
  {
    switch (index->second)
    {
      case MediaSessionDescription::GROUPED:
      {
        VLOG(10) << "Creating session manager for grouped RTP session";
        const RtpSessionParametersGroup_t& group = mediaDescription.getGroupedSessionParameters(index->first);
        std::unique_ptr<GroupedRtpSessionManager> pGroupedSessionManager
            = doCreateGroupedSessionManager(simpleMediaSession, group, applicationParameters,
                                            mediaDescription.isSender());

        // each RTP session in the group must be identified by a MID line
        // bind RTP session handlers to grouped session manager

        for (const RtpSessionParameters& rtpParameters : group)
        {
          RtpSession::ptr pRtpSession = doCreateGroupedRtpSession(simpleMediaSession, rtpParameters, pGroupedSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender());
          pGroupedSessionManager->addRtpSession(rtpParameters, pRtpSession);
        }

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pGroupedSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pGroupedSessionManager));
        break;
      }
      case MediaSessionDescription::NON_GROUPED:
      {
        VLOG(10) << "Creating session manager for non-grouped RTP session";
        const RtpSessionParameters& parameters = mediaDescription.getNonGroupedSessionParameters(index->first);
        // two stage construction: create RTP session manager
        std::unique_ptr<RtpSessionManager> pRtpSessionManager
            =  doCreateSessionManager(simpleMediaSession, parameters, applicationParameters);

        // two stage construction: create RTP session
        RtpSession::ptr pRtpSession = doCreateRtpSession(simpleMediaSession, parameters, pRtpSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender());
        pRtpSessionManager->setRtpSession(pRtpSession);

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pRtpSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pRtpSessionManager));
        break;
      }
      default:
      {
        // should not happen
        assert(false);
        // comp warning
        return std::unique_ptr<IRtpSessionManager>();
      }
    }
  }
  else
  {
    VLOG(2) << "Failed to create session manager for RTP session";
    return std::unique_ptr<IRtpSessionManager>();
  }
}

std::unique_ptr<IRtpSessionManager> SimpleMediaSessionFactory::createSessionManager(const MediaSessionDescription& mediaDescription,
                                                                                    SimpleMediaSessionV2& simpleMediaSession,
                                                                                    boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                                                    const GenericParameters &applicationParameters,
                                                                                    std::shared_ptr<ICooperativeCodec> pCooperative)
{
  if (index)
  {
    switch (index->second)
    {
      case MediaSessionDescription::GROUPED:
      {
        VLOG(10) << "Creating session manager for grouped RTP session";
        const RtpSessionParametersGroup_t& group = mediaDescription.getGroupedSessionParameters(index->first);
        std::unique_ptr<GroupedRtpSessionManager> pGroupedSessionManager
            = doCreateGroupedSessionManager(simpleMediaSession, group, applicationParameters,
                                            mediaDescription.isSender());

        pGroupedSessionManager->setCooperative(pCooperative);
        // each RTP session in the group must be identified by a MID line
        // bind RTP session handlers to grouped session manager

        for (const RtpSessionParameters& rtpParameters : group)
        {
          RtpSession::ptr pRtpSession = doCreateGroupedRtpSession(simpleMediaSession, rtpParameters, pGroupedSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender());
          pGroupedSessionManager->addRtpSession(rtpParameters, pRtpSession);
        }

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pGroupedSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pGroupedSessionManager));
        break;
      }
      case MediaSessionDescription::NON_GROUPED:
      {
        VLOG(10) << "Creating session manager for non-grouped RTP session";
        const RtpSessionParameters& parameters = mediaDescription.getNonGroupedSessionParameters(index->first);
        // two stage construction: create RTP session manager
        std::unique_ptr<RtpSessionManager> pRtpSessionManager
            =  doCreateSessionManager(simpleMediaSession, parameters, applicationParameters);

        pRtpSessionManager->setCooperative(pCooperative);

        // two stage construction: create RTP session
        RtpSession::ptr pRtpSession = doCreateRtpSession(simpleMediaSession, parameters, pRtpSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender());
        pRtpSessionManager->setRtpSession(pRtpSession);

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pRtpSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pRtpSessionManager));
        break;
      }
      default:
      {
        assert(false);
      }
    }
  }
  VLOG(2) << "Failed to create session manager for RTP session";
  return std::unique_ptr<IRtpSessionManager>();
}

std::unique_ptr<IRtpSessionManager> SimpleMediaSessionFactory::createSessionManager(const MediaSessionDescription& mediaDescription,
                                                                                    ExistingConnectionAdapter& adapter,
                                                                                    SimpleMediaSessionV2& simpleMediaSession,
                                                                                    boost::optional<MediaSessionDescription::MediaIndex_t> index,
                                                                                    const GenericParameters &applicationParameters,
                                                                                    std::shared_ptr<ICooperativeCodec> pCooperative)
{
  VLOG(10) << "SimpleMediaSessionFactory::createSessionManager";
  if (index)
  {
    switch (index->second)
    {
      case MediaSessionDescription::GROUPED:
      {
        VLOG(10) << "Creating session manager for grouped RTP session";
        const RtpSessionParametersGroup_t& group = mediaDescription.getGroupedSessionParameters(index->first);
        std::unique_ptr<GroupedRtpSessionManager> pGroupedSessionManager
            = doCreateGroupedSessionManager(simpleMediaSession, group, applicationParameters,
                                            mediaDescription.isSender());

        pGroupedSessionManager->setCooperative(pCooperative);

        // each RTP session in the group must be identified by a MID line
        // bind RTP session handlers to grouped session manager

        for (const RtpSessionParameters& rtpParameters : group)
        {
          RtpSession::ptr pRtpSession = doCreateGroupedRtpSession(simpleMediaSession, rtpParameters, pGroupedSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender(), adapter);
          pGroupedSessionManager->addRtpSession(rtpParameters, pRtpSession);
        }

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pGroupedSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pGroupedSessionManager));
        break;
      }
      case MediaSessionDescription::NON_GROUPED:
      {
        VLOG(10) << "Creating session manager for non-grouped RTP session";
        const RtpSessionParameters& parameters = mediaDescription.getNonGroupedSessionParameters(index->first);
        // two stage construction: create RTP session manager
        std::unique_ptr<RtpSessionManager> pRtpSessionManager
            =  doCreateSessionManager(simpleMediaSession, parameters, applicationParameters);

        pRtpSessionManager->setCooperative(pCooperative);

        // two stage construction: create RTP session
        RtpSession::ptr pRtpSession = doCreateRtpSession(simpleMediaSession, parameters, pRtpSessionManager->getRtpReferenceClock(), applicationParameters, mediaDescription.isSender(), adapter);
        pRtpSessionManager->setRtpSession(pRtpSession);

        boost::optional<bool> bUseMediaSampleTime = applicationParameters.getBoolParameter(ApplicationParameters::use_media_sample_time);
        if (bUseMediaSampleTime && *bUseMediaSampleTime)
        {
          VLOG(5) << "Using media sample timestamp mode";
          pRtpSessionManager->getRtpReferenceClock().setMode(RtpReferenceClock::RC_MEDIA_SAMPLE_TIME);
        }

        return std::unique_ptr<IRtpSessionManager>(std::move(pRtpSessionManager));
        break;
      }
      default:
      {
        assert(false);
      }
    }
  }
  VLOG(2) << "Failed to create session manager for RTP session or no such session";
  return std::unique_ptr<IRtpSessionManager>();
}

} // rtp_plus_plus
