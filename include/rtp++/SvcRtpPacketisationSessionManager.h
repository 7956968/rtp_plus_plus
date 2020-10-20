#pragma once
#include <rtp++/GroupedRtpSessionManager.h>
#include <rtp++/RtpTime.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>

namespace rtp_plus_plus
{

/**
 * @brief The SvcRtpPacketisationSessionManager class is used to output RTP packetisation info
 */
class SvcRtpPacketisationSessionManager : public GroupedRtpSessionManager
{
public:
  static std::unique_ptr<SvcRtpPacketisationSessionManager> create(boost::asio::io_service& ioService,
                                                                const GenericParameters& applicationParameters);

  SvcRtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                 const GenericParameters& applicationParameters);

  ~SvcRtpPacketisationSessionManager();

  virtual void doSend(const media::MediaSample& mediaSample);
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples);

  virtual void doSend(const media::MediaSample& mediaSample, const std::string& sMid);
  virtual void doSend(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid);

  virtual bool doIsSampleAvailable() const
  {
    return false;
  }

  virtual bool doIsSampleAvailable(const std::string& sMid) const
  {
    return false;
  }

  virtual std::vector<media::MediaSample> doGetNextSample()
  {
    return std::vector<media::MediaSample>();
  }

  virtual std::vector<media::MediaSample> doGetNextSample(const std::string& sMid)
  {
    return std::vector<media::MediaSample>();
  }

  virtual void doHandleIncomingRtp(const RtpPacket& rtpPacket, const EndPoint& ep,
                                   bool bSSRCValidated, bool bRtcpSynchronised,
                                   const boost::posix_time::ptime& tPresentation,
                                   const std::string& sMid)
  {

  }

  virtual void doHandleIncomingRtcp(const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep,
                                    const std::string& sMid)
  {

  }

  virtual void onSessionsStarted()
  {

  }

  virtual void onSessionsStopped()
  {

  }

private:

  bool m_bVerbose;
  boost::posix_time::ptime m_tLastAu;

  uint32_t m_uiAccessUnitIndex;
  boost::mutex m_lock;

  uint16_t m_uiLastRtpSN;
};

}
