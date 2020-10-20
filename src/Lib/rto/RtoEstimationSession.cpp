#include "CorePch.h"
#include <rtp++/rto/RtoEstimationSession.h>
#include <stdexcept>
#include <functional>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/RtpSessionManager.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
#include <rtp++/mediasession/VirtualMediaSessionFactory.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/rto/Channel.h>
#include <rtp++/rfc3550/SdesInformation.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/rto/VirtualRtpSession.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/util/TracesUtil.h>

namespace rtp_plus_plus
{
namespace rto
{

RtoEstimationSession::RtoEstimationSession(rfc4566::SessionDescription& sdp, const InterfaceDescriptions_t& addresses,
                                           const std::string& sChannelConfigFile,
                                           const GenericParameters& applicationParameters)
  :m_pWork(new boost::asio::io_service::work(m_ioService))
{  
  // HACK: just look at first RTP session parameter object
  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::SdesInformation("RTO Estimation Sender CNAME"), sdp, addresses, true);
  const RtpSessionParameters& rtpParameters = mediaSessionDescription.getNonGroupedSessionParameters(0);

  VirtualMediaSessionFactory factory;

#ifndef DEBUG_DONT_CREATE_SENDER
  m_pRtpSenderSession = factory.create(mediaSessionDescription, applicationParameters);
  //m_pRtpSenderSession = createVirtualRtpSession(rtpParameters, applicationParameters);
#endif

  MediaSessionDescription receiverMediaSessionDescription = MediaSessionDescription(rfc3550::SdesInformation("RTO Estimation Sender CNAME"), sdp, addresses, false);
  const RtpSessionParameters& rtpReceiverParameters = receiverMediaSessionDescription.getNonGroupedSessionParameters(0);

#ifndef DEBUG_DONT_CREATE_RECEIVER
  m_pRtpReceiverSession = factory.create(receiverMediaSessionDescription, applicationParameters);
  //m_pRtpReceiverSession = createVirtualRtpSession(rtpReceiverParameters, applicationParameters);
#endif

  // we might not want to create one of the sessions during debugging
  if (m_pRtpSenderSession && m_pRtpReceiverSession)
  {
    // create channel between them
    if (!initiliseChannel(sChannelConfigFile, rtpParameters, rtpReceiverParameters))
      BOOST_THROW_EXCEPTION(std::runtime_error("Failed to parse channel configuration file"));
  }

}

NetworkPacket RtoEstimationSession::createVirtualNetworkPacket(Buffer packet)
{
  NetworkPacket networkPacket;
  // Make copy
  uint8_t* pData = new uint8_t[packet.getSize()];
  memcpy((void*)pData, (const char*)packet.data(), packet.getSize());
  networkPacket.setData(pData, packet.getSize());
  return networkPacket;
}

void RtoEstimationSession::onSendRtp(Buffer rtpPacket, const EndPoint& from, const EndPoint& to)
{
#ifdef DEBUG_RTP
  VLOG(20) << "Sending RTP from " << from.getAddress() << ":" << from.getPort()
           << " to " << to.getAddress() << ":" << to.getPort();
#endif

  // create virtual network packet
  NetworkPacket networkPacket = createVirtualNetworkPacket(rtpPacket);

#ifdef DEBUG_DONT_CREATE_SENDER
  return;
#endif
#ifdef DEBUG_DONT_CREATE_RECEIVER
  return;
#endif

  if (!checkChannelValidity(from, to))
    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid channel configuration"));

  // find channel
  std::unique_ptr<Channel>& pChannel = m_mChannelMap[from.toString()][to.toString()];
  pChannel->sendRtpPacketOverChannel(networkPacket, from, to);
}

bool RtoEstimationSession::checkChannelValidity( const EndPoint& from, const EndPoint& to)
{
  auto it = m_mChannelMap.find(from.toString());
  if (it == m_mChannelMap.end())
  {
    LOG(WARNING) << "Failed to locate \"from\" channel from " << from << " to " << to;
    return false;
  }

  auto it2 = m_mChannelMap[from.toString()].find(to.toString());
  bool bRes = (it2 != m_mChannelMap[from.toString()].end());
  if (!bRes)
  {
    LOG(WARNING) << "Failed to locate \"to\" channel from " << from << " to " << to;
  }
  return bRes;
}

void RtoEstimationSession::onSendRtcp(Buffer rtcpPacket, const EndPoint& from, const EndPoint& to)
{
#ifdef DEBUG_RTP
  VLOG(20) << "Sending RTCP from " << from.getAddress() << ":" << from.getPort()
          << " to " << to.getAddress() << ":" << to.getPort();
#endif

#ifdef DEBUG_DONT_CREATE_SENDER
  return;
#endif
#ifdef DEBUG_DONT_CREATE_RECEIVER
  return;
#endif

  // create virtual network packet
  NetworkPacket networkPacket = createVirtualNetworkPacket(rtcpPacket);

  if (!checkChannelValidity(from, to))
    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid channel configuration")); 
  // find channel

  std::unique_ptr<Channel>& pChannel = m_mChannelMap[from.toString()][to.toString()];
  pChannel->sendRtcpPacketOverChannel(networkPacket, from, to);
}

void RtoEstimationSession::start()
{
  if (m_pRtpSenderSession)
  {
    m_pSenderThread = boost::make_shared<boost::thread>(boost::bind(&SimpleMediaSession::start, boost::ref(m_pRtpSenderSession)));
    // m_pRtpSenderSession->start();
  }
  if (m_pRtpReceiverSession)
  {
    m_pReceiverThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&SimpleMediaSession::start, boost::ref(m_pRtpReceiverSession))));
    // m_pRtpReceiverSession->start();
  }

  m_ioService.reset();
  m_pThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, boost::ref(m_ioService))));
}

