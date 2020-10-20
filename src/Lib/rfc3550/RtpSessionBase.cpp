#include "CorePch.h"
#include <rtp++/rfc3550/RtpSessionBase.h>

#include <boost/exception/all.hpp>
#include <rtp++/rfc3550/RtcpPacketBase.h>
#include <rtp++/util/ApplicationParameters.h>

#ifndef EXIT_ON_TRUE
#define EXIT_ON_TRUE(condition, log_message) if ((condition)){\
  LOG(WARNING) << log_message;return false;}
#endif

namespace rfc3550
{

RtpSessionBase::RtpSessionBase(const RtpSessionParameters& parameters, const GenericParameters& applicationParameters)
  :m_parameters(parameters),
  m_applicationParameters(applicationParameters),
  m_state(SS_STOPPED),
  m_uiByeSentCount(0)
{
}

RtpSessionBase::~RtpSessionBase()
{
}

bool RtpSessionBase::initialise()
{
  try
  {
    m_state = SS_STOPPED;
    // create components
    m_vPayloadPacketisers = createPayloadPacketisers(m_parameters, m_applicationParameters);
    if (m_vPayloadPacketisers.empty())
    {
      // create default packetiser
      LOG(WARNING) << "Unable to create suitable packetiser, creating default packetiser";
      m_vPayloadPacketisers.push_back(PayloadPacketiserBase::create());
    }

    m_vSessionDbs = createSessionDatabases(m_parameters);
    EXIT_ON_TRUE(m_vSessionDbs.empty(), "Failed to construct session database(s)");

    m_vRtpInterfaces = createRtpNetworkInterfaces(m_parameters);
    EXIT_ON_TRUE(m_vRtpInterfaces.empty(), "Failed to create RTP network interfaces");
    initialiseRtpNetworkInterfaces(m_parameters);

    // create playout buffer
    m_vReceiverBuffers = createRtpPlayoutBuffers(m_parameters);
    EXIT_ON_TRUE(m_vReceiverBuffers.empty(), "Failed to construct receiver buffers");
    configureReceiverBuffers(m_parameters, m_applicationParameters);

    // TODO: abstract and define interface for receiver buffer
    // TODO: then move the following to RtpPlayoutBuffer implementation
    // set receiver buffer clock frequency so that we can adjust the jitter buffer
#if 0
    m_pPlayoutBuffer->setClockFrequency(m_parameters.getRtpTimestampFrequency());
    // set buffer latency if configured in application parameters
    boost::optional<uint32_t> uiBufferLatency = m_applicationParameters.getUintParameter(ApplicationParameters::buf_lat);
    if (uiBufferLatency) m_pPlayoutBuffer->setPlayoutBufferLatency(*uiBufferLatency);
#endif

    // RTCP inialisation MUST only occur after network interface initialisation
    // which is after the RTP network interfaces have been constructed
    m_vRtcpReportManagers = createRtcpReportManagers(m_parameters, m_applicationParameters, m_vSessionDbs);
    EXIT_ON_TRUE(m_vRtcpReportManagers.empty(), "Failed to construct RTCP report managers");
    initialiseRtcp();

    // RTP session state
    m_vRtpSessionStates = createRtpSessionStates(m_parameters);
    EXIT_ON_TRUE(m_vRtpSessionStates.empty(), "Failed to create RTP session states");

    // RTP flows: linking up all the previously constructed objects
    m_vRtpFlows = createRtpFlows(m_parameters);
    EXIT_ON_TRUE(m_vRtpFlows.empty(), "Failed to create RTP flows");

    return true;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }
  return false;
}

boost::system::error_code RtpSessionBase::start()
{
  if (m_state == SS_STOPPED)
  {
    if (initialise())
    {
      boost::system::error_code ec = doStart();
      if (!ec)
      {
        m_state = SS_STARTED;
      }
      return ec;
    }
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  else
  {
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

boost::system::error_code RtpSessionBase::stop()
{
  if (m_state == SS_STARTED)
  {
    VLOG(2) << "Shutting down RTP session";
    m_state = SS_SHUTTING_DOWN;
    VLOG(2) << "Shutting down RTCP";
    shutdownRtcp();
    // let subclasses shutdown
    boost::system::error_code ec = doStop();
    VLOG(2) << "Waiting for shutdown to complete";
    // FIXME: this should only be reset once the shutdown is complete
    // i.e. all asynchronous handlers have been called
    // Commenting this out: for now the solution is to manually call reset
    // which will reset all session state
    // m_state = SS_STOPPED;
    return ec;
  }
  else
  {
    LOG(WARNING) << "Invalid state to call stop: " << m_state;
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
}

boost::system::error_code RtpSessionBase::reset()
{
  // reset state so that the session can be started again
  m_state = SS_STOPPED;
  // TODO: reinit all components
  return boost::system::error_code();
}

void RtpSessionBase::sendSample(MediaSample::ptr pSample)
{
  if (m_state == SS_STARTED)
  {
    doSendSample(pSample);
  }
  else if (m_state == SS_STOPPED)
  {
    LOG_FIRST_N(WARNING, 1) << "Session has not been started.";
  }
  else if (m_state == SS_SHUTTING_DOWN)
  {
    LOG_FIRST_N(INFO, 1) << "Shutting down, not sending any more samples.";
  }
}

void RtpSessionBase::sendSamples(const std::vector<MediaSample::ptr>& vMediaSamples)
{
  if (m_state == SS_STARTED)
  {
    doSendSamples(vMediaSamples);
  }
  else if (m_state == SS_STOPPED)
  {
    LOG_FIRST_N(WARNING, 1) << "Session has not been started.";
  }
  else if (m_state == SS_SHUTTING_DOWN)
  {
    LOG_FIRST_N(INFO, 1) << "Shutting down, not sending any more samples.";
  }
}

void RtpSessionBase::doSendSamples(const std::vector<MediaSample::ptr>& vMediaSamples)
{
  for (MediaSample::ptr pMediaSample: vMediaSamples)
  {
    doSendSample(pMediaSample);
  }
}

void RtpSessionBase::onIncomingRtp( const RtpPacket& rtpPacket, const EndPoint& ep )
{
#ifdef DEBUG_INCOMING_RTP
  VLOG(10) << "Received RTP from " << ep.getAddress() << ":" << ep.getPort();
#endif

  // convert arrival time to RTP time
  uint32_t uiRtpTime = convertTimeToRtpTimestamp(rtpPacket.getArrivalTime(), m_parameters.getRtpTimestampFrequency(), m_parameters.getRtpTimestampBase());
  RtpPacket& rPacket = const_cast<RtpPacket&>(rtpPacket);
  rPacket.setRtpArrivalTimestamp(uiRtpTime);

  if (m_incomingRtp) m_incomingRtp(rtpPacket, ep);

  doHandleIncomingRtp(rtpPacket, ep);
}

void RtpSessionBase::onOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint& ep )
{
  doHandleOutgoingRtp(rtpPacket, ep);
  if (m_parameters.getRetransmissionTimeout() > 0)
  {
    storeRtpPacketForRetransmissionAndSchedulePacketRemoval(rtpPacket, ep, m_parameters.getRetransmissionTimeout());
  }
}

void RtpSessionBase::onIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep )
{
#ifdef DEBUG_INCOMING_RTP
  VLOG(10) << "Received RTCP from " << ep.getAddress() << ":" << ep.getPort();
#endif

  if (m_incomingRtcp) m_incomingRtcp(compoundRtcp, ep);
  doHandleIncomingRtcp(compoundRtcp, ep);
}

void RtpSessionBase::onOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep )
{
  doHandleOutgoingRtcp(compoundRtcp, ep);

  // This code forms an integral part of the event loop: it assumes that each RtcpReportManager
  // must send a BYE at the end of the session and that each sent BYE results in this method being
  // called.

  // check if the outgoing packet contains an RTCP BYE: if so, shutdown the network interfaces
  // If all required BYEs have been sent, shutdown the network interfaces
  std::for_each(compoundRtcp.begin(), compoundRtcp.end(), [this](RtcpPacketBase::ptr pRtcpPacket)
  {
    if (pRtcpPacket->getPacketType() == PT_RTCP_BYE)
    {
      ++m_uiByeSentCount;
    }
  });

  VLOG_IF(5, m_uiByeSentCount > 0) << m_uiByeSentCount << " RTCP BYE(s) sent. " << m_vRtcpReportManagers.size() << " report managers.";
  if (m_uiByeSentCount == m_vRtcpReportManagers.size())
  {
    LOG(INFO) << "RTCP BYE(s) sent: shutting down RTP interface";
    shutdownNetworkInterfaces();
  }
}

void RtpSessionBase::initialiseRtpNetworkInterfaces(const RtpSessionParameters &rtpParameters)
{
  std::for_each(m_vRtpInterfaces.begin(), m_vRtpInterfaces.end(), [this, &rtpParameters](std::unique_ptr<RtpNetworkInterface>& pRtpInterface)
  {
    configureNetworkInterface(pRtpInterface, rtpParameters);

    pRtpInterface->setIncomingRtpHandler(boost::bind(&RtpSessionBase::onIncomingRtp, this, _1, _2) );
    pRtpInterface->setIncomingRtcpHandler(boost::bind(&RtpSessionBase::onIncomingRtcp, this, _1, _2) );
    pRtpInterface->setOutgoingRtpHandler(boost::bind(&RtpSessionBase::onOutgoingRtp, this, _1, _2) );
    pRtpInterface->setOutgoingRtcpHandler(boost::bind(&RtpSessionBase::onOutgoingRtcp, this, _1, _2) );
    pRtpInterface->recv();
  });
}

void RtpSessionBase::shutdownNetworkInterfaces()
{
  std::for_each(m_vRtpInterfaces.begin(), m_vRtpInterfaces.end(), [this](std::unique_ptr<RtpNetworkInterface>& pRtpInterface)
  {
    pRtpInterface->shutdown();
  });
  VLOG(10) << "RTP network interfaces shutdown";
}

void RtpSessionBase::initialiseRtcp()
{
  std::for_each(m_vRtcpReportManagers.begin(), m_vRtcpReportManagers.end(), [this](std::unique_ptr<rfc3550::RtcpReportManager>& pRtcpReportManager)
  {
    pRtcpReportManager->startReporting();
  });

  m_uiByeSentCount = 0;
}

void RtpSessionBase::shutdownRtcp()
{
  std::for_each(m_vRtcpReportManagers.begin(), m_vRtcpReportManagers.end(), [this](std::unique_ptr<rfc3550::RtcpReportManager>& pRtcpReportManager)
  {
    pRtcpReportManager->scheduleFinalReportAndShutdown();
  });
  VLOG(10) << "RTCP report managers shutdown";
}

}
