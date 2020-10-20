#include "CorePch.h"
#include <rtp++/network/UdpForwarder.h>
#include <boost/bind.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/make_shared.hpp>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

UdpForwarder::ptr UdpForwarder::create(boost::asio::io_service& ioService,
                                       const std::string& sBindIp,
                                       unsigned short uiBindPort,
                                       uint32_t uiPacketLossProbability,
                                       uint32_t uiOwd,
                                       uint32_t uiJitter)
{
  return boost::make_shared<UdpForwarder>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort, uiPacketLossProbability, uiOwd, uiJitter);
}

UdpForwarder::ptr UdpForwarder::create(boost::asio::io_service& ioService,
                                       const EndPoint& endpoint,
                                       uint32_t uiPacketLossProbability,
                                       uint32_t uiOwd,
                                       uint32_t uiJitter)
{
  return boost::make_shared<UdpForwarder>(boost::ref(ioService), endpoint.getAddress(), endpoint.getPort(), uiPacketLossProbability, uiOwd, uiJitter);
}

UdpForwarder::ptr UdpForwarder::create(boost::asio::io_service& ioService,
                                       const std::string& sBindIp,
                                       unsigned short uiBindPort,
                                       uint32_t uiPacketLossProbability,
                                       const std::vector<double>& vDelays)
{
  return boost::make_shared<UdpForwarder>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort, uiPacketLossProbability, vDelays);
}

UdpForwarder::ptr UdpForwarder::create(boost::asio::io_service& ioService,
                                       const EndPoint& endpoint,
                                       uint32_t uiPacketLossProbability,
                                       const std::vector<double>& vDelays)
{
  return boost::make_shared<UdpForwarder>(boost::ref(ioService), endpoint.getAddress(), endpoint.getPort(), uiPacketLossProbability, vDelays);
}

UdpForwarder::UdpForwarder(boost::asio::io_service& ioService,
                           const std::string& sBindIp, unsigned short uiBindPort,
                           uint32_t uiPacketLossProbability,
                           uint32_t uiOwd,
                           uint32_t uiJitter)
  :m_rIoService(ioService),
    m_timer(ioService),
    m_sIpAddress(sBindIp),
    m_uiPort(uiBindPort),
    m_address(boost::asio::ip::address::from_string(sBindIp)),
    m_endpoint(m_address, uiBindPort),
    m_socket(m_rIoService),
    m_bTimeOut(false),
    m_uiTimeoutMs(1000),
    m_uiPacketLossProbability(uiPacketLossProbability),
    m_bUseJitter(true),
    m_uiOwdMs(uiOwd),
    m_uiJitterMs(uiJitter),
    m_uiDelayIndex(0)
{
  VLOG(2) << "Creating UDP forwarder: Loss: " << m_uiPacketLossProbability
          << " OWD: " << uiOwd
          << " Jitter: " << m_uiJitterMs;
  initialise();
}


UdpForwarder::UdpForwarder(boost::asio::io_service& ioService,
                           const std::string& sBindIp, unsigned short uiBindPort,
                           uint32_t uiPacketLossProbability, const std::vector<double>& vDelays)
  :m_rIoService(ioService),
    m_timer(ioService),
    m_sIpAddress(sBindIp),
    m_uiPort(uiBindPort),
    m_address(boost::asio::ip::address::from_string(sBindIp)),
    m_endpoint(m_address, uiBindPort),
    m_socket(m_rIoService),
    m_bTimeOut(false),
    m_uiTimeoutMs(1000),
    m_uiPacketLossProbability(uiPacketLossProbability),
    m_bUseJitter(false),
    m_uiOwdMs(0),
    m_uiJitterMs(0),
    m_vDelays(vDelays),
    m_uiDelayIndex(0)
{
  initialise();
}

