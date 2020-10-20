#pragma once
#include <rtp++/RtpSessionManager.h>
#include <rtp++/sctp/SctpH264RtpPolicyManager.h>
#include <rtp++/sctp/SctpRtpPolicy.h>

namespace rtp_plus_plus
{
namespace sctp
{

class SctpH264RtpSessionManager : public RtpSessionManager
{
public:

  static std::unique_ptr<SctpH264RtpSessionManager> create(boost::asio::io_service& ioService,
                                                          const GenericParameters& applicationParameters);

  SctpH264RtpSessionManager(boost::asio::io_service& ioService,
                           const GenericParameters& applicationParameters);

  void setSctpPolicies(const std::vector<std::string>& vPolicies );

  void send(const media::MediaSample& mediaSample);
  void send(const std::vector<media::MediaSample>& vMediaSamples);

  /**
   * @brief onMemberUpdate This method is called when member updates occur
   * @param memberUpdate The updated session information
   */
  virtual void handleMemberUpdate(const MemberUpdate& memberUpdate);

protected:
  virtual void onSessionStarted();
  virtual void onSessionStopped();

protected:
  SctpRtpPolicy m_sctpPolicy;
  SctpH264RtpPolicyManager m_sctpPolicyManager;
};

}
}
