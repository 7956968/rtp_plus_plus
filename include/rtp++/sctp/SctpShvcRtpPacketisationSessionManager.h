#pragma once
#include <rtp++/RtpSessionManager.h>
#include <rtp++/RtpTime.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/sctp/SctpH265RtpPolicyManager.h>
#include <rtp++/sctp/SctpRtpPolicy.h>

namespace rtp_plus_plus
{
namespace sctp
{

/**
 * @brief The SctpSvcRtpPacketisationSessionManager class is used to output RTP packetisation info
 */
class SctpShvcRtpPacketisationSessionManager : public RtpSessionManager
{
public:
  static std::unique_ptr<SctpShvcRtpPacketisationSessionManager> create(boost::asio::io_service& ioService,
                                                                const GenericParameters& applicationParameters);

  SctpShvcRtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                 const GenericParameters& applicationParameters);

  ~SctpShvcRtpPacketisationSessionManager();

  void setSctpPolicies(const std::vector<std::string>& vPolicies );

  virtual void send(const media::MediaSample& mediaSample);
  virtual void send(const std::vector<media::MediaSample>& vMediaSamples);

private:

  bool m_bVerbose;
  uint32_t m_uiAccessUnitIndex;
  boost::mutex m_lock;

  SctpRtpPolicy m_sctpPolicy;
  SctpH265RtpPolicyManager m_sctpPolicyManager;

  uint16_t m_uiLastRtpSN;
};

}
}
