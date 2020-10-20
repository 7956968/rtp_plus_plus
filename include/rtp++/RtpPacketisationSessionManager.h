#pragma once
#include <rtp++/RtpSessionManager.h>
#include <rtp++/RtpTime.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>

namespace rtp_plus_plus
{

/**
 * @brief The RtpPacketisationSessionManager class is used to output RTP packetisation info
 */
class RtpPacketisationSessionManager : public RtpSessionManager
{
public:
  static std::unique_ptr<RtpPacketisationSessionManager> create(boost::asio::io_service& ioService,
                                                                const GenericParameters& applicationParameters);

  RtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                 const GenericParameters& applicationParameters);

  ~RtpPacketisationSessionManager();

  virtual void send(const media::MediaSample& mediaSample);
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples);

private:

  bool m_bVerbose;
  boost::posix_time::ptime m_tLastAu;
  uint32_t m_uiLastRtp;

  uint32_t m_uiAccessUnitIndex;
  boost::mutex m_lock;

  uint16_t m_uiLastRtpSN;
};

}
