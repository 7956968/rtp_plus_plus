#pragma once
#include <map>
#include <memory>
#include <rtp++/RtpSession.h>
#include <rtp++/rto/VirtualRtpSessionBase.h>

namespace rtp_plus_plus
{
namespace rto
{

class VirtualRtpSession : public RtpSession,
                          public VirtualRtpSessionBase
{
public:
  typedef boost::shared_ptr<VirtualRtpSession> ptr;

  static ptr create(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters, RtpReferenceClock& rtpReferenceClock, const GenericParameters& parameters, bool bIsSender, bool bDisableRtcp);
  VirtualRtpSession(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters, RtpReferenceClock& rtpReferenceClock, const GenericParameters& parameters, bool bIsSender, bool bDisableRtcp);

  virtual bool sampleAvailable() const { return false; }
  // virtual MediaSample::ptr getNextSample(uint8_t& uiFormat) { return MediaSample::ptr(); }

  // Simple forwarder
  void receiveRtpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to);
  void receiveRtcpPacket(NetworkPacket networkPacket, const EndPoint& from, const EndPoint& to);

protected:
  virtual std::vector<RtpNetworkInterface::ptr> createRtpNetworkInterfaces(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  virtual std::vector<rfc3550::RtcpReportManager::ptr> createRtcpReportManagers(const RtpSessionParameters &rtpParameters, const GenericParameters& parameters);


protected:
  bool m_bDisableRtcp;
  // map to lookup RTP interface using EndPoint
  std::map<std::string, uint32_t> m_mEndPointInterface;
};

} // rto
} // rtp_plus_plus

