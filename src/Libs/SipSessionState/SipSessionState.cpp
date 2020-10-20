#include "stdafx.h"
#include "SipSessionState.h"

namespace rtp_plus_plus
{

SipSessionState::SipSessionState()
{
}

SipSessionState::~SipSessionState()
{
}

boost::system::error_code SipSessionState::initialise(const GenericParameters &applicationParameters,
                                                      const rfc3261::SipContext& context,
                                                      std::shared_ptr<rfc3264::OfferAnswerModel> pOfferAnswer)
{
  boost::system::error_code ec;
  m_pSipAgent = std::unique_ptr<rfc3261::SipAgent>(new rfc3261::SipAgent(ec, m_serviceManager.getIoService(), applicationParameters, context, pOfferAnswer));
  // note that at this point video and audio devices have not been set yet.
  if (ec)
  {
    m_pSipAgent.reset();
  }
  return ec;
}

boost::system::error_code SipSessionState::initialiseRtsp(const GenericParameters& applicationParameters,
  const rfc2326::RtspUri& rtspUri,
  const std::vector<std::string>& vLocalInterfaces)
{
  m_pRtspClientSession = std::unique_ptr<rfc2326::RtspClientSession>(
    new rfc2326::RtspClientSession(m_serviceManager.getIoService(), applicationParameters, rtspUri, vLocalInterfaces));

  uint32_t uiServiceId;
  bool bSuccess = m_serviceManager.registerService(boost::bind(&rfc2326::RtspClientSession::start, boost::ref(*m_pRtspClientSession.get())),
    boost::bind(&rfc2326::RtspClientSession::shutdown, boost::ref(*m_pRtspClientSession.get())),
    uiServiceId);
  if (bSuccess)
  {
    VLOG(2) << "Register RTSP client session with service manager";
    return boost::system::error_code();
  }
  else
  {
    LOG(WARNING) << "Failed to register RTSP client session with service manager";
    m_pRtspClientSession.reset();
    return boost::system::error_code(boost::system::errc::state_not_recoverable, boost::system::get_generic_category());
  }
}

}  // rtp_plus_plus

