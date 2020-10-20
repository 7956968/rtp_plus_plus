#include "CorePch.h"
#include <rtp++/network/RtpNetworkInterface.h>
#include <cpputil/FileUtil.h>

// #define DEBUG_RTCP

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{

RtpNetworkInterface::RtpNetworkInterface()
  :m_bSecureRtp(false)
{

}

RtpNetworkInterface::RtpNetworkInterface(std::unique_ptr<RtpPacketiser> pPacketiser)
  :m_pRtpPacketiser(std::move(pPacketiser)),
    m_bSecureRtp(false)
{

}

RtpNetworkInterface::~RtpNetworkInterface()
{
}

void RtpNetworkInterface::setRtcpValidator(rfc3550::RtcpValidator::ptr pValidator)
{
  m_pRtpPacketiser->setRtcpValidator(std::move(pValidator));
}

void RtpNetworkInterface::registerRtcpParser(RtcpParserInterface::ptr pParser)
{
  m_pRtpPacketiser->registerRtcpParser(std::move(pParser) );
}

void RtpNetworkInterface::setRtpPacketiser(std::unique_ptr<RtpPacketiser> pPacketiser)
{
  m_pRtpPacketiser = std::move(pPacketiser);
}

bool RtpNetworkInterface::secureRtp(bool bSecure)
{
#ifdef ENABLE_LIB_SRTP
  // TODO: get SSRC from session
  uint32_t uiSsrc = 12345;
  ssrc_t ssrc;
  ssrc.type = ssrc_specific;
  ssrc.value = uiSsrc;

  // set policy to describe a policy for an SRTP stream
  crypto_policy_set_rtp_default(&m_srtpPolicy.rtp);
  crypto_policy_set_rtcp_default(&m_srtpPolicy.rtcp);
  m_srtpPolicy.ssrc = ssrc;
  m_srtpPolicy.key  = m_srtpKey;
  m_srtpPolicy.next = NULL;
  //policy.ssrc.type  = ssrc_specific;
  //policy.ssrc.value = ssrc;
  m_srtpPolicy.key  = (uint8_t *) m_srtpKey;
  m_srtpPolicy.ekt  = NULL;
  m_srtpPolicy.next = NULL;
  m_srtpPolicy.window_size = 128;
  m_srtpPolicy.allow_repeat_tx = 0;
  m_srtpPolicy.rtp.sec_serv = sec_serv_conf_and_auth;
  //policy.rtcp.sec_serv = sec_serv_conf_and_auth;  /* we don't do RTCP anyway */
  // TODO
  m_srtpPolicy.rtcp.sec_serv = sec_serv_none;  /* we don't do RTCP anyway */

  // set key to random value
  err_status_t err = crypto_get_random(m_srtpKey, 30);
  if (err)
  {
    LOG(WARNING) << "Error in crypto get random key: " << err;
    return false;
  }

  // allocate and initialize the SRTP session
  err = srtp_create(&m_srtpSession, &m_srtpPolicy);
  if (err)
  {
    LOG(WARNING) << "Error in srtp sender session create: " << err;
    return false;
  }

  m_bSecureRtp = bSecure;
  return true;
#else
  return false;
#endif
}

bool RtpNetworkInterface::send(const RtpPacket& rtpPacket, const EndPoint& destination)
{
  // TODO: could estimate the incoming framerate here: this would be useful when needing to fragment a frame over multiple RTP packets

  // we need to keep track of the packet oder in which RTP/RTCP is sent
  {
    boost::mutex::scoped_lock l(m_lock);
    m_qRtp.push_back(rtpPacket);
  }

#if 1
  VLOG(15) << "RTP sending SN: " << rtpPacket.getSequenceNumber()
           << " TS: " << rtpPacket.getRtpTimestamp()
           << " to " << destination;
#endif

  if (m_bSecureRtp)
  {
    // send the packet using the networking interface
    // TODO: how to handle MTU etc?!?
    // TODO: payload packetiser MTU needs to be adapted?!?
    Buffer rtpBuffer = m_pRtpPacketiser->packetise(rtpPacket, 1460);

#ifdef RTP_DEBUG
    m_rtpDump.dumpRtp(rtpBuffer);
#endif

#ifdef ENABLE_LIB_SRTP
    // protected packet
    int len = rtpPacket.getSize();
    err_status_t err = srtp_protect(m_srtpSession, (void*)rtpBuffer.data(), &len);
    if (err)
    {
      LOG(WARNING) << "Error encoding SRTP: " << err;
      return false;
    }
    else
    {
      // now len contains the updated buffer size:
      // we have to consume the difference to send the whole buffer
      int diff = len - rtpPacket.getSize();
      rtpBuffer.consumePostBuffer(diff);
    }

    // send packet to registered endpoints
    return doSendRtp(rtpBuffer, destination);

#else
    LOG(WARNING) << "Can't send packet: NO SRTP support!!!";
    return false;
#endif

  }
  else
  {
    // send the packet using the networking interface
    Buffer rtpBuffer = m_pRtpPacketiser->packetise(rtpPacket);

#ifdef RTP_DEBUG
    m_rtpDump.dumpRtp(rtpBuffer);
#endif
#if 0
    VLOG(2) << "Packetised RTP packet " << rtpBuffer.getSize();
#endif
    // send packet to registered endpoints
    return doSendRtp(rtpBuffer, destination);
  }
}

bool RtpNetworkInterface::send(const CompoundRtcpPacket& compoundPacket, const EndPoint& destination)
{
#ifdef RTCP_STRICT
  if (!m_pRtpPacketiser->validate(compoundPacket))
  {
    return false;
  }
#endif

  // we need to keep track of the packet order in which RTP/RTCP is sent
  {
    boost::mutex::scoped_lock l(m_rtcplock);
    m_qRtcp.push_back(compoundPacket);
  }

  if (m_bSecureRtp)
  {
    Buffer rtcpBuffer = m_pRtpPacketiser->packetise(compoundPacket, 1460);
    // send the packet using the networking interface
    // TODO: how to handle MTU etc?!?
    // TODO: payload packetiser MTU needs to be adapted?!?
#ifdef ENABLE_LIB_SRTP
#if 1
    // send RTCP unencrypted for now until we have encrypted RTP working
    return doSendRtcp(rtcpBuffer, destination);
#else
    // protected packet
    // TODO: calculate RTCP packet size somehow
    // int len = rtpPacket.getSize();
    int orig_len = 100;
    int len = orig_len;
    err_status_t err = srtp_protect_rtcp(m_srtpSession, (void*)rtcpBuffer.data(), &len);
    if (err)
    {
      LOG(WARNING) << "Error encoding SRTP: " << err;
      return false;
    }
    else
    {
      // now len contains the updated buffer size:
      // we have to consume the difference to send the whole buffer
      int diff = len - orig_len;
      rtcpBuffer.consumePostBuffer(diff);
    }
    return doSendRtcp(rtcpBuffer, destination);

#endif
#else
    LOG(WARNING) << "Can't send packet: NO SRTP support!!!";
    return false;
#endif
  }
  else
  {
    Buffer rtcpBuffer = m_pRtpPacketiser->packetise(compoundPacket);
    return doSendRtcp(rtcpBuffer, destination);
  }
}

void RtpNetworkInterface::onRtpSent(Buffer buffer, const EndPoint& ep)
{
#ifdef DEBUG_RTP
  DLOG(INFO) << "Sent RTP packet of size: " << buffer.getSize() << " to " << ep.toString();
#endif

  boost::mutex::scoped_lock l(m_lock);
  // pop RTP packet
  const RtpPacket& rtpPacket = m_qRtp.front();
  if (m_outgoingRtp) m_outgoingRtp(rtpPacket, ep);
  m_qRtp.pop_front();
}

void RtpNetworkInterface::onRtcpSent(Buffer buffer, const EndPoint& ep)
{
#ifdef DEBUG_RTCP
  DLOG(INFO) << "Sent RTCP packet of size: " << buffer.getSize() << " to " << ep.toString();
#endif

  boost::mutex::scoped_lock l(m_rtcplock);
  CompoundRtcpPacket& rtcpPacket = m_qRtcp.front();
  if (m_outgoingRtcp) m_outgoingRtcp(rtcpPacket, ep);
  m_qRtcp.pop_front();
}

void RtpNetworkInterface::processIncomingRtpPacket(NetworkPacket networkPacket, const EndPoint& ep)
{
  // parse RTP packet
  boost::optional<RtpPacket> rtpPacket = m_pRtpPacketiser->depacketise(networkPacket);
  if (rtpPacket)
  {
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
#ifdef MEASURE_RTP_INTERARRIVAL_TIME
    if (!m_tPreviousArrival.is_not_a_date_time())
    {
      VLOG(10) << "[" << this << "] Difference: " << (tNow - m_tPreviousArrival).total_milliseconds() << " ms";
    }
    m_tPreviousArrival = tNow;
#endif

    // Set RTP arrival time: this is needed for jitter calculations
    rtpPacket->setArrivalTime(tNow);
    rtpPacket->setSource(ep);

#ifdef DEBUG_RTP
    DLOG(INFO) << "Parsed RTP packet: " << rtpPacket->getHeader();
#endif
    if (m_incomingRtp) m_incomingRtp(*rtpPacket, ep);
  }
  else
  {
    LOG(WARNING) << "Failed to parse RTP packet from " << ep << " size: " << networkPacket.getSize();
    static int iErrorFile = 0;
    std::ostringstream ostr;
    ostr << "rtp_error_" << ep << "_" << iErrorFile++ << ".log";
    FileUtil::writeFile(ostr.str(), networkPacket, true);
  }
}

void RtpNetworkInterface::processIncomingRtcpPacket(NetworkPacket networkPacket, const EndPoint& ep)
{
  VLOG(COMPONENT_LOG_LEVEL) << "Parsing incoming network RTCP packet of size: " << networkPacket.getSize() << " from " << ep.toString();

#ifdef DEBUG_RTCP
  DLOG(INFO) << "Parsing incoming network RTCP packet of size: " << networkPacket.getSize() << " from " << ep.toString();
#endif

  // parse RTCP packet
  CompoundRtcpPacket compoundPacket =  m_pRtpPacketiser->depacketiseRtcp(networkPacket);
  if (compoundPacket.empty())
  {
    LOG(WARNING) << "Failed to parse RCTP packet";
    static int iRtcpErrorFile = 0;
    std::ostringstream ostr;
    ostr << "rtcp_error_" << ep << "_" << iRtcpErrorFile++ << ".log";
    FileUtil::writeFile(ostr.str(), networkPacket, true);
  }
  else
  {
    if (m_incomingRtcp) m_incomingRtcp(compoundPacket, ep);
  }
}

} // rtp_plus_plus
