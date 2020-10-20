#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceController.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/media/IMediaSampleSource.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/network/EndPoint.h>

namespace rtp_plus_plus {
namespace mprtp {

class MpRtpTranslator : public ServiceController
{
public:
  static boost::shared_ptr<MpRtpTranslator> create(const GenericParameters &applicationParameters,
                                                   const rfc4566::SessionDescription& sdp1, const rfc4566::SessionDescription& sdp2,
                                                   const InterfaceDescriptions_t& netConf1, const InterfaceDescriptions_t& netConf2);


  MpRtpTranslator(const GenericParameters &applicationParameters,
                  const rfc4566::SessionDescription& sdp1, const rfc4566::SessionDescription& sdp2,
                  const InterfaceDescriptions_t& netConf1, const InterfaceDescriptions_t& netConf2);

protected:
  /**
   * @fn  virtual boost::system::error_code MpRtpTranslator::doStart()
   *
   * @brief Starts all media sessions managed by this component
   *
   * @return  .
   */
  virtual boost::system::error_code doStart();
  /**
   * @fn  virtual boost::system::error_code MpRtpTranslator::doStop()
   *
   * @brief Stops all media sessions managed by this component
   *
   * @return  .
   */
  virtual boost::system::error_code doStop();

private:

  void handleIncomingAudioRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  void handleIncomingVideoRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  void handleIncomingAudioMpRtp(const RtpPacket& rtpPacket, const EndPoint& ep);
  void handleIncomingVideoMpRtp(const RtpPacket& rtpPacket, const EndPoint& ep);

  boost::shared_ptr<boost::thread> m_pMpRtpThread;
  boost::shared_ptr<boost::thread> m_pRtpThread;
  boost::shared_ptr<SimpleMediaSession> m_pMpRtpMediaSession;
  boost::shared_ptr<SimpleMediaSession> m_pRtpMediaSession;
  // flags for setting up session info
  bool m_bFirstMpRtpAudio;
  bool m_bFirstMpRtpVideo;
  bool m_bFirstRtpAudio;
  bool m_bFirstRtpVideo;
};

} // mprtp
} // rtp_plus_plus
