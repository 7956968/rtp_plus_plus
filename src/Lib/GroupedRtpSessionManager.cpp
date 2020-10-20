#include "CorePch.h"
#include <rtp++/GroupedRtpSessionManager.h>
#include <boost/optional.hpp>
#include <cpputil/ExceptionBase.h>
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/application/ApplicationUtil.h>

using boost::optional;

namespace rtp_plus_plus
{

GroupedRtpSessionManager::GroupedRtpSessionManager(const GenericParameters& applicationParameters)
  :IRtpSessionManager(applicationParameters),
    m_eState(SS_STOPPED),
    m_uiByeReceivedCount(0),
    m_bExitOnBye(false),
    m_bAnalyse(false)
{
  optional<bool> exitOnBye = applicationParameters.getBoolParameter(app::ApplicationParameters::exit_on_bye);
  if (exitOnBye) m_bExitOnBye = *exitOnBye;

  optional<bool> bAnalyse = applicationParameters.getBoolParameter(app::ApplicationParameters::analyse);
  if (bAnalyse) m_bAnalyse = *bAnalyse;
}

GroupedRtpSessionManager::~GroupedRtpSessionManager()
{
  if (m_bAnalyse)
  {
    for (auto& mapEntry : m_mLostSequenceNumbers)
    {
      std::ostringstream ostr;
      std::vector<uint32_t>& vSequenceNumbers = mapEntry.second;
      if (!vSequenceNumbers.empty())
      {
        uint32_t uiMin = vSequenceNumbers[0];
        uint32_t uiMax = vSequenceNumbers[vSequenceNumbers.size() - 1];
        for (size_t i = uiMin; i <= uiMax; ++i)
        {
          auto it = std::find_if(vSequenceNumbers.begin(), vSequenceNumbers.end(), [i](uint32_t uiSN)
          {
            return (uint32_t)i == uiSN;
          });
          if (it == vSequenceNumbers.end())
          {
            // couldn't find it
            ostr << i << " ";
          }
        }
      }
    VLOG(2) << "RTP sequence number losses. Mid: " << mapEntry.first << " : "
              << ostr.str();
    }
  }
}

void GroupedRtpSessionManager::addRtpSession(const RtpSessionParameters& rtpParameters,
                                             RtpSession::ptr pRtpSession)
{
  m_vRtpParameters.push_back(rtpParameters);
  m_mRtpSessions[rtpParameters.getMid()] = pRtpSession;
  m_vMids.push_back(rtpParameters.getMid());

  pRtpSession->setIncomingRtpHandler(
    boost::bind(&GroupedRtpSessionManager::handleIncomingRtp, this, _1, _2, _3, _4, _5, rtpParameters.getMid()));

  pRtpSession->setIncomingRtcpHandler(
    boost::bind(&GroupedRtpSessionManager::handleIncomingRtcp, this, _1, _2, rtpParameters.getMid()));

  pRtpSession->setMemberUpdateHandler(
    boost::bind(&GroupedRtpSessionManager::handleGroupedMemberUpdate, this, _1, rtpParameters.getMid()));

  onRtpSessionAdded(rtpParameters.getMid(), rtpParameters, pRtpSession);
}

std::vector<std::string> GroupedRtpSessionManager::getMediaTypes() const
{
  std::vector<std::string> vMediaTypes;
  for (const RtpSessionParameters& rtpParameter : m_vRtpParameters )
  {
    vMediaTypes.push_back(rtpParameter.getEncodingName());
  }
  return vMediaTypes;
}

std::string GroupedRtpSessionManager::getPrimaryMediaType() const
{
  // TODO: will this always be the first one???
  return m_vRtpParameters[0].getEncodingName();
}

const RtpSessionParameters& GroupedRtpSessionManager::getPrimaryRtpSessionParameters() const
{
  if (m_vRtpParameters.empty())
    BOOST_THROW_EXCEPTION(ExceptionBase("No RTP sessions have been set"));

  return m_vRtpParameters[0];
}

RtpSession& GroupedRtpSessionManager::getPrimaryRtpSession()
{
  if (m_vMids.empty())
    BOOST_THROW_EXCEPTION(ExceptionBase("No RTP sessions have been set"));
  return *(m_mRtpSessions[m_vMids[0]].get());
}

boost::system::error_code GroupedRtpSessionManager::start()
{
  if (m_eState == SS_STOPPED)
  {
    VLOG(2) << "[" << this << "] Starting media session";
    for (auto& pair :  m_mRtpSessions)
    {
      VLOG(2) << "[" << this << "] Starting RTP session with MID: " << pair.first;
      boost::system::error_code ec = pair.second->start();
      if (ec)
      {
        LOG(WARNING) << "Failed to start RTP session: " << ec.message();
        return ec;
      }
    }
    onSessionsStarted();

    m_eState = SS_STARTED;
    return boost::system::error_code();
  }
  else
  {
    LOG(WARNING) << "Session manager already started: " << m_eState;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

boost::system::error_code GroupedRtpSessionManager::stop()
{
  VLOG(2) << "[" << this << "] Stopping media session";
  if (m_eState == SS_STARTED)
  {
    boost::system::error_code res_ec;
      // stop all RTP sessions
    for (auto& pair :  m_mRtpSessions)
    {
      VLOG(2) << "[" << this << "] Stopping RTP session with MID: " << pair.first;
      boost::system::error_code ec = pair.second->stop();
      if (ec)
      {
        LOG(WARNING) << "Failed to stop RTP session: " << ec.message();
        // NOTE: shortcoming of this code is that we can only return one error (the last one)
        res_ec = ec;
      }
    }
    onSessionsStopped();

    m_eState = SS_STOPPED;
    return res_ec;
  }
  else
  {
    LOG(WARNING) << "Session manager already stopped: " << m_eState;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

void GroupedRtpSessionManager::send(const media::MediaSample& mediaSample)
{
  return doSend(mediaSample);
}

void GroupedRtpSessionManager::send(const std::vector<media::MediaSample>& vMediaSamples)
{
  return doSend(vMediaSamples);
}

void GroupedRtpSessionManager::send(const media::MediaSample& mediaSample, const std::string& sMid)
{
  return doSend(mediaSample, sMid);
}

void GroupedRtpSessionManager::send(const std::vector<media::MediaSample>& vMediaSamples, const std::string& sMid)
{
  return doSend(vMediaSamples, sMid);
}

bool GroupedRtpSessionManager::isSampleAvailable() const
{
  return doIsSampleAvailable();
}

bool GroupedRtpSessionManager::isSampleAvailable(const std::string& sMid) const
{
  return doIsSampleAvailable(sMid);
}

std::vector<media::MediaSample> GroupedRtpSessionManager::getNextSample()
{
  return doGetNextSample();
}

std::vector<media::MediaSample> GroupedRtpSessionManager::getNextSample(const std::string& sMid)
{
  return doGetNextSample(sMid);
}

RtpSession::ptr GroupedRtpSessionManager::getRtpSession(const std::string& sMid)
{
  auto it = m_mRtpSessions.find(sMid);
  if (it == m_mRtpSessions.end())
    return RtpSession::ptr();
  else
    return m_mRtpSessions[sMid];
}

const std::string& GroupedRtpSessionManager::getMid(uint32_t uiIndex) const
{
  return m_vMids.at(uiIndex);
}

const uint32_t GroupedRtpSessionManager::getSessionCount() const
{
  return m_vMids.size();
}

void GroupedRtpSessionManager::onRtpSessionAdded(const std::string& sMid,
                                                 const RtpSessionParameters& rtpParameters,
                                                 RtpSession::ptr pRtpSession)
{
  // NOOP
}

void GroupedRtpSessionManager::handleIncomingRtp(const RtpPacket &rtpPacket, const EndPoint &ep,
                                                 bool bSSRCValidated, bool bRtcpSynchronised,
                                                 const boost::posix_time::ptime &tPresentation,
                                                 const std::string &sMid)
{
  VLOG(15) << "RTP [IN] SN: " << rtpPacket.getExtendedSequenceNumber()
           << " PTS: " << tPresentation
           << " From: " << ep
           << " Valid: " << bSSRCValidated
           << " Sync: " << bRtcpSynchronised
           << " Size: " << rtpPacket.getSize()
           << " RTP TS: " << rtpPacket.getRtpTimestamp()
           << " Mid: " << sMid;

  doHandleIncomingRtp(rtpPacket, ep, bSSRCValidated, bRtcpSynchronised, tPresentation, sMid);

  if (m_bAnalyse)
  {
    m_mLostSequenceNumbers[sMid].push_back(rtpPacket.getExtendedSequenceNumber());
  }
}

void GroupedRtpSessionManager::handleIncomingRtcp(const CompoundRtcpPacket &compoundRtcp,
                                                  const EndPoint &ep, const std::string &sMid)
{
  VLOG(15) << "Incoming RTCP from : " << ep << " Mid: " << sMid;
  doHandleIncomingRtcp(compoundRtcp, ep, sMid);
}

void GroupedRtpSessionManager::handleGroupedMemberUpdate(const MemberUpdate& memberUpdate, const std::string &sMid)
{
  VLOG(15) << "Member update from Mid: " << sMid << ":" << memberUpdate;
  if (memberUpdate.isMemberJoin())
  {

  }
  else if (memberUpdate.isMemberUpdate())
  {

  }
  else if (memberUpdate.isMemberLeave())
  {
    VLOG(2) << "Bye received.";
    if( m_bExitOnBye)
    {
      ++m_uiByeReceivedCount;

#ifdef WAIT_FOR_ALL_BYES
      // only once all byes have been received
      // if BYE is lost the application won't end
      if (m_uiByeReceivedCount == m_vMids.size())
#else
      // only on first BYE
      if (m_uiByeReceivedCount == 1)
#endif
      {
        VLOG(2) << "Shutting down session.";
        stop();
        VLOG(2) << "Session stopped.";
        // HACK for now
        app::ApplicationUtil::handleApplicationExit(boost::system::error_code(), boost::shared_ptr<SimpleMediaSession>());
        VLOG(2) << "Session stop signal fired.";
      }
    }
  }
}

} // rtp_plus_plus
