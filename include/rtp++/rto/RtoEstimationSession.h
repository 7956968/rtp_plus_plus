#pragma once
#include <memory>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/rfc3550/SdesInformation.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rto/Channel.h>

namespace rtp_plus_plus
{
namespace rto
{

/// @def DEBUG_RTP Debug information about incoming/outgoing RTP/RTCP
// #define DEBUG_RTP

/// @def DEBUG_DONT_CREATE_SENDER Don't create Rtp sender session.
// #define DEBUG_DONT_CREATE_SENDER

/// @def DEBUG_DONT_CREATE_RECEIVER Don't create Rtp receiver session.
// #define DEBUG_DONT_CREATE_RECEIVER

class RtoEstimationSession
{
public:
  RtoEstimationSession(rfc4566::SessionDescription& sdp, const InterfaceDescriptions_t& addresses,
                       const std::string& sChannelConfigFile, 
                       const GenericParameters &applicationParameters);

  boost::shared_ptr<SimpleMediaSession> getSenderRtpSession() const { return m_pRtpSenderSession; }

  void onSendRtp(Buffer rtpPacket, const EndPoint& from, const EndPoint& to);
  void onSendRtcp(Buffer rtcpPacket, const EndPoint& from, const EndPoint& to);

  void start();
  void stop();

private:

  boost::shared_ptr<SimpleMediaSession> createVirtualRtpSession(const RtpSessionParameters& rtpParameters, const GenericParameters &parameters);

  bool initiliseChannel(const std::string& sChannelConfig, const RtpSessionParameters& rtpParameters, const RtpSessionParameters& rtpReceiverParameters);

  bool checkChannelValidity( const EndPoint& from, const EndPoint& to);

  NetworkPacket createVirtualNetworkPacket(Buffer packet);

  // Channels are demultiplexed by src endpoint, and then by destination end point
  std::map<std::string, std::map<std::string, std::unique_ptr<Channel> > > m_mChannelMap;

  boost::shared_ptr<SimpleMediaSession> m_pRtpSenderSession;
  boost::shared_ptr<SimpleMediaSession> m_pRtpReceiverSession;
  // HACK: adding boost asio service here for now: need to abstract it out at a later stage
  boost::asio::io_service m_ioService;
  boost::shared_ptr<boost::asio::io_service::work> m_pWork;
  boost::shared_ptr<boost::thread> m_pThread;
  boost::shared_ptr<boost::thread> m_pSenderThread;
  boost::shared_ptr<boost::thread> m_pReceiverThread;
};

} // rto
} // rtp_plus_plus
