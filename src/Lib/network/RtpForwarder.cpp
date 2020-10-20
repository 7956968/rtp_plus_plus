#include "CorePch.h"
#include <rtp++/network/RtpForwarder.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <rtp++/RtpUtil.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/network/MuxedUdpRtpNetworkInterface.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/UdpForwarder.h>
#include <rtp++/network/UdpRtpNetworkInterface.h>

namespace rtp_plus_plus
{

boost::shared_ptr<RtpForwarder> RtpForwarder::create(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                                                            const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                                                            std::vector<NetworkProperties_t> networkProps, bool bStrict)
{
  return boost::make_shared<RtpForwarder>(rIoService, localInterfaces, host1, host2, networkProps, bStrict);
}

boost::shared_ptr<RtpForwarder> RtpForwarder::create(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                                                            const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                                                            uint32_t uiLossProbability, const std::vector<double>& vDelays, bool bStrict)
{
  return boost::make_shared<RtpForwarder>(rIoService, localInterfaces, host1, host2, uiLossProbability, vDelays, bStrict);
}

RtpForwarder::RtpForwarder(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                           const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                           std::vector<NetworkProperties_t> networkProps, bool bStrict)
  :m_rIoService(rIoService),
    m_vNetworkProperties(networkProps),
    m_uiPacketLossProbability(0),
    m_bStrict(bStrict)
{
  VLOG(1) << "RTP forwarder Loss probability: " << m_uiPacketLossProbability
          << " Strict: " << bStrict;

  srand(static_cast<uint32_t>(time(NULL)));

  if (m_bStrict)
  {
    setupRtpForwarders(localInterfaces, host1, host2);
  }
  else
  {
    setupUdpForwarders(localInterfaces, host1, host2);
  }
}

RtpForwarder::RtpForwarder(boost::asio::io_service& rIoService, const InterfaceDescriptions_t& localInterfaces,
                           const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2,
                           uint32_t uiLossProbability,
                           const std::vector<double>& vDelays,
                           bool bStrict)
  :m_rIoService(rIoService),
    m_uiPacketLossProbability(uiLossProbability),
    m_vDelays(vDelays),
    m_bStrict(bStrict)
{
  VLOG(1) << "RTP forwarder Loss probability: " << m_uiPacketLossProbability
          << " Strict: " << bStrict
          << " Delay vector size: " << m_vDelays.size();
  srand(static_cast<uint32_t>(time(NULL)));

  if (m_bStrict)
  {
    setupRtpForwarders(localInterfaces, host1, host2);
  }
  else
  {
    setupUdpForwarders(localInterfaces, host1, host2);
  }
}

RtpForwarder::~RtpForwarder()
{
  DLOG(INFO) << "Rtp forwarder Destructor";
}

void RtpForwarder::setupRtpForwarders(const InterfaceDescriptions_t& localInterfaces,
                                      const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2)
{
  if (host1.size()!=host2.size())
  {
    LOG(WARNING) << "Remote host interface sizes don't match";
  }

  uint32_t uiCurrentInterfaceIndex = 0;
  // iterate over local endpoints and setup forwarding at RTP layer
  for (size_t j = 0; j < localInterfaces.size(); ++j)
  {
    const MediaInterfaceDescriptor_t& addresses_descriptors = localInterfaces[j];
    const MediaInterfaceDescriptor_t& host1_descriptors = host1[j];
    const MediaInterfaceDescriptor_t& host2_descriptors = host2[j];

    // An address descriptor has RTP and RTCP address information
    for (size_t i = 0; i < addresses_descriptors.size(); ++i)
    {
      AddressDescriptor rtp_descriptor = std::get<0>(addresses_descriptors.at(i));
      AddressDescriptor rtcp_descriptor = std::get<1>(addresses_descriptors.at(i));

      // remember the "current" RTP interface index for later binding
      uiCurrentInterfaceIndex = m_vRtpInterfaces.size();

      // check for RTP/RTCP multiplexing over one port
      if (rtp_descriptor.getPort() != rtcp_descriptor.getPort())
      {
        boost::system::error_code ec;
        std::unique_ptr<RtpNetworkInterface> pRtpInterface = std::unique_ptr<UdpRtpNetworkInterface>(new UdpRtpNetworkInterface(boost::ref(m_rIoService), EndPoint(rtp_descriptor), EndPoint(rtcp_descriptor), ec));
        if (!ec)
        {
          m_vRtpInterfaces.push_back(std::move(pRtpInterface));
        }
        else
        {
          LOG(WARNING) << "Failed to setup up forwarding relationship";
        }
      }
      else
      {
        boost::system::error_code ec;
        std::unique_ptr<RtpNetworkInterface> pRtpInterface = std::unique_ptr<MuxedUdpRtpNetworkInterface>(new MuxedUdpRtpNetworkInterface(boost::ref(m_rIoService), EndPoint(rtp_descriptor), ec));
        if (!ec)
        {
          m_vRtpInterfaces.push_back(std::move(pRtpInterface));
        }
        else
        {
          LOG(WARNING) << "Failed to setup up forwarding relationship";
        }
      }

      // update forwarding maps
      assert(host1_descriptors.size() == host2_descriptors.size());
      for (size_t k = 0; k < host1_descriptors.size(); ++k)
      {
        AddressDescriptor rtpAddress1 = std::get<0>(host1_descriptors[k]);
        AddressDescriptor rtcpAddress1 = std::get<1>(host1_descriptors[k]);
        AddressDescriptor rtpAddress2 = std::get<0>(host2_descriptors[k]);
        AddressDescriptor rtcpAddress2 = std::get<1>(host2_descriptors[k]);
        m_map[rtpAddress1.getAddress()][rtpAddress1.getPort()] = EndPoint(rtpAddress2);
        m_map[rtpAddress2.getAddress()][rtpAddress2.getPort()] = EndPoint(rtpAddress1);
        m_map[rtcpAddress1.getAddress()][rtcpAddress1.getPort()] = EndPoint(rtcpAddress2);
        m_map[rtcpAddress2.getAddress()][rtcpAddress2.getPort()] = EndPoint(rtcpAddress1);
        // network chars
        NetworkProperties_t networkProps = (j < m_vNetworkProperties.size()) ? m_vNetworkProperties[j] : std::make_tuple(0u, 0u, 0u);
        m_networkMap[rtpAddress1.getAddress()][rtpAddress1.getPort()] = networkProps;
        m_networkMap[rtpAddress2.getAddress()][rtpAddress2.getPort()] = networkProps;
        m_networkMap[rtcpAddress1.getAddress()][rtcpAddress1.getPort()] = networkProps;
        m_networkMap[rtcpAddress2.getAddress()][rtcpAddress2.getPort()] = networkProps;

      }

      m_vRtpInterfaces[uiCurrentInterfaceIndex]->setIncomingRtpHandler(boost::bind(&RtpForwarder::onIncomingRtp, this, _1, _2, uiCurrentInterfaceIndex) );
      m_vRtpInterfaces[uiCurrentInterfaceIndex]->setIncomingRtcpHandler(boost::bind(&RtpForwarder::onIncomingRtcp, this, _1, _2, uiCurrentInterfaceIndex) );
      m_vRtpInterfaces[uiCurrentInterfaceIndex]->setOutgoingRtpHandler(boost::bind(&RtpForwarder::onOutgoingRtp, this, _1, _2, uiCurrentInterfaceIndex) );
      m_vRtpInterfaces[uiCurrentInterfaceIndex]->setOutgoingRtcpHandler(boost::bind(&RtpForwarder::onOutgoingRtcp, this, _1, _2, uiCurrentInterfaceIndex) );
      m_vRtpInterfaces[uiCurrentInterfaceIndex]->recv();
    }
  }
}

void RtpForwarder::setupUdpForwarders(const InterfaceDescriptions_t& localInterfaces,
                                      const InterfaceDescriptions_t& host1, const InterfaceDescriptions_t& host2)
{
  VLOG(2) << "Constructing " << localInterfaces.size() << " UDP forwarder(s)";
  // iterate over local endpoints and setup forwarding at UDP layer
  for (size_t j = 0; j < localInterfaces.size(); ++j)
  {
    const MediaInterfaceDescriptor_t& addresses_descriptors = localInterfaces[j];
    const MediaInterfaceDescriptor_t& host1_descriptors = host1[j];
    const MediaInterfaceDescriptor_t& host2_descriptors = host2[j];

    // An address decsriptor has RTP and RTCP address information
    for (size_t i = 0; i < addresses_descriptors.size(); ++i)
    {
      uint32_t uiLoss = (j < m_vNetworkProperties.size()) ? std::get<0>(m_vNetworkProperties[j]) : 0;
      uint32_t uiOwd = (j < m_vNetworkProperties.size()) ? std::get<1>(m_vNetworkProperties[j]) : 0;
      uint32_t uiJitter = (j < m_vNetworkProperties.size()) ? std::get<2>(m_vNetworkProperties[j]) : 0;

      AddressDescriptor rtpDescriptor = std::get<0>(addresses_descriptors.at(i));
      AddressDescriptor rtcpDescriptor = std::get<1>(addresses_descriptors.at(i));

      UdpForwarder::ptr pRtpFwd;
      if (m_vDelays.empty())
      {
        VLOG(2) << "Constructing RTP forwarder";
        pRtpFwd = UdpForwarder::create( boost::ref(m_rIoService), EndPoint(rtpDescriptor), uiLoss, uiOwd, uiJitter );
      }
      else
      {
        VLOG(2) << "Constructing RTP forwarder";
        pRtpFwd = UdpForwarder::create( boost::ref(m_rIoService), EndPoint(rtpDescriptor), m_uiPacketLossProbability, m_vDelays );
      }
      m_vUdpForwarders.push_back(pRtpFwd);

      UdpForwarder::ptr pRtcpFwd;
      // check for RTP/RTCP multiplexing over one port
      if (rtpDescriptor.getPort() != rtcpDescriptor.getPort())
      {
        if (m_vDelays.empty())
        {
          VLOG(2) << "Constructing RTCP forwarder";
          pRtcpFwd = UdpForwarder::create(  boost::ref(m_rIoService), EndPoint(rtcpDescriptor), uiLoss, uiOwd, uiJitter);
        }
        else
        {
          VLOG(2) << "Constructing RTCP forwarder";
          pRtcpFwd = UdpForwarder::create(  boost::ref(m_rIoService), EndPoint(rtcpDescriptor), m_uiPacketLossProbability, m_vDelays);
        }
        m_vUdpForwarders.push_back(pRtcpFwd);
      }

      // setup forwarding
      assert(host1_descriptors.size() == host2_descriptors.size());
      for (size_t k = 0; k < host1_descriptors.size(); ++k)
      {
        AddressDescriptor rtpAddress1 = std::get<0>(host1_descriptors[k]);
        AddressDescriptor rtcpAddress1 = std::get<1>(host1_descriptors[k]);
        AddressDescriptor rtpAddress2 = std::get<0>(host2_descriptors[k]);
        AddressDescriptor rtcpAddress2 = std::get<1>(host2_descriptors[k]);
        VLOG(2) << "Constructing RTP forwarding pair " << EndPoint(rtpAddress1) << " to " << EndPoint(rtpAddress2);

        pRtpFwd->addForwardingPair(EndPoint(rtpAddress1), EndPoint(rtpAddress2));
        if (pRtcpFwd)
        {
          VLOG(2) << "Constructing RTCP forwarding pair " << EndPoint(rtcpAddress1) << " to " << EndPoint(rtcpAddress2);
          pRtcpFwd->addForwardingPair(EndPoint(rtcpAddress1), EndPoint(rtcpAddress2));
        }
      }

      std::for_each(m_vUdpForwarders.begin(), m_vUdpForwarders.end(), [](UdpForwarder::ptr pFwd)
      {
        pFwd->recv();
      });
    }
  }
}

void RtpForwarder::sendRtpPacket(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex )
{
  delete pTimer;
  if (!ec)
  {
    // send immediately
    m_vRtpInterfaces[uiInterfaceIndex]->send(rtpPacket, ep);
  }
}

void RtpForwarder::sendRtcpPacket(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex )
{
  delete pTimer;
  if (!ec)
  {
    // send immediately
    m_vRtpInterfaces[uiInterfaceIndex]->send(compoundRtcp, ep);
  }
}

void RtpForwarder::onIncomingRtp( const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex )
{
  double dRtt = extractRTTFromRtpPacket(rtpPacket);
  EndPoint& forwardTo = m_map[ep.getAddress()][ep.getPort()];
  NetworkProperties_t& networkProps = m_networkMap[ep.getAddress()][ep.getPort()];

  LOG_EVERY_N(INFO, 100) << "Received RTP from " << ep.getAddress() << ":" << ep.getPort()
                         << " One way RTT: " << dRtt << "s Fwd: " << forwardTo.getAddress() << ":" << forwardTo.getPort();

  uint32_t uiLoss = std::get<0>(networkProps);
  uint32_t uiOwd = std::get<1>(networkProps);
  uint32_t uiJitter = std::get<2>(networkProps);
  uint32_t uiRand = rand()%100;
  if (uiRand >= uiLoss)
  {
    if (uiOwd + uiJitter > 0)
    {
      boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
      // calculate random jitter value
      uint32_t uiJitterNow = rand()%uiJitter;
      double dBase = uiOwd - uiJitter/2.0;
      if (dBase < 0.0) dBase = 0.0;
      pTimer->expires_from_now(boost::posix_time::milliseconds(static_cast<uint32_t>(dBase + uiJitterNow)));
      pTimer->async_wait(boost::bind(&RtpForwarder::sendRtpPacket, shared_from_this(), _1, pTimer, rtpPacket, forwardTo, uiInterfaceIndex));
    }
    else
    {
      // send immediately
      m_vRtpInterfaces[uiInterfaceIndex]->send(rtpPacket, forwardTo);
    }
  }
  else
  {
    VLOG(1) << "Dropping packet " << rtpPacket.getSequenceNumber();
  }
}

void RtpForwarder::onOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint& ep, const uint32_t uiInterfaceIndex)
{
#if 0
  VLOG(1) << "Packet " << rtpPacket.getSequenceNumber() << " forwarded to " << ep << " on RTP interface " << uiInterfaceIndex;
#endif
}

