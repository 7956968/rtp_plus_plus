#pragma once
#include <map>
#include <string>
#include "DllSetup.h"
#include <cpputil/ServiceManager.h>
#include <rtp++/rfc2326/RtspClientSession.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipAgent.h>

namespace rtp_plus_plus
{

/**
 * @brief Used for sharing state between Windows DirectShow applications and filters.
 * The service manager provides the io_service.
 *
 * @TODO: rename to be more generic
 */
class SIP_SESSION_STATE_API SipSessionState
{
public:
	/**
   * @brief Named constructor.  Use this method to retrieve the SipSessionState singleton
   */
  static SipSessionState& getInstance()
	{
    static SipSessionState instance;
		return instance;
	}
	/**
   * @brief Destructor
   */
  ~SipSessionState();
  /**
   * @brief Getter for service manager
   */
  ServiceManager& getServiceManager() { return m_serviceManager; }
  /**
   * @brief initialise context
   */
  boost::system::error_code initialise(const GenericParameters &applicationParameters,
                                       const rfc3261::SipContext& context,
                                       std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswer);
  /**
   * @brief initialises RTSP context
   */
  boost::system::error_code initialiseRtsp(const GenericParameters& applicationParameters,
    const rfc2326::RtspUri& rtspUri,
    const std::vector<std::string>& vLocalInterfaces = std::vector<std::string>());
  /**
   * @brief Getter for if state has been initialised successfully
   */
  bool isIntialised() { return m_pSipAgent != 0; }
  /**
   * @brief Getter for if state has been initialised successfully
   */
  bool isRtspIntialised() { return m_pRtspClientSession != 0; }
  /**
   * @brief Getter for SIP agent. Should only be called if state has been initialised successfully
   */
  rfc3261::SipAgent& getSipAgent() { return *(m_pSipAgent.get()); };
  /**
   * @brief Getter for RTSP client session. Should only be called if state has been initialised successfully
   */
  rfc2326::RtspClientSession& getRtspClientSession() { return *(m_pRtspClientSession.get()); };
  /**
   * @ brief Setter for video device
   */
  void setVideoDevice(std::shared_ptr<media::IVideoDevice> pVideoDevice)
  {
    if (m_pSipAgent)
      m_pSipAgent->setVideoDevice(pVideoDevice);
  }
  /**
   * @ brief Setter for audio device
   */
  void setAudioDevice(std::shared_ptr<media::IVideoDevice> pAudioDevice)
  {
    if (m_pSipAgent)
      m_pSipAgent->setAudioDevice(pAudioDevice);
  }

protected:
  SipSessionState();
	// Prohibit copying
  SipSessionState(const SipSessionState& rLogger);
  SipSessionState operator=(const SipSessionState& rLogger);

private:
  ServiceManager m_serviceManager;
  std::unique_ptr<rfc3261::SipAgent> m_pSipAgent;
  std::unique_ptr<rfc2326::RtspClientSession> m_pRtspClientSession;
};

} // rtp_plus_plus
