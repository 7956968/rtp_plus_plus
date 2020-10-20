#pragma once
#include <boost/function.hpp>
#include <rtp++/RtpSessionManager.h>

namespace rtp_plus_plus
{

typedef boost::function<void (const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                              bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation)> RtpForward_t;

class ForwardIncomingRtpSessionManager : public RtpSessionManager
{
public:
  static std::unique_ptr<ForwardIncomingRtpSessionManager> create(boost::asio::io_service& ioService,
                                                   const GenericParameters& applicationParameters)
  {
    return std::unique_ptr<ForwardIncomingRtpSessionManager>( new ForwardIncomingRtpSessionManager(ioService, applicationParameters) );
  }

  ForwardIncomingRtpSessionManager(boost::asio::io_service& ioService,
                    const GenericParameters& applicationParameters)
    : RtpSessionManager(ioService, applicationParameters)
  {

  }

  void setRtpForward(RtpForward_t rtpForward){ m_forward = rtpForward; }

private:
  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep, bool bSSRCValidated,
                         bool bRtcpSynchronised, const boost::posix_time::ptime& tPresentation)
  {
    if (m_forward)
    {
      m_forward(rtpPacket, ep , bSSRCValidated, bRtcpSynchronised, tPresentation);
    }
    else
    {
      LOG_FIRST_N(WARNING, 1) << "Forwarding function has not been set";
    }
  }

  RtpForward_t m_forward;
};

} // rtp_plus_plus
