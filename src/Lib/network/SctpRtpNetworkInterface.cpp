#include "CorePch.h"
#include <rtp++/network/SctpRtpNetworkInterface.h>
#include <rtp++/application/ApplicationParameters.h>
#include <boost/bind.hpp>

namespace rtp_plus_plus
{
namespace sctp
{

const uint32_t SctpRtpNetworkInterface::RTCP_CHANNEL_ID = 0;

SctpRtpNetworkInterface::SctpRtpNetworkInterface(const EndPoint &localRtpEp,
                                                 const EndPoint &remoteRtpEp,
                                                 bool bIsActive,
                                                 const sctp::SctpRtpPolicy& sctpPolicy,
                                                 bool bConnect,
                                                 const GenericParameters &applicationParameters)
  : RtpNetworkInterface(std::unique_ptr<RtpPacketiser>(new RtpPacketiser())),
    m_localRtpEp(localRtpEp),
    m_remoteRtpEp(remoteRtpEp),
    m_bIsConnected(false),
    m_sctpPolicy(sctpPolicy),
    m_bShuttingDown(false),
    // FOR NOW USING THE SAME RTP and SCTP ports when encapsulating over UDP
#ifdef ENABLE_SCTP_USERLAND
    m_bConnect(bConnect),
    m_sctpAssociation(localRtpEp.getPort(), localRtpEp.getPort(), remoteRtpEp.getPort(), bConnect),
#endif
    m_dRtt(-1.0),
    m_ePriotizationMode(PRIORITISE_NONE)
{
  boost::optional<std::string> sPrioritisationMode
      = applicationParameters.getStringParameter(app::ApplicationParameters::sctp_prio);
  if (sPrioritisationMode)
  {
    if (*sPrioritisationMode == app::ApplicationParameters::sctp_prio_base_layer)
    {
      m_ePriotizationMode = PRIORITISE_BL;
      VLOG(2) << "Using sctp base layer prioritisation";
    }
    else if (*sPrioritisationMode == app::ApplicationParameters::sctp_prio_scalability)
    {
      m_ePriotizationMode = PRIORITISE_SCALABLITY;
      VLOG(2) << "Using sctp scalability prioritisation";
    }
    else
    {
      VLOG(2) << "Using default sctp prioritisation";
    }
  }
  else
  {
    VLOG(2) << "Using default sctp prioritisation";
  }

  // HACK to stop SCTP connection when using RTP packetiser
  if (bConnect)
  {
#ifdef ENABLE_SCTP_USERLAND
  m_sctpAssociation.setRecvCallback(boost::bind(&SctpRtpNetworkInterface::onSctpRecv, this, _1, _2));
  m_sctpAssociation.setSendFailedCallback(boost::bind(&SctpRtpNetworkInterface::onSctpSendFail, this, _1, _2));
  // TODO: always bind local end point?
  m_sctpAssociation.bind();

  // active hosts should assign channel 0 for RTCP
  // passive hosts should start listening for incoming connections

  if (bIsActive)
  {
    LOG(INFO) << "Active host: connecting to " << remoteRtpEp;
    // BLOCKING?
    if (!m_sctpAssociation.connect(m_remoteRtpEp))
    {
      LOG(WARNING) << "Failed to connect to " << remoteRtpEp;
    }
    else
    {
      m_bIsConnected = true;
      LOG(INFO) << "Connected to " << remoteRtpEp;

      LOG(INFO) << "Opening RTCP/RTP channels";

      // open default RTCP channel
      const PolicyDescriptor& rtcpPolicy = m_sctpPolicy.getRtcpPolicy();
      if (!createChannel(rtcpPolicy))
      {
        LOG(WARNING) << "Unable to open channel!";
        // TODO: how to respond: try again later? Exit app?
        assert(false);
      }
      else
      {
        assert(rtcpPolicy.Channel_Id == 0);
        LOG(INFO) << "RTCP Channel opened";
        m_mChannelStatus[rtcpPolicy.Channel_Id] = true;
      }
      // open default RTP policy

      const PolicyDescriptor& rtpPolicy = m_sctpPolicy.getRtpPolicy();
      if (!createChannel(rtpPolicy))
      {
        LOG(WARNING) << "Unable to open channel!";
        // TODO: how to respond: try again later? Exit app?
      }
      else
      {
        LOG(INFO) << "RTP Channel opened";
        assert(rtpPolicy.Channel_Id == 1);
        m_mChannelStatus[rtpPolicy.Channel_Id] = true;
      }

      const std::vector<PolicyDescriptor>& rtpPolicies = m_sctpPolicy.getAdditionalRtpPolicies();
      for (auto& policy: rtpPolicies)
      {
        // only add static descriptors: RTT-based ones have to be added at run-time
        if (policy.RttFactor <= 0.0)
        {
          if (!createChannel(policy))
          {
            LOG(WARNING) << "Unable to open channel!";
            // TODO: how to respond: try again later? Exit app?
            m_mChannelStatus[policy.Channel_Id] = false;
          }
          else
          {
            LOG(INFO) << "Channel opened";
            m_mChannelStatus[policy.Channel_Id] = true;
          }
        }
        else
        {
          m_mChannelStatus[policy.Channel_Id] = false;
        }
      }
      m_sctpAssociation.print_status();
    }
  }
  else
  {
    LOG(INFO) << "Passive host: waiting for connections";
    // BLOCKING?
    if (m_sctpAssociation.listen())
    {
      LOG(INFO) << "Passive host: after listen";
      m_bIsConnected = true;
    }
    else
    {
      LOG(WARNING) << "Failed to listen";
    }
  }
#else
  LOG(FATAL) << "NO SCTP support is configured";
#endif
  }
}

void SctpRtpNetworkInterface::reset()
{

}

void SctpRtpNetworkInterface::updateRoundTripTime(double dRtt)
{
  // Once the RTT is set, the send call will open necessary channels
  m_dRtt = dRtt;
}

void SctpRtpNetworkInterface::shutdown()
{
  VLOG(2) << "Shutting down SCTP association";
  m_bShuttingDown = true;

#ifdef ENABLE_SCTP_USERLAND
  if (m_bConnect)
  {
    // close all associations
    if (!m_sctpAssociation.shutdown())
    {
      LOG(WARNING) << "Failed to shutdown socket";
    }
    else
    {
      VLOG(5) << "Shutdown SCTP socket";
    }
  }
  m_bIsConnected = false;
#endif
}

bool SctpRtpNetworkInterface::recv()
{
#ifdef ENABLE_SCTP_USERLAND
  // NOOP should suffice here: the sctp user stack
  // seems to launch a background thread to do reads
  // and invokes the callback?
  return true;
#else
  return false;
#endif
}

bool SctpRtpNetworkInterface::doSendRtp(Buffer rtpBuffer, const EndPoint &rtpEp)
{
  if (m_bShuttingDown)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTP packets";
    return false;
  }

