//#include "CorePch.h"
//#include <rtp++/rfc3550/MultiLayerRtpSession.h>

//namespace rfc3550
//{

//MultiLayerRtpSession::MultiLayerRtpSession(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters)
//  : rfc3550::RtpSession(rIoService, rtpParameters, applicationParameters)
//{

//}

//MultiLayerRtpSession::~MultiLayerRtpSession()
//{

//}

//boost::system::error_code MultiLayerRtpSession::doStart()
//{
//  return RtpSession::doStart();
//}

//boost::system::error_code MultiLayerRtpSession::doStop()
//{
//  return RtpSession::doStop();
//}

//void MultiLayerRtpSession::doSendSample(MediaSample::ptr pSample)
//{
//  // get end point list
//  assert( m_parameters.getRemoteEndPointCount() > 0);

//  std::vector<RtpPacket> rtpPackets = generateRtpPackets(pSample);

//  // select path for each packet
//  std::for_each(rtpPackets.begin(), rtpPackets.end(), [this](const RtpPacket& rtpPacket)
//  {
//    std::pair<uint32_t, uint32_t> indexPair = getSelectedFlow(rtpPacket);
//    sendPacket(rtpPacket, indexPair);
//  });
//}

//bool MultiLayerRtpSession::sampleAvailable() const
//{
//  return RtpSession::sampleAvailable();
//}

//MediaSample::ptr MultiLayerRtpSession::getNextSample(uint8_t& uiFormat)
//{
//  return RtpSession::getNextSample(uiFormat);
//}

//void MultiLayerRtpSession::sendPacket(const RtpPacket& rtpPacket, std::pair<uint32_t, uint32_t> indexPair)
//{
//  m_vRtpInterfaces[indexPair.first]->send(rtpPacket, m_parameters.getRemoteEndPoint(indexPair.second).first);
//}

//std::vector<RtpNetworkInterface::ptr> MultiLayerRtpSession::createRtpNetworkInterfaces(const RtpSessionParameters &rtpParameters)
//{
//  return createRtpNetworkInterfacesAndMapping(rtpParameters);
//}

//std::vector<RtpPlayoutBuffer::ptr> MultiLayerRtpSession::createRtpPlayoutBuffer(const RtpSessionParameters &rtpParameters)
//{
//  return createLayerDeinterleavingBuffers(rtpParameters);
//}

//}
