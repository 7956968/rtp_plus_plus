#include "CorePch.h"
#include <rtp++/network/DccpRtpConnection.h>
#ifdef ENABLE_ASIO_DCCP
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/make_shared.hpp>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>
#include <rtp++/network/NetworkPacket.h>

using namespace boost::asio::ip;

namespace rtp_plus_plus
{

DccpRtpConnection::ptr DccpRtpConnection::create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
{
  return boost::make_shared<DccpRtpConnection>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort);
}

DccpRtpConnection::DccpRtpConnection(boost::asio::io_service& ioService)
  :m_rIoService(ioService),
    m_timer(ioService),
    m_socket(m_rIoService),
    m_bConnectionInProgress(false)
{
  VLOG(2) << "DccpRtpConnection constructor";
}

DccpRtpConnection::DccpRtpConnection(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
  :m_rIoService(ioService),
    m_timer(ioService),
    m_sIpAddress(sBindIp),
    m_uiPort(uiBindPort),
    m_address(boost::asio::ip::address::from_string(sBindIp)),
    m_endpoint(m_address, uiBindPort),
    #if 0
    m_socket(m_rIoService, boost::asio::ip::dccp::endpoint(boost::asio::ip::dccp::v4(), uiBindPort)),
    #else
    m_socket(m_rIoService),
    #endif
    m_bConnectionInProgress(false)
{
  initialise();
  VLOG(2) << "DccpRtpConnection constructor: socket bound to " << sBindIp << ":" << uiBindPort;
}

DccpRtpConnection::~DccpRtpConnection()
{
  DLOG(INFO) << "[" << this << "] Destructor";
}

void DccpRtpConnection::start(dccp::resolver::iterator endpoint_iter)
{
  connect(endpoint_iter);
}

void DccpRtpConnection::read()
{
#if 0
  VLOG(2) << "DCCP read";
#endif
  m_socket.async_receive_from(
        boost::asio::buffer(m_data, max_length), m_lastSenderEndpoint,
        boost::bind(&DccpRtpConnection::readCompletionHandler, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void DccpRtpConnection::start()
{
  read();
}

void DccpRtpConnection::stop()
{
  m_socket.close();
}

void DccpRtpConnection::connect(dccp::resolver::iterator endpoint_iter)
{
  //tcp::resolver resolver(m_rIoService);
  //// TODO:
  //std::string sRemoteIp = "127.0.0.1";
  //uint16_t uiRemotePort = 49170;
  //tcp::resolver::query query(sRemoteIp, toString(uiRemotePort));
  //tcp::resolver::iterator iterator = resolver.resolve(query);

  dccp::endpoint endpoint = *endpoint_iter;
  socket().async_connect(endpoint,
                         boost::bind(&DccpRtpConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++endpoint_iter));
  m_bConnectionInProgress = true;
}

void DccpRtpConnection::handleConnect( const boost::system::error_code& error, dccp::resolver::iterator endpointIterator )
{
  if (!error)
  {
    LOG(INFO) << "Connection success";
    m_bConnectionInProgress = false;
    // start read
    start();
    // check for queued writes
    // send next sample if one was queued in the meantime
    if ( !m_vDeliveryQueue.empty() )
    {
      NetworkPackage_t package = m_vDeliveryQueue.front();
      Buffer networkPacket = package.first;
      EndPoint ep = package.second;

      boost::asio::ip::dccp::endpoint destination(boost::asio::ip::address::from_string(ep.getAddress()), ep.getPort());
      m_socket.async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
                              destination,
                              boost::bind(&DccpRtpConnection::writeCompletionHandler,
                                          shared_from_this(),
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred)
                              );
    }
  }
  else
  {
    if (error == boost::asio::error::invalid_argument)
    {
      if (endpointIterator == dccp::resolver::iterator())
      {
        // Failed to connect
        LOG(WARNING) << "Failed to connect using endpoint! No more endpoints.";
        // TODO: how to notify application: will a read or write on the unconnected socket trigger
        // an error?
        //m_writeCompletionHandler(error, pRequest, shared_from_this());
      }
      else
      {
        dccp::endpoint endpoint = *endpointIterator;
        socket().async_connect(endpoint, boost::bind(&DccpRtpConnection::handleConnect,
                                                     shared_from_this(), boost::asio::placeholders::error, ++endpointIterator));
      }
    }
    else
    {
      // TODO: how to notify application: will a read or write on the unconnected socket trigger
      // an error?
      //m_writeCompletionHandler(error, pRequest, shared_from_this());
    }
  }
}

void DccpRtpConnection::send(Buffer networkPacket, const EndPoint& endpoint)
{
  VLOG(10) << "Outgoing data packet of size " << networkPacket.getSize();
  boost::mutex::scoped_lock l(m_lock);
  bool bBusyWriting = !m_vDeliveryQueue.empty();
  m_vDeliveryQueue.push_back( std::make_pair(networkPacket, endpoint) );

  if (!m_bConnectionInProgress && !bBusyWriting)
  {
    boost::asio::ip::dccp::endpoint destination(boost::asio::ip::address::from_string(endpoint.getAddress()), endpoint.getPort());
    m_socket.async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
                            destination,
                            boost::bind(&DccpRtpConnection::writeCompletionHandler,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                            );
  }
  else
  {
#if 1
    VLOG(10) << "Packet queued, connection or write in progress";
#endif
  }
}

void DccpRtpConnection::close()
{
  VLOG(1) << "[" << this << "] Closing socket [" << m_sIpAddress << ":" << m_uiPort << "]";
  m_socket.close();
}

void DccpRtpConnection::initialise()
{
  m_socket.open(m_endpoint.protocol());
  // Binding to port 0 should result in the OS selecting the port
  VLOG(1) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";

  // this needs to happen before the bind
  boost::asio::socket_base::reuse_address option(true);
  m_socket.set_option(option);

  boost::system::error_code ec;
  m_socket.bind(m_endpoint, ec);

  if (ec)
  {
    LOG(WARNING) << "Error binding socket to " << m_sIpAddress << ":" << m_uiPort << " " << ec.message();
  }
  // it helps to increase the buffer size to lessen packet loss
  //boost::asio::socket_base::receive_buffer_size option(udp_receiver_buffer_size_kb);
  //m_socket.set_option(option);
}

void DccpRtpConnection::writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  boost::mutex::scoped_lock l(m_lock);

  NetworkPackage_t package = m_vDeliveryQueue.front();
  m_vDeliveryQueue.pop_front();
  if (m_fnOnSend) m_fnOnSend(ec, shared_from_this(), package.first, package.second);

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
      boost::asio::ip::dccp::endpoint endPoint(boost::asio::ip::address::from_string(ep.getAddress()), ep.getPort());
      m_socket.async_send_to( boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
                              endPoint,
                              boost::bind(&DccpRtpConnection::writeCompletionHandler,
                                          shared_from_this(),
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred)
                              );
    }
  }
  else
  {
    LOG(WARNING) << "Send failed: Code: " << ec.value() << " Message: " << ec.message();
  }
}