  if (!m_bIsConnected)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Not connected, unable to deliver RTP packets";
    // TODO: try connect here if is active connection?!?
    return false;
  }

  uint32_t id = rtpEp.getId();

  // TODO: HACK until we can pass through the channel somehow
  if (id == RTCP_CHANNEL_ID )
  {
    id = 1;
  }

  // check if channel has been opened
  auto it =  m_mChannelStatus.find(id);
  if (it != m_mChannelStatus.end())
  {
    // if channel is not open yet, use default channel
    if (!it->second)
    {
      LOG(WARNING) << "Channel not open yet: using default RTP channel: "  << id;
#if 1
      // open channel now
      if (m_dRtt > 0.0)
      {
        LOG(WARNING) << "RTT measured: " << m_dRtt << "s Opening additional channels";
#if 1
        bool bNew = false;
        // first RTT update
        const std::vector<PolicyDescriptor>& rtpPolicies = m_sctpPolicy.getAdditionalRtpPolicies();
        for (auto& policy: rtpPolicies)
        {
          if (m_mChannelStatus[policy.Channel_Id] == false &&
              policy.SctpPrPolicy == SCTP_PR_POLICY_SCTP_TTL &&
              policy.RttFactor > 0.0)
          {
            if (!createChannel(policy, m_dRtt))
            {
              LOG(WARNING) << "Unable to open channel!";
              // TODO: how to respond: try again later? Exit app?
            }
            else
            {
              LOG(INFO) << "Channel opened";
              m_mChannelStatus[policy.Channel_Id] = true;
              bNew = true;
            }
          }
        }
#ifdef ENABLE_SCTP_USERLAND
        if (bNew)
          m_sctpAssociation.print_status();
#endif
    #endif
      }
      else
      {
        id = 1;
      }
#else
      id = 1;
#endif
    }
  }
  else
  {
    // if channel is invalid, use default channel
    LOG_FIRST_N(WARNING, 1) << "Invalid channel : " << id << " using default RTP channel";
    id = 1;
  }

#ifdef ENABLE_SCTP_USERLAND

#if 0
  VLOG(2) << "Sending RTP on channel " << id;
#endif