void UdpForwarder::send(Buffer networkPacket, const EndPoint& endpoint)
{
  boost::mutex::scoped_lock l(m_lock);
  bool bBusyWriting = !m_vDeliveryQueue.empty();
  m_vDeliveryQueue.push_back( std::make_pair(networkPacket, endpoint) );
  if (!bBusyWriting)
  {
    boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(endpoint.getAddress()), endpoint.getPort());
    m_socket.async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
                            destination,
                            boost::bind(&UdpForwarder::writeCompletionHandler,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                            );
  }
  else
  {
#if 0
    DLOG(INFO) << "Packet queued, write in progress";
#endif
  }
}

void UdpForwarder::sendPacketDelayed(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, Buffer networkPacket, const EndPoint& endpoint)
{
  if (!ec)
  {
    send(networkPacket, endpoint);
  }
  m_mTimers.erase(pTimer);
  delete pTimer;
}

void UdpForwarder::close()
{
  using boost::asio::deadline_timer;
  VLOG(1) << "[" << this << "] Closing socket [" << m_sIpAddress << ":" << m_uiPort << "]";
  m_socket.close();

  // the async handler will reclaim all the memory
  std::for_each(m_mTimers.begin(), m_mTimers.end(), [this](const std::pair<deadline_timer*, deadline_timer*>& pair)
  {
    pair.second->cancel();
  });
  m_mTimers.clear();
}

void UdpForwarder::recv()
{
#if 0
  VLOG(2) << "[" << this << "] recv";
#endif

  m_socket.async_receive_from(
        boost::asio::buffer(m_data, max_length), m_lastSenderEndpoint,
        boost::bind(&UdpForwarder::readCompletionHandler, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

  if (m_bTimeOut)
  {
    /* Create a task that will be called if we wait more than 300ms */
    m_timer.expires_from_now(boost::posix_time::milliseconds(m_uiTimeoutMs));
    m_timer.async_wait(boost::bind(&UdpForwarder::timeoutHandler, shared_from_this(), boost::asio::placeholders::error));
  }
}

void UdpForwarder::initialise()
{
  if (m_address.is_multicast())
  {
    // HACK FOR MULTICAST: to be re-thought?
    LOG(INFO) << "Address is multicast " << m_sIpAddress << ":" << m_uiPort;
    boost::asio::ip::udp::endpoint listen_endpoint(boost::asio::ip::address::from_string("0.0.0.0"), m_uiPort);
    m_socket.open(listen_endpoint.protocol());
    m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    VLOG(1) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";
    m_socket.bind(listen_endpoint);

    // Join the multicast group to receive data
    m_socket.set_option(
          boost::asio::ip::multicast::join_group(m_address));

  }
  else
  {
    m_socket.open(m_endpoint.protocol());
    // Binding to port 0 should result in the OS selecting the port
    VLOG(1) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";
    boost::system::error_code ec;
    //m_socket.bind(m_endpoint, ec);
    m_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), m_uiPort), ec);
    if (ec)
    {
      LOG(WARNING) << "Error binding socket: " << ec.message();
    }

  }

  // it helps to increase the buffer size to lessen packet loss
  boost::asio::socket_base::receive_buffer_size option(udp_receiver_buffer_size_kb);
  m_socket.set_option(option);
}

void UdpForwarder::writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  boost::mutex::scoped_lock l(m_lock);

  NetworkPackage_t package = m_vDeliveryQueue.front();
  m_vDeliveryQueue.pop_front();

  if (!ec)
  {
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort << "] Sent " << bytes_transferred << " to " << package.second.getAddress() << ":" << package.second.getPort();

    // send next sample if one was queued in the meantime
    if ( !m_vDeliveryQueue.empty() )
    {
#if 0
      DLOG(INFO) << "Sending queued packet Queue size: " << m_vDeliveryQueue.size();
#endif
      NetworkPackage_t package = m_vDeliveryQueue.front();
      Buffer networkPacket = package.first;
      EndPoint ep = package.second;
      boost::asio::ip::udp::endpoint endPoint(boost::asio::ip::address::from_string(ep.getAddress()), ep.getPort());
      m_socket.async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
                              endPoint,
                              boost::bind(&UdpForwarder::writeCompletionHandler,
                                          shared_from_this(),
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred)
                              );
    }
  }
  else
  {
    LOG(WARNING) << "Send failed: ", ec.message();
  }
}

