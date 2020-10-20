#include "CorePch.h"
#include <rtp++/network/UdpSocketWrapper.h>
#include <boost/bind.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <rtp++/RtpTime.h>

using namespace boost::asio::ip;

namespace rtp_plus_plus
{

UdpSocketWrapper::ptr UdpSocketWrapper::create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
{
  return boost::make_shared<UdpSocketWrapper>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort);
}

UdpSocketWrapper::ptr UdpSocketWrapper::create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, boost::system::error_code& ec)
{
  return boost::make_shared<UdpSocketWrapper>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort, boost::ref(ec));
}

UdpSocketWrapper::ptr UdpSocketWrapper::create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, std::unique_ptr<boost::asio::ip::udp::socket> pSocket)
{
  return boost::make_shared<UdpSocketWrapper>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort, std::move(pSocket));
}

UdpSocketWrapper::UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
  :m_rIoService(ioService),
  m_timer(ioService),
  m_sIpAddress(sBindIp),
  m_uiPort(uiBindPort),
  m_address(boost::asio::ip::address::from_string(sBindIp)),
  m_endpoint(m_address, uiBindPort),
  m_pSocket(new udp::socket(m_rIoService)),
  m_bTimeOut(false),
  m_uiTimeoutMs(1000)
{
  boost::system::error_code ec = initialise();
  if (ec)
  {
    boost::system::system_error e(ec);
    boost::throw_exception(e);
  }
}

UdpSocketWrapper::UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, boost::system::error_code& ec)
  :m_rIoService(ioService),
  m_timer(ioService),
  m_sIpAddress(sBindIp),
  m_uiPort(uiBindPort),
  m_address(boost::asio::ip::address::from_string(sBindIp)),
  m_endpoint(m_address, uiBindPort),
  m_pSocket(new udp::socket(m_rIoService)),
  m_bTimeOut(false),
  m_uiTimeoutMs(1000)
{
  ec = initialise();
}

UdpSocketWrapper::UdpSocketWrapper(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort, std::unique_ptr<boost::asio::ip::udp::socket> pSocket)
  :m_rIoService(ioService),
  m_timer(ioService),
  m_sIpAddress(sBindIp),
  m_uiPort(uiBindPort),
  m_address(boost::asio::ip::address::from_string(sBindIp)),
  m_endpoint(m_address, uiBindPort),
  m_pSocket(std::move(pSocket)),
  m_bTimeOut(false),
  m_uiTimeoutMs(1000)
{

}

UdpSocketWrapper::~UdpSocketWrapper()
{
  VLOG(10) << "[" << this << "] Destructor";
}