  // Hack: in oder to select which channel data should be sent over we will use the ID
  // field in the endpoint. The Channel 0 will be used for RTCP.
  int res = m_sctpAssociation.send_user_message(id, (char*)rtpBuffer.data(), rtpBuffer.getSize());
  // HACK: manually call handler: this send is synchronous.
  // If we change it to async, we must call the handler in the
  // completion handler
  onRtpSent(rtpBuffer, rtpEp);

  if (!res)
  {
    LOG(WARNING) << "Message sending failed.";
    return false;
  }
  else
  {
    VLOG(15) << "Message sent.";
    return true;
  }
#else
  return false;
#endif
}

bool SctpRtpNetworkInterface::doSendRtcp(Buffer rtcpBuffer, const EndPoint &rtcpEp)
{
  if (m_bShuttingDown)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Shutting down, unable to deliver RTCP packets";
    return false;
  }

  if (!m_bIsConnected)
  {
    // only log first occurence
    LOG_FIRST_N(INFO, 1) << "Not connected, unable to deliver RCTP packets";
    // TODO: try connect here if is active connection?!?
    // if passive, continue waiting?!?
    return false;
  }

  // Hack: in oder to select which channel data should be sent over we will use the ID
  // field in the endpoint. The Channel 0 will be used for RTCP.
#ifdef ENABLE_SCTP_USERLAND
  const uint32_t RTCP_CHANNEL_ID = 0;
  int res = m_sctpAssociation.send_user_message(RTCP_CHANNEL_ID, (char*)rtcpBuffer.data(), rtcpBuffer.getSize());
  // HACK: we rely on the onRtcpSent to be called to shutdown the RTP session
  // manually call handler: this send is synchronous.
  // If we change it to async, we must call the handler in the
  // completion handler.
  onRtcpSent(rtcpBuffer, rtcpEp);
  if (!res)
  {
    LOG(WARNING) << "Sending RTCP failed.";
    return false;
  }
  else
  {
    VLOG(5) << "RTCP sent.";
    return true;
  }
#else
  return false;
#endif

}