void UdpForwarder::timeoutHandler(const boost::system::error_code& ec)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    LOG(WARNING) << "Receive timeout out";
    // socket read timed out
    if (m_fnOnTimeout) m_fnOnTimeout(shared_from_this());
  }
}

void UdpForwarder::readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received)
{
  if (m_bTimeOut)
  {
    // cancel timeout
    m_timer.cancel();
  }

  EndPoint ep;
  if (!ec )
  {
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort  << "] Received " << bytes_received << " from " << m_lastSenderEndpoint.address().to_string() << ":" << m_lastSenderEndpoint.port();

    uint32_t uiRand = rand()%100;
    if (uiRand >= m_uiPacketLossProbability)
    {
      uint8_t* pData = new uint8_t[bytes_received];
      memcpy(pData, m_data, bytes_received);
      Buffer networkPacket;
      networkPacket.setData(pData, bytes_received);
      ep.setAddress(m_lastSenderEndpoint.address().to_string());
      ep.setPort(m_lastSenderEndpoint.port());
      // check if the packet is from an unknown source???
      auto it = m_map.find(ep.getAddress());
      if (it == m_map.end())
      {
        LOG(WARNING) << m_sIpAddress << ":" << m_uiPort << " Packet from unknown source: " << ep;
        for (auto& it: m_map)
        {
          for (auto& it2: it.second)
          {
            VLOG(2) << "FWD MAP INFO IP: " << it.first << ":" << it2.first << " " << it2.second;
          }
        }
        recv();
        return;
      }
      else
      {
        auto it2 = it->second.find(ep.getPort());
        if (it2 == it->second.end() )
        {
          LOG(WARNING) << m_sIpAddress << ":" << m_uiPort << " Packet from unknown source port: " << ep;
          for (auto& it: m_map)
          {
            for (auto& it2: it.second)
            {
              VLOG(2) << "FWD MAP INFO IP: " << it.first << ":" << it2.first << " " << it2.second;
            }
          }
          recv();
          return;
        }
        else
        {
          EndPoint forwardTo = m_map[ep.getAddress()][ep.getPort()];
          if (m_bUseJitter)
          {
            if (m_uiOwdMs + m_uiJitterMs > 0)
            {
              // send packet delayed
              boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
              // calculate random jitter value
              uint32_t uiWait = m_uiOwdMs;
              if (m_uiJitterMs)
              {
                uint32_t uiJitter = rand()%m_uiJitterMs;
                double dBase = static_cast<double>(m_uiOwdMs);
                dBase -= m_uiJitterMs/2.0;
                if (dBase < 0.0) dBase = 0.0;
                uiWait = static_cast<uint32_t>(dBase + uiJitter);
              }
              pTimer->expires_from_now(boost::posix_time::milliseconds(uiWait));
              pTimer->async_wait(boost::bind(&UdpForwarder::sendPacketDelayed, shared_from_this(), _1, pTimer, networkPacket, forwardTo));
              m_mTimers.insert(std::make_pair(pTimer, pTimer));
            }
            else
            {
              // send packet immediately
              send(networkPacket, forwardTo);
            }
          }
          else
          {
            if (!m_vDelays.empty())
            {
              uint32_t uiDelayMs = static_cast<uint32_t>(m_vDelays[m_uiDelayIndex] * 1000 + 0.5);

              // send packet delayed
              boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_rIoService);
              pTimer->expires_from_now(boost::posix_time::milliseconds(uiDelayMs));
              pTimer->async_wait(boost::bind(&UdpForwarder::sendPacketDelayed, shared_from_this(), _1, pTimer, networkPacket, forwardTo));
              m_mTimers.insert(std::make_pair(pTimer, pTimer));

              m_uiDelayIndex = (m_uiDelayIndex + 1) % m_vDelays.size();
            }
            else
            {
              // send packet immediately
              send(networkPacket, forwardTo);
            }
          }
        }
      }
    }
    else
    {
      VLOG(1) << "Dropping packet ";
    }

    recv();
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Receive failed: " << ec.message();
    }
  }
}

}