void UdpSocketWrapper::send(Buffer networkPacket, const EndPoint& endpoint)
{
  boost::mutex::scoped_lock l(m_lock);
  bool bBusyWriting = !m_vDeliveryQueue.empty();
  m_vDeliveryQueue.push_back( std::make_pair(networkPacket, endpoint) );
  if (!bBusyWriting)
  {
    boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(endpoint.getAddress()), endpoint.getPort());
    m_pSocket->async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
      destination,
      boost::bind(&UdpSocketWrapper::writeCompletionHandler,
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

void UdpSocketWrapper::close()
{
  VLOG(10) << "[" << this << "] Closing socket [" << m_sIpAddress << ":" << m_uiPort << "]";
  m_pSocket->close();
}

void UdpSocketWrapper::recv()
{
  m_pSocket->async_receive_from(
    boost::asio::buffer(m_data, max_length), m_lastSenderEndpoint,
    boost::bind(&UdpSocketWrapper::readCompletionHandler, shared_from_this(),
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));

  if (m_bTimeOut)
  {
    /* Create a task that will be called if we wait more than 300ms */
    m_timer.expires_from_now(boost::posix_time::milliseconds(m_uiTimeoutMs));
    m_timer.async_wait(boost::bind(&UdpSocketWrapper::timeoutHandler, shared_from_this(), boost::asio::placeholders::error));
  }
}

boost::system::error_code UdpSocketWrapper::initialise()
{
  if (m_address.is_multicast())
  {
    // HACK FOR MULTICAST: to be re-thought?
    LOG(INFO) << "Address is multicast " << m_sIpAddress << ":" << m_uiPort;
    boost::asio::ip::udp::endpoint listen_endpoint(boost::asio::ip::address::from_string("0.0.0.0"), m_uiPort);
    m_pSocket->open(listen_endpoint.protocol());
    m_pSocket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
    VLOG(10) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";
    boost::system::error_code ec;
    m_pSocket->bind(listen_endpoint, ec);
    if (ec)
    {
      LOG(WARNING) << "Failed to bind socket to port " << m_uiPort;
      return ec;
    }
    // Join the multicast group to receive data
    m_pSocket->set_option(
      boost::asio::ip::multicast::join_group(m_address));

  }
  else
  {
    m_pSocket->open(m_endpoint.protocol());
    // Binding to port 0 should result in the OS selecting the port
    VLOG(10) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";
    boost::system::error_code ec;
    //m_pSocket->bind(m_endpoint, ec);
    //m_pSocket->bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), m_uiPort), ec);
    m_pSocket->bind(m_endpoint, ec);
    if (ec)
    {
      LOG(WARNING) << "Failed to bind socket to port " << m_uiPort;
      return ec;
    }
  }

  boost::asio::socket_base::receive_buffer_size getOption;
  boost::system::error_code ecDummy;
  m_pSocket->get_option(getOption, ecDummy);
  VLOG(10) << "UDP socket buffer size before: " << getOption.value();

  // it helps to increase the buffer size to lessen packet loss
  boost::asio::socket_base::receive_buffer_size option(udp_receiver_buffer_size_kb);

  m_pSocket->set_option(option);

  m_pSocket->get_option(getOption, ecDummy);
  VLOG(10) << "UDP socket buffer size after: " << getOption.value();
  return boost::system::error_code();
}

void UdpSocketWrapper::writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  boost::mutex::scoped_lock l(m_lock);

  NetworkPackage_t package = m_vDeliveryQueue.front();
  m_vDeliveryQueue.pop_front();
  if (m_fnOnSend)
    m_fnOnSend(ec, shared_from_this(), package.first, package.second);
  else
    VLOG(2) << "Send callback has not been set";

  if (!ec)
  {
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort << "] Sent " << bytes_transferred << " to " << package.second.getAddress() << ":" << package.second.getPort();

    // send next sample if one was queued in the meantime
    if ( !m_vDeliveryQueue.empty() )
    {
#if 0
      DLOG(INFO) << "Sending queued packet Queue size: " << m_vDeliveryQueue.size();
#else
      size_t size = m_vDeliveryQueue.size();
      VLOG_IF(5, size > 1) << "Send queue: " << size;
#endif
      NetworkPackage_t package = m_vDeliveryQueue.front();
      Buffer networkPacket = package.first;
      EndPoint ep = package.second;
      boost::asio::ip::udp::endpoint endPoint(boost::asio::ip::address::from_string(ep.getAddress()), ep.getPort());
      m_pSocket->async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
        endPoint,
        boost::bind(&UdpSocketWrapper::writeCompletionHandler,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)
        );
    }
  }
  else
  {
    LOG(WARNING) << "Send failed from " << m_sIpAddress << ":" << m_uiPort << " to " << package.second << " Ec: " << ec.message();
  }
}

void UdpSocketWrapper::timeoutHandler(const boost::system::error_code& ec)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // socket read timed out
    if (m_fnOnTimeout) m_fnOnTimeout(shared_from_this());
    else
      VLOG(2) << "Timeout callback has not been set";
  }
}

void UdpSocketWrapper::readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received)
{
// #define DEBUG_NTP
#ifdef DEBUG_NTP
  DLOG(INFO) << "NTP arrival time: " << convertNtpTimestampToPosixTime(getNTPTimeStamp());
#endif

  NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
  if (m_bTimeOut)
  {
    // cancel timeout
    m_timer.cancel();
  }

  EndPoint ep;
  if (!ec )
  {
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort  << "] Received " << bytes_received << " from " << m_lastSenderEndpoint.address().to_string() << ":" << m_lastSenderEndpoint.port();

    uint8_t* pData = new uint8_t[bytes_received];
    memcpy(pData, m_data, bytes_received);
    networkPacket.setData(pData, bytes_received);
    ep.setAddress(m_lastSenderEndpoint.address().to_string());
    ep.setPort(m_lastSenderEndpoint.port());
  }
  // let calling clas handle error and logging
#if 0
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Receive failed: " << ec.message();
    }
  }
#endif
  if (m_fnOnRecv)
    m_fnOnRecv(ec, shared_from_this(), networkPacket, ep);
  else
    VLOG(2) << "Recv callback has not been set";
}

} // rtp_plus_plus