void RtoEstimationSession::stop()
{
  LOG(INFO) << "Stopping IO service";
  m_pWork.reset();
  LOG(INFO) << "Stopping sender RTP session";
  if (m_pRtpSenderSession) m_pRtpSenderSession->stop();
  LOG(INFO) << "Stopping receiver RTP session";
  if (m_pRtpReceiverSession) m_pRtpReceiverSession->stop();
  LOG(INFO) << "Stopping virtual channels";
  // stop all channels
  for (auto it = m_mChannelMap.begin(); it != m_mChannelMap.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      it2->second->stop();
    }
  }
  LOG(INFO) << "Joining thread";
  m_pThread->join();
  LOG(INFO) << "stop complete";
}

bool RtoEstimationSession::initiliseChannel(const std::string& sChannelConfig, const RtpSessionParameters& rtpParameters, const RtpSessionParameters& rtpReceiverParameters)
{
  boost::filesystem::path source(sChannelConfig);
  // file source: make sure file exists
  if (!source.is_absolute())
    source = boost::filesystem::current_path() / sChannelConfig;
  if (!boost::filesystem::exists(source))
  {
    LOG(ERROR) << "Channel config " << sChannelConfig << " does not exist";
    return false;
  }
  else
  {
    LOG(INFO) << "Using file source: " << sChannelConfig;
    std::ifstream if1(sChannelConfig.c_str(), std::ifstream::in );
    // the format of the channel configuration file is the following
    // src-interface-index dst-interface-index loss-probability
    // Note: this index is 1-based
    std::string sLine;
    getline(if1, sLine);
    while (if1.good())
    {
      std::istringstream istr(sLine);
      int32_t  uiSrcIndex = -1, uiDstIndex = -1, uiLossProbability = -1;
      std::string sDataFile;
      istr >> uiSrcIndex >> uiDstIndex >> uiLossProbability >> sDataFile;
      const EndPointPair_t& localEps = rtpParameters.getLocalEndPoint(uiSrcIndex - 1);
      const EndPointPair_t& remoteEps = rtpParameters.getRemoteEndPoint(uiDstIndex - 1);

      std::vector<double> vDelays;
      bool bSuccess = TracesUtil::loadOneWayDelayDataFile(sDataFile, vDelays);
      if (!bSuccess) return false;

      // add forward channels

      // HACK based on there being only 1 session
      RtpSession::ptr pRtpReceiverSession;
      if (m_pRtpReceiverSession->hasAudio())
      {
        std::unique_ptr<IRtpSessionManager>& pAudioManager = m_pRtpReceiverSession->getAudioSessionManager();
        RtpSessionManager* pRtpSessionManager = dynamic_cast<RtpSessionManager*>(pAudioManager.get());
        assert(pRtpSessionManager);
        pRtpReceiverSession = pRtpSessionManager->getRtpSession();
      }
      else
      {
        assert(m_pRtpReceiverSession->hasVideo());
        std::unique_ptr<IRtpSessionManager>& pVideoManager = m_pRtpReceiverSession->getVideoSessionManager();
        RtpSessionManager* pRtpSessionManager = dynamic_cast<RtpSessionManager*>(pVideoManager.get());
        assert(pRtpSessionManager);
        pRtpReceiverSession = pRtpSessionManager->getRtpSession();
      }

      VirtualRtpSessionBase::ptr pVirtualRtpSession = boost::dynamic_pointer_cast<VirtualRtpSessionBase>(pRtpReceiverSession);
      assert (pVirtualRtpSession);

      // set all callbacks
      pVirtualRtpSession->setRtpCallback(boost::bind(&RtoEstimationSession::onSendRtp, this, _1, _2, _3));
      pVirtualRtpSession->setRtcpCallback(boost::bind(&RtoEstimationSession::onSendRtcp, this, _1, _2, _3));
      rto::ReceiveCb_t fnOnReceiveRtp = std::bind(&VirtualRtpSessionBase::receiveRtpPacket, pVirtualRtpSession, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      rto::ReceiveCb_t fnOnReceiveRtcp = std::bind(&VirtualRtpSessionBase::receiveRtcpPacket, pVirtualRtpSession, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

      LOG(INFO) << "Creating virtual channel from " << localEps.first << " to " << remoteEps.first;
      m_mChannelMap[localEps.first.toString()][remoteEps.first.toString()] = std::unique_ptr<Channel>(new Channel(uiLossProbability, fnOnReceiveRtp, fnOnReceiveRtcp, m_ioService));
      LOG(INFO) << "Creating virtual channel from " << localEps.second << " to " << remoteEps.second;
      m_mChannelMap[localEps.second.toString()][remoteEps.second.toString()] = std::unique_ptr<Channel>(new Channel(uiLossProbability, fnOnReceiveRtp, fnOnReceiveRtcp, m_ioService));
      m_mChannelMap[localEps.first.toString()][remoteEps.first.toString()]->setChannelDelays(vDelays);
      m_mChannelMap[localEps.second.toString()][remoteEps.second.toString()]->setChannelDelays(vDelays);

      // add reverse channels
      // TODO2: add some kind of shared wrapper around vector so it can be shared

      // HACK based on there being only 1 session
      RtpSession::ptr pRtpSenderSession;
      if (m_pRtpSenderSession->hasAudio())
      {
        std::unique_ptr<IRtpSessionManager>& pAudioManager = m_pRtpSenderSession->getAudioSessionManager();
        RtpSessionManager* pRtpSessionManager = dynamic_cast<RtpSessionManager*>(pAudioManager.get());
        assert(pRtpSessionManager);
        pRtpSenderSession = pRtpSessionManager->getRtpSession();
      }
      else
      {
        assert(m_pRtpSenderSession->hasVideo());
        std::unique_ptr<IRtpSessionManager>& pVideoManager = m_pRtpSenderSession->getVideoSessionManager();
        RtpSessionManager* pRtpSessionManager = dynamic_cast<RtpSessionManager*>(pVideoManager.get());
        assert(pRtpSessionManager);
        pRtpSenderSession = pRtpSessionManager->getRtpSession();
      }

      VirtualRtpSessionBase::ptr pVirtualSenderRtpSession = boost::dynamic_pointer_cast<VirtualRtpSessionBase>(pRtpSenderSession);
      assert (pVirtualSenderRtpSession);

      pVirtualSenderRtpSession->setRtpCallback(boost::bind(&RtoEstimationSession::onSendRtp, this, _1, _2, _3));
      pVirtualSenderRtpSession->setRtcpCallback(boost::bind(&RtoEstimationSession::onSendRtcp, this, _1, _2, _3));
      rto::ReceiveCb_t fnOnReceiveRtpSender = std::bind(&VirtualRtpSessionBase::receiveRtpPacket, pVirtualSenderRtpSession, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      rto::ReceiveCb_t fnOnReceiveRtcpSender = std::bind(&VirtualRtpSessionBase::receiveRtcpPacket, pVirtualSenderRtpSession, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            LOG(INFO) << "Creating virtual channel from " << remoteEps.first << " to " << localEps.first;
      m_mChannelMap[remoteEps.first.toString()][localEps.first.toString()] = std::unique_ptr<Channel>(new Channel(uiLossProbability, fnOnReceiveRtpSender, fnOnReceiveRtcpSender, m_ioService));
      LOG(INFO) << "Creating virtual channel from " << remoteEps.second << " to " << localEps.second;
      m_mChannelMap[remoteEps.second.toString()][localEps.second.toString()] = std::unique_ptr<Channel>(new Channel(uiLossProbability, fnOnReceiveRtpSender, fnOnReceiveRtcpSender, m_ioService));
      m_mChannelMap[remoteEps.first.toString()][localEps.first.toString()]->setChannelDelays(vDelays);
      m_mChannelMap[remoteEps.second.toString()][localEps.second.toString()]->setChannelDelays(vDelays);

      getline(if1, sLine);
    }
    return true;
  }
}

} // rto
} // rtp_plus_plus
