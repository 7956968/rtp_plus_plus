#pragma once
#include <thread>
#include <boost/asio/io_service.hpp>
#include <cpputil/ServiceManager.h>
#include <rtp++/media/VideoInputSource.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <PeerConnectionApi/peer_connection_api.grpc.pb.h>

#include "RtcConfig.h"


namespace rtp_plus_plus {

// fwd
class IVideoModel;
class IDataModel;

/**
 * @brief The RtcServiceImpl class
 */
class RtcServiceImpl final : public peer_connection::PeerConnectionApi::Service
{
public:
  /**
   * @brief RtcServiceImpl
   * @param rtcConfig
   */
  RtcServiceImpl(const RtcConfig& rtcConfig);
  /**
   * @brief RtcServiceImpl
   *
   */
  ~RtcServiceImpl();
  /**
   * @brief createOffer
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status createOffer(::grpc::ServerContext* context,
                                     const ::peer_connection::OfferDescriptor* request,
                                     ::peer_connection::CreateSessionDescriptionResponse* response);
  /**
   * @brief createAnswer
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status createAnswer(::grpc::ServerContext* context,
                                      const ::peer_connection::AnswerDescriptor* request,
                                      ::peer_connection::CreateSessionDescriptionResponse* response);
  /**
   * @brief setRemoteDescription
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status setRemoteDescription(::grpc::ServerContext* context,
                                              const ::peer_connection::SessionDescription* request,
                                              ::peer_connection::SetSessionDescriptionResponse* response);
  /**
   * @brief startStreaming
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status startStreaming(::grpc::ServerContext* context,
                                        const ::peer_connection::StartStreamingRequest* request,
                                        ::peer_connection::StartStreamingResponse* response);
  /**
   * @brief stopStreaming
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status stopStreaming(::grpc::ServerContext* context,
                                       const ::peer_connection::StopStreamingRequest* request,
                                       ::peer_connection::StopStreamingResponse* response);
  /**
   * @brief shutdown
   * @param context
   * @param request
   * @param response
   * @return
   */
  virtual ::grpc::Status shutdown(::grpc::ServerContext* context,
                                  const ::peer_connection::ShutdownRequest* request,
                                  ::peer_connection::ShutdownResponse* response);

private:
  /**
   * @brief updateLocalConnectionDataInCaseOfNat
   * @param localSdp
   */
  void updateLocalConnectionDataInCaseOfNat(rfc4566::SessionDescription& localSdp);
  /**
   * @brief doStartStreaming
   */
  void doStartStreaming();
  /**
   * @brief handleMediaSourceEofV2 handles end of local media source
   */
  void handleMediaSourceEofV2(const boost::system::error_code& ec);
  /**
   * @brief onVideoReceived
   */
  void onVideoReceived();

private:
  RtcConfig m_rtcConfig;
  GenericParameters m_applicationParameters;
  rfc3261::SipContext m_sipContext;

  ServiceManager m_serviceManager;
  std::unique_ptr<boost::asio::io_service::work> m_pWork;

  MediaSessionNetworkManager m_mediaSessionNetworkManager;
  std::unique_ptr<std::thread> m_pThread;

  boost::optional<rfc4566::SessionDescription> m_localDescription;
  boost::optional<rfc4566::SessionDescription> m_remoteDescription;

  boost::shared_ptr<rtp_plus_plus::SimpleMediaSessionV2> m_pMediaSession;
  // when using video model
  IVideoModel* m_pModel;
  IDataModel* m_pDataModel;  
  std::shared_ptr<VideoInputSource> m_pVideoSource;
  // when using file video data or live encoder
  std::shared_ptr<media::VirtualVideoDeviceV2> m_pVideoDevice;

#if 0
  boost::asio::io_service m_ioService;
  std::unique_ptr<boost::asio::io_service::work> m_pWork;
  std::unique_ptr<boost::thread> m_pThread;
#endif
};

} // rtp_plus_plus