void DccpRtpConnection::timeoutHandler(const boost::system::error_code& ec)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // socket read timed out
    if (m_fnOnTimeout) m_fnOnTimeout(shared_from_this());
  }
}

void DccpRtpConnection::readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received)
{
#if 0
  VLOG(2) << "DCCP read completion handler bytes_received: " << bytes_received;
#endif
  // #define DEBUG_NTP
#ifdef DEBUG_NTP
  DLOG(INFO) << "NTP arrival time: " << convertNtpTimestampToPosixTime(getNTPTimeStamp());
#endif

  NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
  boost::system::error_code error_code_to_return = ec;

  EndPoint ep;
  // the DCCP stack integration seems to result in a zero read size instead of an error code
  if (!ec && bytes_received > 0)
  {
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort  << "] Received " << bytes_received << " from " << m_lastSenderEndpoint.address().to_string() << ":" << m_lastSenderEndpoint.port();

    uint8_t* pData = new uint8_t[bytes_received];
    memcpy(pData, m_data, bytes_received);
    networkPacket.setData(pData, bytes_received);
    ep.setAddress(m_lastSenderEndpoint.address().to_string());
    ep.setPort(m_lastSenderEndpoint.port());
    read();
  }
  // let calling clas handle error and logging
#if 1
  else
  {
    if (bytes_received == 0)
    {
      LOG(WARNING) << "TOREVISE: Zero size DCCP read. Forcing error to connection reset";
      // force error for now since DCCP stack does not seem to report error code here?
      error_code_to_return = boost::asio::error::connection_reset;
    }
    else
    {
      if (ec != boost::asio::error::operation_aborted)
      {
        LOG(WARNING) << "Receive failed: " << ec.message();
      }
    }
  }
#endif
  if (m_fnOnRecv)
    m_fnOnRecv(error_code_to_return, shared_from_this(), networkPacket, ep);
}

}
#endif