void RtpForwarder::onIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex )
{
  VLOG(1) << "Received RTCP from " << ep.getAddress() << ":" << ep.getPort() << " on interface " << uiInterfaceIndex;
  EndPoint forwardTo = m_map[ep.getAddress()][ep.getPort()];
  NetworkProperties_t& networkProps = m_networkMap[ep.getAddress()][ep.getPort()];
  uint32_t uiOwd = std::get<1>(networkProps);
  uint32_t uiJitter = std::get<2>(networkProps);

  if (uiOwd + uiJitter > 0)
  {
    boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
    // calculate random jitter value
    uint32_t uiJitterNow = rand()%uiJitter;
    pTimer->expires_from_now(boost::posix_time::milliseconds(uiOwd + uiJitterNow));
    pTimer->async_wait(boost::bind(&RtpForwarder::sendRtcpPacket, shared_from_this(), _1, pTimer, compoundRtcp, forwardTo, uiInterfaceIndex));
  }
  else
  {
    // send immediately
    m_vRtpInterfaces[uiInterfaceIndex]->send(compoundRtcp, forwardTo);
  }
}

void RtpForwarder::onOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep, const uint32_t uiInterfaceIndex )
{
  VLOG(1) << "RTCP forwarded to " << ep << " on RTP interface " << uiInterfaceIndex;
}

}