bool SctpRtpNetworkInterface::createChannel(const PolicyDescriptor& policy)
{
#ifdef ENABLE_SCTP_USERLAND
  channel* newChannel = m_sctpAssociation.open_channel(policy.Unordered, policy.SctpPrPolicy, policy.PrVal);
  if (newChannel == NULL)
  {
    LOG(WARNING) << "Creating channel failed. Expected Id: " << policy.Channel_Id;
    return false;
  }
  else
  {
    VLOG(5) << "Channel with id " << newChannel->id << " created. Expected ID: " << policy.Channel_Id;
    switch (m_ePriotizationMode)
    {
      case PRIORITISE_BL:
      {
        VLOG(2) << " SCTP: Prioritising base layer";
        uint16_t prio = 0;
        if (policy.Layer == LAYER_TYPE_BL)
        {
          prio = 0;
        }
        else if (policy.Layer == LAYER_TYPE_EL)
        {
          prio = 1;
        }

        if (m_sctpAssociation.setChannelPriority(newChannel->id, prio))
        {
          VLOG(2) << "Set priority of channel: " << newChannel->id << " to " << prio;
        }
        else
        {
          LOG(WARNING) << "setsockopt Failed to set priority of channel " << newChannel->id;
        }
        break;
      }
      case PRIORITISE_SCALABLITY:
      {
        VLOG(2) << " SCTP: Prioritising according to scalable ID";
        // the priority is calculated as the shifted sum of the scalability components
        // since it needs to fit into a uint16_t, we give each component 4 bits
        // this should be sufficient for the number of layers currently foreseen.
        assert(policy.D_L_ID < 16 && policy.D_L_ID < 16 && policy.T_ID < 16);

        // default
        uint16_t uiPriority = 0;
        if (policy.D_L_ID == -1 || policy.Q_T_ID == -1 || policy.T_ID == -1)
        {
          LOG(WARNING) << "Invalid qualifier, using default priority ("
                       << " DL:QT:T: " << policy.D_L_ID << ":" << policy.Q_T_ID << ":" << policy.T_ID << ")";
        }
        else
        {
          uiPriority = static_cast<uint16_t>((policy.D_L_ID << 8) + (policy.Q_T_ID << 4) + policy.T_ID);
        }
        if (m_sctpAssociation.setChannelPriority(newChannel->id, uiPriority))
        {
          VLOG(2) << "Set priority of channel: " << newChannel->id
                  << " to " << uiPriority
                  << " DL:QT:T: " << policy.D_L_ID << ":" << policy.Q_T_ID << ":" << policy.T_ID;
        }
        else
        {
          LOG(WARNING) << "setsockopt Failed to set priority of channel " << newChannel->id;
        }
        break;
      }
      case PRIORITISE_NONE:
      default:
      {
        VLOG(2) << " SCTP: no prioritisation";
        break;
      }
    }

    return true;
  }
#else
  return false;
#endif
}

// HACK: needs to be refactored
bool SctpRtpNetworkInterface::createChannel(const PolicyDescriptor& policy, double dRtt)
{
#ifdef ENABLE_SCTP_USERLAND
  uint32_t uiDelayMs = static_cast<uint32_t>(dRtt * policy.RttFactor * 1000);
  channel* newChannel = m_sctpAssociation.open_channel(policy.Unordered, policy.SctpPrPolicy, uiDelayMs);
  if (newChannel == NULL)
  {
    LOG(WARNING) << "Creating channel failed. Expected Id: " << policy.Channel_Id;
    return false;
  }
  else
  {
    VLOG(5) << "Channel with id " << newChannel->id << " created. Expected ID: "
            << policy.Channel_Id << " PrVal: " << uiDelayMs
            << " RttFactor: " << policy.RttFactor
            << " RTT: " << dRtt;
    return true;
  }
#else
  return false;
#endif
}

/// This function is called by the SCTP receive thread. It should not call methods on the SCTP
/// association. Seems to deadlock application
void SctpRtpNetworkInterface::onSctpRecv(uint32_t uiChannelId, const NetworkPacket& networkPacket)
{
#ifdef DEBUG_INCOMING_RTP
  LOG_EVERY_N(INFO, 100) << "[" << this << "] Read  " << google::COUNTER << "RTP packets";
#endif

  if (uiChannelId == RTCP_CHANNEL_ID)
  {
    VLOG(15) << "Incoming RTCP SCTP on channel " << uiChannelId << " Size: " << networkPacket.getSize();
    processIncomingRtcpPacket(networkPacket, m_remoteRtpEp);
  }
  else
  {
    VLOG(15) << "Incoming RTP SCTP on channel " << uiChannelId << " Size: " << networkPacket.getSize();
    processIncomingRtpPacket(networkPacket, m_remoteRtpEp);
  }
}

void SctpRtpNetworkInterface::onSctpSendFail(uint8_t* pData, uint32_t uiLength)
{
  VLOG(2) << "SctpRtpNetworkInterface::onSctpSendFail Lost packet: size: " << uiLength;
  // TODO: need to differentiate between failed RTP and RTCP?
  assert(uiLength > 4);
  uint16_t uiSN = (pData[2] << 8) | pData[3];
  VLOG(2) << "Unable to send packet with SN " << uiSN;
}

} // sctp
} // rtp_plus_plus
